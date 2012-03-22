#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/call/conn.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/CallChannel>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestCallChannel : public Test
{
    Q_OBJECT

public:
    TestCallChannel(QObject *parent = 0)
        : Test(parent), mConn(0)
    { }

protected Q_SLOTS:
    void expectRequestContentFinished(Tp::PendingOperation *op);
    void expectSuccessfulRequestReceiving(Tp::PendingOperation *op);

    void onContentRemoved(const Tp::CallContentPtr &content,
            const Tp::CallStateReason &reason);
    void onCallStateChanged(Tp::CallState newState);
    void onCallFlagsChanged(Tp::CallFlags newFlags);
    void onRemoteMemberFlagsChanged(
            const QHash<Tp::ContactPtr, Tp::CallMemberFlags> &remoteMemberFlags,
            const Tp::CallStateReason &reason);
    void onRemoteMembersRemoved(const Tp::Contacts &remoteMembers,
            const Tp::CallStateReason &reason);
    void onLocalSendingStateChanged(Tp::SendingState localSendingState,
            const Tp::CallStateReason &reason);
    void onRemoteSendingStateChanged(
            const QHash<Tp::ContactPtr, Tp::SendingState> &remoteSendingStates,
            const Tp::CallStateReason &reason);

    void onLocalHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);

    void onNewChannels(const Tp::ChannelDetailsList &details);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testOutgoingCall();
    void testIncomingCall();
    void testHold();
    void testHangup();
    void testCallMembers();
    void testDTMF();
    void testFeatureCore();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;

    CallChannelPtr mChan;
    CallContentPtr mRequestContentReturn;
    CallContentPtr mContentRemoved;
    CallStateReason mCallStateReason;
    CallState mCallState;
    CallFlags mCallFlags;
    QHash<ContactPtr, CallMemberFlags> mRemoteMemberFlags;
    Contacts mRemoteMembersRemoved;
    SendingState mLSSCReturn;
    QQueue<uint> mLocalHoldStates;
    QQueue<uint> mLocalHoldStateReasons;

    // Remote sending state changed state-machine
    enum {
        RSSCStateInitial,
        RSSCStatePendingSend,
        RSSCStateSending,
        RSSCStateDone
    } mRSSCState;
    int mSuccessfulRequestReceivings;
};

void TestCallChannel::expectRequestContentFinished(Tp::PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(2);
        return;
    }

    if (!op->isValid()) {
        qWarning() << "inconsistent results";
        mLoop->exit(3);
        return;
    }

    PendingCallContent *pmc = qobject_cast<PendingCallContent*>(op);
    mRequestContentReturn = pmc->content();
    mLoop->exit(0);
}

void TestCallChannel::onLocalSendingStateChanged(Tp::SendingState state,
        const Tp::CallStateReason &reason)
{
    qDebug() << "local sending state changed";
    mLSSCReturn = state;
    mCallStateReason = reason;
    mLoop->exit(0);
}

void TestCallChannel::expectSuccessfulRequestReceiving(Tp::PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(2);
        return;
    }

    if (!op->isValid()) {
        qWarning() << "inconsistent results";
        mLoop->exit(3);
        return;
    }

    if (++mSuccessfulRequestReceivings == 2 && mRSSCState == RSSCStateDone) {
        mLoop->exit(0);
    }
}

void TestCallChannel::onContentRemoved(const CallContentPtr &content,
        const Tp::CallStateReason &reason)
{
    mContentRemoved = content;
    mCallStateReason = reason;
    mLoop->exit(0);
}

void TestCallChannel::onCallStateChanged(CallState newState)
{
    mCallState = newState;
    mLoop->exit(0);
}

void TestCallChannel::onCallFlagsChanged(CallFlags newFlags)
{
    mCallFlags = newFlags;
}

void TestCallChannel::onRemoteMemberFlagsChanged(
        const QHash<ContactPtr, CallMemberFlags> &remoteMemberFlags,
        const CallStateReason &reason)
{
    mRemoteMemberFlags = remoteMemberFlags;
    mLoop->exit(0);
}

void TestCallChannel::onRemoteMembersRemoved(const Tp::Contacts &remoteMembers,
        const Tp::CallStateReason &reason)
{
    mRemoteMembersRemoved = remoteMembers;
}

void TestCallChannel::onRemoteSendingStateChanged(
        const QHash<Tp::ContactPtr, SendingState> &states,
        const Tp::CallStateReason &reason)
{
    // There should be no further events
    QVERIFY(mRSSCState != RSSCStateDone);

    QCOMPARE(states.size(), 1);
    Tp::ContactPtr otherContact = states.keys().first();

    CallContentPtr content = mChan->contentsForType(Tp::MediaStreamTypeVideo).first();
    QVERIFY(content);

    CallStreamPtr stream = content->streams().first();
    QVERIFY(stream);

    if (mRSSCState == RSSCStateInitial) {
        QCOMPARE(states[otherContact], SendingStatePendingSend);
        mRSSCState = RSSCStatePendingSend;
    } else if (mRSSCState == RSSCStatePendingSend) {
        QCOMPARE(states[otherContact], SendingStateSending);
        mRSSCState = RSSCStateSending;

        QVERIFY(connect(stream->requestReceiving(otherContact, false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulRequestReceiving(Tp::PendingOperation*))));
    } else if (mRSSCState == RSSCStateSending) {
        QCOMPARE(states[otherContact], SendingStateNone);
        mRSSCState = RSSCStateDone;

        if (mSuccessfulRequestReceivings == 2) {
            mLoop->exit(0);
        }
    }

    qDebug() << "remote sending state changed to" << states[otherContact];
}

void TestCallChannel::onLocalHoldStateChanged(Tp::LocalHoldState localHoldState,
        Tp::LocalHoldStateReason localHoldStateReason)
{
    mLocalHoldStates.append(localHoldState);
    mLocalHoldStateReasons.append(localHoldStateReason);
    mLoop->exit(0);
}

void TestCallChannel::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    qDebug() << "new channels";
    Q_FOREACH (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        bool requested = details.properties.value(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TP_QT_IFACE_CHANNEL_TYPE_CALL && !requested) {
            mChan = CallChannel::create(mConn->client(), details.channel.path(), details.properties);
            mLoop->exit(0);
        }
    }
}

void TestCallChannel::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("call-channel");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_CALL_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            "simulation-delay", 1,
            NULL);
    QCOMPARE(mConn->connect(Connection::FeatureSelfContact), true);
}

void TestCallChannel::init()
{
    initImpl();

    mRequestContentReturn.reset();
    mContentRemoved.reset();
    mCallStateReason = CallStateReason();
    mCallState = CallStateUnknown;
    mCallFlags = (CallFlags) 0;
    mRemoteMemberFlags.clear();
    mRemoteMembersRemoved.clear();
    mLSSCReturn = (Tp::SendingState) -1;
    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();
}

void TestCallChannel::testOutgoingCall()
{
    qDebug() << "requesting contact for alice";

    QList<ContactPtr> contacts = mConn->contacts(QStringList() << QLatin1String("alice"));
    QCOMPARE(contacts.size(), 1);

    ContactPtr otherContact = contacts.at(0);
    QVERIFY(otherContact);

    qDebug() << "creating the channel";

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                   true);
    mChan = CallChannelPtr::qObjectCast(mConn->createChannel(request));
    QVERIFY(mChan);

    qDebug() << "making the channel ready";

    Features features;
    features << CallChannel::FeatureCallState
             << CallChannel::FeatureContents;

    QVERIFY(connect(mChan->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureCallState));
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureContents));

    QCOMPARE(mChan->callState(), CallStatePendingInitiator);

    QVERIFY(connect(mChan->accept(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->callState(), CallStateInitialised);
    QCOMPARE(mChan->callStateReason().reason, (uint) CallStateChangeReasonUserRequested);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(callStateChanged(Tp::CallState)),
                    SLOT(onCallStateChanged(Tp::CallState))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCallState, CallStateAccepted);
    QCOMPARE(mChan->callState(), CallStateAccepted);

    QCOMPARE(mChan->contents().size(), 1);

    Tp::CallContentPtr content;
    content = mChan->contents().first();
    QVERIFY(content);
    QCOMPARE(content->name(), QString::fromLatin1("audio"));
    QCOMPARE(content->type(), Tp::MediaStreamTypeAudio);
    QCOMPARE(content->disposition(), Tp::CallContentDispositionInitial);
    QVERIFY(mChan->contentByName(QLatin1String("audio")));

/*
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));
    QVERIFY(mChan->groupContacts().contains(otherContact));
*/

    qDebug() << "calling requestContent with a bad type";
    // RequestContent with bad type must fail
    QVERIFY(connect(mChan->requestContent(QLatin1String("content1"), (Tp::MediaStreamType) -1,
                                          Tp::MediaStreamDirectionNone),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QVERIFY(!mRequestContentReturn);

    qDebug() << "calling requestContent with Audio";
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 1);

    mRequestContentReturn.reset();
    QVERIFY(connect(mChan->requestContent(QLatin1String("content1"), Tp::MediaStreamTypeAudio,
                                          Tp::MediaStreamDirectionBidirectional),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(!mRequestContentReturn.isNull());
    QCOMPARE(mRequestContentReturn->name(), QString(QLatin1String("content1")));
    QCOMPARE(mRequestContentReturn->type(), Tp::MediaStreamTypeAudio);
    QCOMPARE(mRequestContentReturn->disposition(), Tp::CallContentDispositionNone);

    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 2);

    qDebug() << "calling requestContent with Video";
    mRequestContentReturn.reset();
    QVERIFY(connect(mChan->requestContent(QLatin1String("content2"), Tp::MediaStreamTypeVideo,
                                          Tp::MediaStreamDirectionBidirectional),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(!mRequestContentReturn.isNull());
    QCOMPARE(mRequestContentReturn->name(), QString(QLatin1String("content2")));
    QCOMPARE(mRequestContentReturn->type(), Tp::MediaStreamTypeVideo);

    // test content removal
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 2);

    content = mChan->contentsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(content);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(contentRemoved(Tp::CallContentPtr,Tp::CallStateReason)),
                    SLOT(onContentRemoved(Tp::CallContentPtr,Tp::CallStateReason))));
    QVERIFY(connect(content->remove(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (mContentRemoved.isNull()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mContentRemoved, content);
    QCOMPARE(mCallStateReason.reason, (uint) Tp::CallStateChangeReasonUserRequested);

    // test content sending changed
    content = mChan->contentsForType(Tp::MediaStreamTypeVideo).first();
    Tp::CallStreamPtr stream = content->streams().first();
    QVERIFY(content);

    QVERIFY(connect(stream.data(),
                    SIGNAL(localSendingStateChanged(Tp::SendingState,Tp::CallStateReason)),
                    SLOT(onLocalSendingStateChanged(Tp::SendingState,Tp::CallStateReason))));

    qDebug() << "stopping sending";

    QCOMPARE(stream->localSendingState(), Tp::SendingStateSending);
    QVERIFY(stream->remoteMembers().contains(otherContact));

    QVERIFY(connect(stream->requestSending(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "stopping receiving";
    QVERIFY(connect(stream->requestReceiving(otherContact, false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "waiting until we're not sending and not receiving";

    while (stream->localSendingState() != Tp::SendingStateNone
            || stream->remoteSendingState(otherContact) != Tp::SendingStateNone) {
        qDebug() << "re-entering mainloop to wait for local and remote SSC -> None";
        // wait local and remote sending state change
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLSSCReturn, Tp::SendingStateNone);
    QCOMPARE(stream->localSendingState(), Tp::SendingStateNone);
    QCOMPARE(stream->remoteSendingState(otherContact), Tp::SendingStateNone);

    qDebug() << "re-enabling sending";

    mLSSCReturn = (Tp::SendingState) -1;

    QVERIFY(connect(stream->requestSending(true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (mLSSCReturn == (Tp::SendingState) -1) {
        qDebug() << "re-entering mainloop to wait for SSC -> Sending";
        // wait sending state change
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLSSCReturn, Tp::SendingStateSending);

    qDebug() << "flushing D-Bus events";
    processDBusQueue(mChan.data());

    qDebug() << "enabling receiving";

    mRSSCState = RSSCStateInitial;
    mSuccessfulRequestReceivings = 0;

    QVERIFY(connect(stream.data(),
                    SIGNAL(remoteSendingStateChanged(QHash<Tp::ContactPtr,Tp::SendingState>,Tp::CallStateReason)),
                    SLOT(onRemoteSendingStateChanged(QHash<Tp::ContactPtr,Tp::SendingState>,Tp::CallStateReason))));

    // test content receiving changed
    QVERIFY(connect(stream->requestReceiving(otherContact, true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulRequestReceiving(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(static_cast<uint>(mRSSCState), static_cast<uint>(RSSCStateDone));
}

void TestCallChannel::testIncomingCall()
{
    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("away"), QLatin1String("preparing for a test"));

    Tp::Client::ConnectionInterfaceRequestsInterface *connRequestsInterface =
        mConn->client()->optionalInterface<Tp::Client::ConnectionInterfaceRequestsInterface>();
    QVERIFY(connect(connRequestsInterface,
                    SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Tp::ChannelDetailsList&))));

    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("available"), QLatin1String("call me?"));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan);
    QCOMPARE(mChan->contents().size(), 0);

    Features features;
    features << CallChannel::FeatureCallState
             << CallChannel::FeatureContents;

    QVERIFY(connect(mChan->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureCallState));
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureContents));

    Tp::ContactPtr otherContact = mChan->initiatorContact();
    QVERIFY(otherContact);

    QCOMPARE(mChan->callState(), CallStateInitialised);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(callFlagsChanged(Tp::CallFlags)),
                    SLOT(onCallFlagsChanged(Tp::CallFlags))));

    QVERIFY(connect(mChan->setRinging(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mCallFlags.testFlag(CallFlagLocallyRinging));
    QVERIFY(mChan->callFlags().testFlag(CallFlagLocallyRinging));

    QVERIFY(connect(mChan->setQueued(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mCallFlags.testFlag(CallFlagLocallyQueued));
    QVERIFY(mChan->callFlags().testFlag(CallFlagLocallyQueued));

    QVERIFY(connect(mChan->accept(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->callState(), CallStateAccepted);
    QVERIFY(!mCallFlags.testFlag(CallFlagLocallyQueued));
    QVERIFY(!mCallFlags.testFlag(CallFlagLocallyRinging));
    QVERIFY(!mChan->callFlags().testFlag(CallFlagLocallyRinging));
    QVERIFY(!mChan->callFlags().testFlag(CallFlagLocallyQueued));

/*
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));
*/

    QCOMPARE(mChan->contents().size(), 1);
    Tp::CallContentPtr content = mChan->contents().first();
    QCOMPARE(content->channel(), mChan);
    QCOMPARE(content->type(), Tp::MediaStreamTypeAudio);

    qDebug() << "requesting a video stream";

    // Request video stream
    QVERIFY(connect(mChan->requestContent(QLatin1String("video_content"), Tp::MediaStreamTypeVideo,
                                          Tp::MediaStreamDirectionBidirectional),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    content = mRequestContentReturn;
    QCOMPARE(content->type(), Tp::MediaStreamTypeVideo);

    QCOMPARE(mChan->contents().size(), 2);
    QVERIFY(mChan->contents().contains(content));

    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test content removal
    content = mChan->contentsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(content);

    qDebug() << "removing the audio content";

    // call does not have the concept of removing streams, it will remove the content the stream
    // belongs
    QVERIFY(connect(mChan.data(),
                    SIGNAL(contentRemoved(Tp::CallContentPtr,Tp::CallStateReason)),
                    SLOT(onContentRemoved(Tp::CallContentPtr,Tp::CallStateReason))));

    QVERIFY(connect(content->remove(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (mContentRemoved.isNull()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mContentRemoved, content);
}

void TestCallChannel::testHold()
{
    QList<ContactPtr> contacts = mConn->contacts(QStringList() << QLatin1String("bob"));
    QCOMPARE(contacts.size(), 1);

    ContactPtr otherContact = contacts.at(0);
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                   true);
    mChan = CallChannelPtr::qObjectCast(mConn->createChannel(request));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(Tp::CallChannel::FeatureLocalHoldState),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureLocalHoldState));

    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(Tp::LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(Tp::LocalHoldStateReasonNone));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(localHoldStateChanged(Tp::LocalHoldState, Tp::LocalHoldStateReason)),
                    SLOT(onLocalHoldStateChanged(Tp::LocalHoldState, Tp::LocalHoldStateReason))));
    // Request hold
    QVERIFY(connect(mChan->requestHold(true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (mLocalHoldStates.size() != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(Tp::LocalHoldStatePendingHold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(Tp::LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(Tp::LocalHoldStateHeld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(Tp::LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(Tp::LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(Tp::LocalHoldStateReasonRequested));

    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();

    // Request unhold
    QVERIFY(connect(mChan->requestHold(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (mLocalHoldStates.size() != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(Tp::LocalHoldStatePendingUnhold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(Tp::LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(Tp::LocalHoldStateUnheld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(Tp::LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(Tp::LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(Tp::LocalHoldStateReasonRequested));
}

void TestCallChannel::testHangup()
{
    qDebug() << "requesting contact for alice";

    QList<ContactPtr> contacts = mConn->contacts(QStringList() << QLatin1String("alice"));
    QCOMPARE(contacts.size(), 1);

    ContactPtr otherContact = contacts.at(0);
    QVERIFY(otherContact);

    qDebug() << "creating the channel";

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                   true);
    mChan = CallChannelPtr::qObjectCast(mConn->createChannel(request));
    QVERIFY(mChan);

    qDebug() << "making the channel ready";

    QVERIFY(connect(mChan->becomeReady(CallChannel::FeatureCallState),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureCallState));

    QCOMPARE(mChan->callState(), CallStatePendingInitiator);

    QVERIFY(connect(mChan->hangup(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->callState(), CallStateEnded);
    QCOMPARE(mChan->callStateReason().reason, (uint) CallStateChangeReasonUserRequested);
}

void TestCallChannel::testCallMembers()
{
    qDebug() << "requesting contact for john";

    QList<ContactPtr> contacts = mConn->contacts(QStringList() << QLatin1String("john"));
    QCOMPARE(contacts.size(), 1);

    ContactPtr otherContact = contacts.at(0);
    QVERIFY(otherContact);

    qDebug() << "creating the channel";

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialVideo"),
                   true);
    mChan = CallChannelPtr::qObjectCast(mConn->createChannel(request));
    QVERIFY(mChan);

    qDebug() << "making the channel ready";

    Features features;
    features << CallChannel::FeatureCallState
             << CallChannel::FeatureCallMembers
             << CallChannel::FeatureContents;

    QVERIFY(connect(mChan->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureCallMembers));
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureContents));

    qDebug() << "accepting the call";

    QCOMPARE(mChan->callState(), CallStatePendingInitiator);
    QCOMPARE(mChan->remoteMembers().size(), 1);

    QVERIFY(connect(mChan->accept(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->callState(), CallStateInitialised);
    QCOMPARE(mChan->callStateReason().reason, (uint) CallStateChangeReasonUserRequested);

    qDebug() << "ringing on the remote side";

    QVERIFY(connect(mChan.data(),
                    SIGNAL(remoteMemberFlagsChanged(QHash<Tp::ContactPtr,Tp::CallMemberFlags>,Tp::CallStateReason)),
                    SLOT(onRemoteMemberFlagsChanged(QHash<Tp::ContactPtr,Tp::CallMemberFlags>,Tp::CallStateReason))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mChan->callState(), CallStateInitialised);
    QCOMPARE(mRemoteMemberFlags.size(), 1);
    QCOMPARE(mChan->remoteMembers().size(), 1);
    QVERIFY(mRemoteMemberFlags.constBegin().value().testFlag(CallMemberFlagRinging));
    QVERIFY(mChan->remoteMemberFlags(otherContact).testFlag(CallMemberFlagRinging));

    QVERIFY(disconnect(mChan.data(),
                       SIGNAL(remoteMemberFlagsChanged(QHash<Tp::ContactPtr,Tp::CallMemberFlags>,Tp::CallStateReason)),
                       this,
                       SLOT(onRemoteMemberFlagsChanged(QHash<Tp::ContactPtr,Tp::CallMemberFlags>,Tp::CallStateReason))));

    qDebug() << "remote contact answers";

    QVERIFY(connect(mChan.data(),
                    SIGNAL(callStateChanged(Tp::CallState)),
                    SLOT(onCallStateChanged(Tp::CallState))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mCallState, CallStateAccepted);
    QCOMPARE(mChan->callState(), CallStateAccepted);

    QVERIFY(disconnect(mChan.data(),
                       SIGNAL(callStateChanged(Tp::CallState)),
                       this,
                       SLOT(onCallStateChanged(Tp::CallState))));

    qDebug() << "testing members";

    QCOMPARE(mChan->contents().size(), 1);

    CallContentPtr content = mChan->contents().at(0);
    QCOMPARE(content->streams().size(), 1);

    QCOMPARE(mChan->remoteMembers().size(), 1);
    QCOMPARE(content->streams().at(0)->remoteMembers().size(), 1);

    ContactPtr contact1 = *mChan->remoteMembers().constBegin();
    ContactPtr contact2 = *content->streams().at(0)->remoteMembers().constBegin();

    QCOMPARE(contact1->id(), QString::fromLatin1("john"));
    QCOMPARE(contact2->id(), QString::fromLatin1("john"));

    qDebug() << "hanging up";

    QVERIFY(connect(mChan.data(),
                    SIGNAL(remoteMembersRemoved(Tp::Contacts,Tp::CallStateReason)),
                    SLOT(onRemoteMembersRemoved(Tp::Contacts,Tp::CallStateReason))));

    QVERIFY(connect(mChan->hangup(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->callState(), CallStateEnded);
    QCOMPARE(mChan->callStateReason().reason, (uint) CallStateChangeReasonUserRequested);

    QCOMPARE(mRemoteMembersRemoved.size(), 1);
    QCOMPARE((*mRemoteMembersRemoved.constBegin())->id(), QString::fromLatin1("john"));
    QCOMPARE(mChan->remoteMembers().size(), 0);
    QCOMPARE(mChan->contents().size(), 0);
}

void TestCallChannel::testDTMF()
{
    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("away"), QLatin1String("preparing for a test"));

    Tp::Client::ConnectionInterfaceRequestsInterface *connRequestsInterface =
        mConn->client()->optionalInterface<Tp::Client::ConnectionInterfaceRequestsInterface>();
    QVERIFY(connect(connRequestsInterface,
                    SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Tp::ChannelDetailsList&))));

    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("available"), QLatin1String("call me?"));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan);

    qDebug() << "making the channel ready";

    QVERIFY(connect(mChan->becomeReady(CallChannel::FeatureContents),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureContents));

    QVERIFY(connect(mChan->accept(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mChan->contents().size(), 1);
    Tp::CallContentPtr content = mChan->contents().first();
    QCOMPARE(content->channel(), mChan);
    QCOMPARE(content->type(), Tp::MediaStreamTypeAudio);

/*  TODO enable when/if the example call CM actually supports DTMF
    QVERIFY(content->supportsDTMF());
    QVERIFY(connect(content->startDTMFTone(DTMFEventDigit0),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(content->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(content->startDTMFTone((DTMFEvent) 1234),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, QString(TP_QT_ERROR_INVALID_ARGUMENT));
*/

    qDebug() << "requesting video content";

    QVERIFY(connect(mChan->requestContent(QLatin1String("video_content"),
                                          Tp::MediaStreamTypeVideo,
                                          Tp::MediaStreamDirectionBidirectional),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    content = mRequestContentReturn;
    QCOMPARE(content->type(), Tp::MediaStreamTypeVideo);

    QVERIFY(!content->supportsDTMF());
    QVERIFY(connect(content->startDTMFTone(DTMFEventDigit0),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, QString(TP_QT_ERROR_NOT_IMPLEMENTED));

    QVERIFY(connect(content->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, QString(TP_QT_ERROR_NOT_IMPLEMENTED));
}

void TestCallChannel::testFeatureCore()
{
    qDebug() << "requesting contact for alice";

    QList<ContactPtr> contacts = mConn->contacts(QStringList() << QLatin1String("alice"));
    QCOMPARE(contacts.size(), 1);

    ContactPtr otherContact = contacts.at(0);
    QVERIFY(otherContact);

    qDebug() << "creating the channel";

    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   TP_QT_IFACE_CHANNEL_TYPE_CALL);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".InitialAudio"),
                   true);
    mChan = CallChannelPtr::qObjectCast(mConn->createChannel(request));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(Tp::CallChannel::FeatureCore));

    QVERIFY(mChan->hasInitialAudio());
    QCOMPARE(mChan->initialAudioName(), QString::fromLatin1("audio"));
    QVERIFY(!mChan->hasInitialVideo());
    QCOMPARE(mChan->initialVideoName(), QString::fromLatin1("video"));
    QVERIFY(mChan->hasMutableContents());
    QVERIFY(mChan->handlerStreamingRequired());

    qDebug() << "creating second CallChannel object";

    //this object is not passed immutable properties on
    //the constructor, so it will have to introspect them.
    CallChannelPtr chan2 = CallChannel::create(mConn->client(), mChan->objectPath(), QVariantMap());

    QVERIFY(connect(chan2->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(chan2->isReady(Tp::CallChannel::FeatureCore));

    QVERIFY(chan2->hasInitialAudio());
    QCOMPARE(chan2->initialAudioName(), QString::fromLatin1("audio"));
    QVERIFY(!chan2->hasInitialVideo());
    QCOMPARE(chan2->initialVideoName(), QString::fromLatin1("video"));
    QVERIFY(chan2->hasMutableContents());
    QVERIFY(chan2->handlerStreamingRequired());
}

void TestCallChannel::cleanup()
{
    mChan.reset();
    cleanupImpl();
}

void TestCallChannel::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestCallChannel)
#include "_gen/call-channel.cpp.moc.hpp"
