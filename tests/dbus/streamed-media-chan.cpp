#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/callable/conn.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/StreamedMediaChannel>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestStreamedMediaChan : public Test
{
    Q_OBJECT

public:
    TestStreamedMediaChan(QObject *parent = 0)
        : Test(parent), mConn(0)
    { }

protected Q_SLOTS:
    void expectRequestStreamsFinished(Tp::PendingOperation *);
    void expectBusyRequestStreamsFinished(Tp::PendingOperation *);

    // Special event handlers for the OutgoingCall state-machine
    void expectOutgoingRequestStreamsFinished(Tp::PendingOperation *);

    void onOutgoingGroupMembersChanged(
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Channel::GroupMemberChangeDetails &);

    void onGroupMembersChanged(
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Channel::GroupMemberChangeDetails &);

    void onStreamRemoved(const Tp::StreamedMediaStreamPtr &);
    void onStreamDirectionChanged(const Tp::StreamedMediaStreamPtr &,
            Tp::MediaStreamDirection,
            Tp::MediaStreamPendingSend);
    void onLSSChanged(Tp::StreamedMediaStream::SendingState);
    void onRSSChanged(Tp::StreamedMediaStream::SendingState);
    void onStreamStateChanged(const Tp::StreamedMediaStreamPtr &,
            Tp::MediaStreamState);
    void onChanInvalidated(Tp::DBusProxy *,
            const QString &, const QString &);

    // Special event handlers for the OutgoingCallTerminate state-machine
    void expectTerminateRequestStreamsFinished(Tp::PendingOperation *);

    void onTerminateGroupMembersChanged(
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Channel::GroupMemberChangeDetails &);

    void onTerminateChanInvalidated(Tp::DBusProxy *,
            const QString &, const QString &);

    void onNewChannels(const Tp::ChannelDetailsList &);
    void onLocalHoldStateChanged(Tp::LocalHoldState,
            Tp::LocalHoldStateReason);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testOutgoingCall();
    void testOutgoingCallBusy();
    void testOutgoingCallNoAnswer();
    void testOutgoingCallTerminate();
    void testIncomingCall();
    void testHold();
    void testHoldNoUnhold();
    void testHoldInabilityUnhold();
    void testDTMF();
    void testDTMFNoContinuousTone();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    StreamedMediaChannelPtr mChan;
    QList<ContactPtr> mContacts;
    StreamedMediaStreams mRequestStreamsReturn;
    Contacts mChangedCurrent;
    Contacts mChangedLP;
    Contacts mChangedRP;
    Contacts mChangedRemoved;
    Channel::GroupMemberChangeDetails mDetails;
    StreamedMediaStreamPtr mStreamRemovedReturn;
    StreamedMediaStreamPtr mSDCStreamReturn;
    Tp::MediaStreamDirection mSDCDirectionReturn;
    Tp::MediaStreamPendingSend mSDCPendingReturn;
    StreamedMediaStreamPtr mSSCStreamReturn;
    Tp::StreamedMediaStream::SendingState mChangedLSS;
    Tp::StreamedMediaStream::SendingState mChangedRSS;
    Tp::MediaStreamState mSSCStateReturn;
    QQueue<uint> mLocalHoldStates;
    QQueue<uint> mLocalHoldStateReasons;

    // state machine for the OutgoingCall test-case
    enum {
        OutgoingStateInitial,
        OutgoingStateRequested,
        OutgoingStateRinging,
        OutgoingStateDone
    } mOutgoingState;
    bool mOutgoingGotRequestStreamsFinished;
    bool mOutgoingAudioDone;

    // state machine for the OutgoingCallTerminate test-case
    enum {
        TerminateStateInitial,
        TerminateStateRequested,
        TerminateStateRinging,
        TerminateStateAnswered,
        TerminateStateTerminated
    } mTerminateState;
};

void TestStreamedMediaChan::expectRequestStreamsFinished(PendingOperation *op)
{
    mRequestStreamsReturn.clear();

    TEST_VERIFY_OP(op);

    qDebug() << "request streams finished successfully";

    PendingStreamedMediaStreams *pms = qobject_cast<PendingStreamedMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();
    mLoop->exit(0);
}

void TestStreamedMediaChan::expectBusyRequestStreamsFinished(PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        // The service signaled busy even before tp-qt finished introspection.
        // FIXME: should the error be something else, actually? Such as, perchance,
        // org.freedesktop.Telepathy.Error.Busy? (fd.o #29757).
        QCOMPARE(op->errorName(), QLatin1String("org.freedesktop.Telepathy.Error.Cancelled"));
        qDebug() << "request streams finished already busy";
        mLoop->exit(0);
        return;
    }

    qDebug() << "request streams finished successfully";

    PendingStreamedMediaStreams *pms = qobject_cast<PendingStreamedMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();
    mLoop->exit(0);
}

void TestStreamedMediaChan::expectOutgoingRequestStreamsFinished(PendingOperation *op)
{
    QVERIFY(op->isFinished());
    QVERIFY(!op->isError());
    QVERIFY(op->isValid());

    PendingStreamedMediaStreams *pms = qobject_cast<PendingStreamedMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();

    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    QCOMPARE(mContacts.size(), 1);
    StreamedMediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);

    // These checks can't work reliably, unless we add some complex backdoors to the test service,
    // to only start changing state / direction when we explicitly tell it so (not automatically
    // when we have requested the stream)
    // QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    // QCOMPARE(stream->direction(), Tp::MediaStreamDirectionBidirectional);

    QCOMPARE(mChan->streams().size(), 1);
    QVERIFY(mChan->streams().contains(stream));

    qDebug() << "stream requested successfully";

    // Only advance to Requested if the remote moving to RP hasn't already advanced it to Ringing or
    // even Answered - tbf it seems the StreamedMediaChannel semantics are quite hard for
    // application code to get right because the events can happen in whichever order. Should this
    // be considered a bug by itself? It'd probably be pretty hard to fix so I hope not :D
    if (mOutgoingState == OutgoingStateInitial) {
        mOutgoingState = OutgoingStateRequested;
    }

    if (mOutgoingState == OutgoingStateDone) {
        // finished later than the membersChanged() - exit mainloop now
        mLoop->exit(0);
    } else {
        // finished earlier than membersChanged() - it will exit
        mOutgoingGotRequestStreamsFinished = true;
    }
}

void TestStreamedMediaChan::onOutgoingGroupMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    // At this point, mContacts should still contain the contact we requested the
    // stream for
    ContactPtr otherContact = mContacts.first();

    if (mOutgoingState == OutgoingStateInitial || mOutgoingState == OutgoingStateRequested) {
        // The target should have become remote pending now
        QVERIFY(groupMembersAdded.isEmpty());
        QVERIFY(groupLocalPendingMembersAdded.isEmpty());
        QCOMPARE(groupRemotePendingMembersAdded.size(), 1);
        QVERIFY(groupMembersRemoved.isEmpty());

        QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
        QCOMPARE(mChan->awaitingRemoteAnswer(), true);

        qDebug() << "call now ringing";

        mOutgoingState = OutgoingStateRinging;
    } else if (mOutgoingState == OutgoingStateRinging) {
        QCOMPARE(groupMembersAdded.size(), 1);
        QVERIFY(groupLocalPendingMembersAdded.isEmpty());
        QVERIFY(groupRemotePendingMembersAdded.isEmpty());
        QVERIFY(groupMembersRemoved.isEmpty());

        QCOMPARE(mChan->groupContacts().size(), 2);
        QVERIFY(mChan->groupContacts().contains(otherContact));
        QCOMPARE(mChan->awaitingRemoteAnswer(), false);

        qDebug() << "call now answered";

        mOutgoingState = OutgoingStateDone;
        mOutgoingAudioDone = true;

        // Exit if we already got finished() from requestStreams() - otherwise the finish callback
        // will exit
        if (mOutgoingGotRequestStreamsFinished) {
            mLoop->exit(0);
        }
    }

    qDebug() << "group members changed";
    mChangedCurrent = groupMembersAdded;
    mChangedLP = groupLocalPendingMembersAdded;
    mChangedRP = groupRemotePendingMembersAdded;
    mChangedRemoved = groupMembersRemoved;
    mDetails = details;
}

void TestStreamedMediaChan::onGroupMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    qDebug() << "group members changed";
    mChangedCurrent = groupMembersAdded;
    mChangedLP = groupLocalPendingMembersAdded;
    mChangedRP = groupRemotePendingMembersAdded;
    mChangedRemoved = groupMembersRemoved;
    mDetails = details;
}

void TestStreamedMediaChan::onStreamRemoved(const StreamedMediaStreamPtr &stream)
{
    qDebug() << "stream" << stream.data() << "removed";
    mStreamRemovedReturn = stream;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamDirectionChanged(const StreamedMediaStreamPtr &stream,
        Tp::MediaStreamDirection direction,
        Tp::MediaStreamPendingSend pendingSend)
{
    qDebug() << "stream" << stream.data() << "direction changed to" << direction;
    mSDCStreamReturn = stream;
    mSDCDirectionReturn = direction;
    mSDCPendingReturn = pendingSend;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onLSSChanged(Tp::StreamedMediaStream::SendingState state)
{
    qDebug() << "onLSSChanged: " << static_cast<uint>(state);
    mChangedLSS = state;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onRSSChanged(Tp::StreamedMediaStream::SendingState state)
{
    qDebug() << "onRSSChanged: " << static_cast<uint>(state);
    mChangedRSS = state;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamStateChanged(const StreamedMediaStreamPtr &stream,
        Tp::MediaStreamState state)
{
    qDebug() << "stream" << stream.data() << "state changed to" << state;
    mSSCStreamReturn = stream;
    mSSCStateReturn = state;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onChanInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "chan invalidated:" << errorName << "-" << errorMessage;
    mLoop->exit(0);
}

void TestStreamedMediaChan::expectTerminateRequestStreamsFinished(PendingOperation *op)
{
    QVERIFY(op->isFinished());

    if (op->isError()) {
        // FIXME: should the error be something else, actually? Such as, perchance,
        // org.freedesktop.Telepathy.Error.Terminated? (fd.o #29757).
        QCOMPARE(op->errorName(), QLatin1String("org.freedesktop.Telepathy.Error.Cancelled"));
        qDebug() << "The remote hung up before we even got to take a look at the stream!";
        mTerminateState = TerminateStateTerminated;
        mLoop->exit(0);
        return;
    }

    QVERIFY(op->isValid());

    PendingStreamedMediaStreams *pms = qobject_cast<PendingStreamedMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();

    qDebug() << "stream requested successfully";

    // Only advance to Requested if the remote moving to RP hasn't already advanced it to Ringing or
    // even Answered - tbf it seems the StreamedMediaChannel semantics are quite hard for
    // application code to get right because the events can happen in whichever order. Should this
    // be considered a bug by itself? It'd probably be pretty hard to fix so I hope not :D
    if (mTerminateState == TerminateStateInitial) {
        mTerminateState = TerminateStateRequested;
    }
}

void TestStreamedMediaChan::onTerminateGroupMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    // At this point, mContacts should still contain the contact we requested the
    // stream for
    ContactPtr otherContact = mContacts.first();

    if (mTerminateState == TerminateStateInitial || mTerminateState == TerminateStateRequested) {
        // The target should have become remote pending now
        QVERIFY(groupMembersAdded.isEmpty());
        QVERIFY(groupLocalPendingMembersAdded.isEmpty());
        QCOMPARE(groupRemotePendingMembersAdded.size(), 1);
        QVERIFY(groupMembersRemoved.isEmpty());

        QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
        QCOMPARE(mChan->awaitingRemoteAnswer(), true);

        qDebug() << "call now ringing";

        mTerminateState = TerminateStateRinging;
    } else if (mTerminateState == TerminateStateRinging) {
        QCOMPARE(groupMembersAdded.size(), 1);
        QVERIFY(groupLocalPendingMembersAdded.isEmpty());
        QVERIFY(groupRemotePendingMembersAdded.isEmpty());
        QVERIFY(groupMembersRemoved.isEmpty());

        QCOMPARE(mChan->groupContacts().size(), 2);
        QVERIFY(mChan->groupContacts().contains(otherContact));
        QCOMPARE(mChan->awaitingRemoteAnswer(), false);

        qDebug() << "call now answered";

        mTerminateState = TerminateStateAnswered;
    } else if (mTerminateState == TerminateStateAnswered) {
        // It might be actually that currently this won't happen before invalidated() is emitted, so
        // we'll never reach this due to having exited the mainloop already - but it's entirely
        // valid for the library to signal either the invalidation or removing the members first, so
        // let's verify the member change in case it does that first.

        qDebug() << "membersChanged() after the call was answered - the remote probably hung up";

        QVERIFY(groupMembersAdded.isEmpty());
        QVERIFY(groupLocalPendingMembersAdded.isEmpty());
        QVERIFY(groupRemotePendingMembersAdded.isEmpty());
        QVERIFY(groupMembersRemoved.contains(otherContact) ||
                groupMembersRemoved.contains(mChan->groupSelfContact())); // can be either, or both

        // the invalidated handler will change state due to the fact that we might get 0-2 of these,
        // but always exactly one invalidated()
    }

    qDebug() << "group members changed";
    mChangedCurrent = groupMembersAdded;
    mChangedLP = groupLocalPendingMembersAdded;
    mChangedRP = groupRemotePendingMembersAdded;
    mChangedRemoved = groupMembersRemoved;
    mDetails = details;
}

void TestStreamedMediaChan::onTerminateChanInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "chan invalidated:" << errorName << "-" << errorMessage;
    mTerminateState = TerminateStateTerminated;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    qDebug() << "new channels";
    Q_FOREACH (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        bool requested = details.properties.value(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !requested) {
            mChan = StreamedMediaChannel::create(mConn->client(),
                    details.channel.path(), details.properties);
            mLoop->exit(0);
        }
    }
}

void TestStreamedMediaChan::onLocalHoldStateChanged(Tp::LocalHoldState localHoldState,
        Tp::LocalHoldStateReason localHoldStateReason)
{
    qDebug() << "local hold state changed:" << localHoldState << localHoldStateReason;
    mLocalHoldStates.append(localHoldState);
    mLocalHoldStateReasons.append(localHoldStateReason);
    mLoop->exit(0);
}

void TestStreamedMediaChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("streamed-media-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_CALLABLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            "simulation-delay", 1,
            NULL);
    QCOMPARE(mConn->connect(Connection::FeatureSelfContact), true);
}

void TestStreamedMediaChan::init()
{
    initImpl();

    mContacts.clear();
    mRequestStreamsReturn.clear();
    mChangedCurrent.clear();
    mChangedLP.clear();
    mChangedRP.clear();
    mChangedRemoved.clear();
    mStreamRemovedReturn.reset();
    mSDCStreamReturn.reset();
    mSDCDirectionReturn = (Tp::MediaStreamDirection) -1;
    mSDCPendingReturn = (Tp::MediaStreamPendingSend) -1;
    mSSCStateReturn = (Tp::MediaStreamState) -1;
    mChangedLSS = (Tp::StreamedMediaStream::SendingState) -1;
    mChangedRSS = (Tp::StreamedMediaStream::SendingState) -1;
    mSSCStreamReturn.reset();
    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();
}

void TestStreamedMediaChan::testOutgoingCall()
{
    mContacts = mConn->contacts(QStringList() << QLatin1String("alice"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 0);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    // RequestStreams with bad type must fail
    QVERIFY(connect(mChan->requestStream(otherContact, (Tp::MediaStreamType) -1),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QCOMPARE(mRequestStreamsReturn.size(), 0);

    // Request audio stream, and wait for:
    //  - the request to finish
    //  - the contact to appear on RP
    //  - the contact to accept the call
    mOutgoingState = OutgoingStateInitial;
    mOutgoingAudioDone = false;
    mOutgoingGotRequestStreamsFinished = false;

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onOutgoingGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    qDebug() << "requesting audio stream";

    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectOutgoingRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(static_cast<int>(mOutgoingState), static_cast<int>(OutgoingStateDone));
    QCOMPARE(mOutgoingAudioDone, true);

    qDebug() << "requesting video stream";

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    StreamedMediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeVideo);

    // These checks can't work reliably, unless we add some complex backdoors to the test service,
    // to only start changing state / direction when we explicitly tell it so (not automatically
    // when we have requested the stream)
    // QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    // QCOMPARE(stream->direction(), Tp::MediaStreamDirectionBidirectional);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    qDebug() << "removing audio stream";

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Tp::StreamedMediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Tp::StreamedMediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStream(stream),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (mChan->streams().size() == 2) {
        qDebug() << "re-entering mainloop to wait for stream removal being signaled";
        // wait stream removed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mStreamRemovedReturn, stream);
    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 0);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream direction/state changed
    stream = mChan->streamsForType(Tp::MediaStreamTypeVideo).first();
    QVERIFY(stream);

    qDebug() << "changing stream direction, currently" << stream->direction();
    qDebug() << "state currently" << stream->state();

    if (stream->state() != Tp::MediaStreamStateConnected) {
        QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Tp::StreamedMediaStreamPtr &,
                            Tp::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Tp::StreamedMediaStreamPtr &,
                            Tp::MediaStreamState))));
    } else {
        // Pretend that we saw the SSC to Connected (although it might have happened even before the
        // stream request finished, in which case we have no change of catching it, because we don't
        // have the stream yet)
        mSSCStreamReturn = stream;
        mSSCStateReturn = Tp::MediaStreamStateConnected;
    }

    QCOMPARE(stream->localSendingRequested(), false);
    QCOMPARE(stream->remoteSendingRequested(), false);
    QCOMPARE(stream->sending(), true);
    QCOMPARE(stream->receiving(), true);

    /* request only receiving now */
    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Tp::StreamedMediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Tp::StreamedMediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend))));
    QVERIFY(connect(stream.data(),
                    SIGNAL(localSendingStateChanged(Tp::StreamedMediaStream::SendingState)),
                    SLOT(onLSSChanged(Tp::StreamedMediaStream::SendingState))));
    QVERIFY(connect(stream.data(),
                    SIGNAL(remoteSendingStateChanged(Tp::StreamedMediaStream::SendingState)),
                    SLOT(onRSSChanged(Tp::StreamedMediaStream::SendingState))));
    QVERIFY(connect(stream->requestDirection(Tp::MediaStreamDirectionReceive),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (!mSDCStreamReturn || !mSSCStreamReturn || (static_cast<int>(mChangedLSS) == -1)) {
        qDebug() << "re-entering mainloop to wait for stream direction change and state change";
        // wait direction and state changed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    // If this fails, we also got a remote state change signal, although we shouldn't have
    QCOMPARE(static_cast<int>(mChangedRSS), -1);
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Tp::MediaStreamDirectionReceive);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionReceive);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Tp::MediaStreamStateConnected);

    QCOMPARE(stream->sending(), false);
    QCOMPARE(stream->receiving(), true);
}

void TestStreamedMediaChan::testOutgoingCallBusy()
{
    // This identifier contains the magic string (busy), which means the example
    // will simulate rejection of the call as busy rather than accepting it.
    mContacts = mConn->contacts(QStringList() << QLatin1String("alice (busy)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 0);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectBusyRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    if (mChan->isValid()) {
        qDebug() << "waiting for the channel to become invalidated";

        QVERIFY(connect(mChan.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                            const QString &, const QString &)),
                    SLOT(onChanInvalidated(Tp::DBusProxy *,
                            const QString &, const QString &))));
        QCOMPARE(mLoop->exec(), 0);
    } else {
        qDebug() << "not waiting for the channel to become invalidated, it has been invalidated" <<
            "already";
    }

    QCOMPARE(mChan->groupContacts().size(), 0);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->streams().size(), 0);
}

void TestStreamedMediaChan::testOutgoingCallNoAnswer()
{
    // This identifier contains the magic string (no answer), which means the example
    // will never answer.
    mContacts = mConn->contacts(QStringList() << QLatin1String("alice (no answer)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 0);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    /* After the initial flurry of D-Bus messages, alice still hasn't answered */
    processDBusQueue(mConn->client().data());

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));
    // wait the contact to appear on RP
    while (mChan->groupRemotePendingContacts().size() == 0) {
        mLoop->processEvents();
    }
    QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
    QCOMPARE(mChan->awaitingRemoteAnswer(), true);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 1);

    /* assume we're never going to get an answer, and hang up */
    mChan->requestClose();

    QVERIFY(connect(mChan.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SLOT(onChanInvalidated(Tp::DBusProxy *,
                                           const QString &, const QString &))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->groupContacts().size(), 0);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->streams().size(), 0);
}

void TestStreamedMediaChan::testOutgoingCallTerminate()
{
    // This identifier contains the magic string (terminate), which means the example
    // will simulate answering the call but then terminating it.
    mContacts = mConn->contacts(QStringList() << QLatin1String("alice (terminate)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 0);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    // Request audio stream, and verify that following doing so, we get events for:
    //  [0-2].5) the stream request finishing (sadly this can happen before, or between, any of the
    //           following) - is this a bug?
    //  1) the remote appearing on the RP contacts -> should be awaitingRemoteAnswer()
    //  2) the remote answering the call -> should not be awaitingRemoteAnswer(), should have us and
    //     them as the current members
    //  3) the channel being invalidated (due to the remote having terminated the call) -> exits the
    //     mainloop
    //
    //  Previously, this test used to spin the mainloop until each of the events had seemingly
    //  happened, only checking for the events between the iterations. This is race-prone however,
    //  as multiple events can happen in one mainloop iteration if the test executes slowly compared
    //  with the simulated network events from the service (eg. in valgrind or under high system
    //  load).

    mTerminateState = TerminateStateInitial;

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onTerminateGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SLOT(onTerminateChanInvalidated(Tp::DBusProxy *,
                                           const QString &, const QString &))));

    qDebug() << "calling, hope somebody answers and doesn't immediately hang up!";

    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectTerminateRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(static_cast<int>(mTerminateState), static_cast<int>(TerminateStateTerminated));

    qDebug() << "oh crap, nobody wants to talk to me";
}


void TestStreamedMediaChan::testIncomingCall()
{
    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("away"), QLatin1String("preparing for a test"));

    Client::ConnectionInterfaceRequestsInterface *connRequestsInterface =
        mConn->client()->optionalInterface<Client::ConnectionInterfaceRequestsInterface>();

    QVERIFY(connect(connRequestsInterface,
                    SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Tp::ChannelDetailsList&))));
    mConn->client()->lowlevel()->setSelfPresence(QLatin1String("available"), QLatin1String("call me?"));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan);
    QCOMPARE(mChan->streams().size(), 0);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 1);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), true);
    QCOMPARE(mChan->awaitingRemoteAnswer(), false);
    QVERIFY(mChan->groupLocalPendingContacts().contains(mConn->client()->selfContact()));

    ContactPtr otherContact = *mChan->groupContacts().begin();
    QCOMPARE(otherContact, mChan->initiatorContact());

    QVERIFY(connect(mChan->acceptCall(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->client()->selfContact()));

    QCOMPARE(mChan->streams().size(), 1);
    StreamedMediaStreamPtr stream = mChan->streams().first();
    QCOMPARE(stream->channel(), mChan);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);

    qDebug() << "requesting a stream with a bad type";

    // RequestStreams with bad type must fail
    QVERIFY(connect(mChan->requestStream(otherContact, (Tp::MediaStreamType) -1),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QCOMPARE(mRequestStreamsReturn.size(), 0);

    qDebug() << "requesting a video stream";

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeVideo);

    // These checks can't work reliably, unless we add some complex backdoors to the test service,
    // to only start changing state / direction when we explicitly tell it so (not automatically
    // when we have requested the stream)
    // QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    // QCOMPARE(stream->direction(), Tp::MediaStreamDirectionBidirectional);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    qDebug() << "removing the audio stream";

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Tp::StreamedMediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Tp::StreamedMediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStreams(StreamedMediaStreams() << stream),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (mChan->streams().size() == 2) {
        // wait stream removed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mStreamRemovedReturn, stream);
    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 0);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream direction/state changed
    stream = mChan->streamsForType(Tp::MediaStreamTypeVideo).first();
    QVERIFY(stream);

    qDebug() << "requesting direction (false, true) - currently" << stream->direction();
    qDebug() << "current stream state" << stream->state();

    if (stream->state() != Tp::MediaStreamStateConnected) {
        QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Tp::StreamedMediaStreamPtr &,
                            Tp::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Tp::StreamedMediaStreamPtr &,
                            Tp::MediaStreamState))));
    } else {
        // Pretend that we saw the SSC to Connected (although it might have happened even before the
        // stream request finished, in which case we have no change of catching it, because we don't
        // have the stream yet)
        mSSCStreamReturn = stream;
        mSSCStateReturn = Tp::MediaStreamStateConnected;
    }

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Tp::StreamedMediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Tp::StreamedMediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend))));
    QVERIFY(connect(stream.data(),
                    SIGNAL(localSendingStateChanged(Tp::StreamedMediaStream::SendingState)),
                    SLOT(onLSSChanged(Tp::StreamedMediaStream::SendingState))));
    QVERIFY(connect(stream.data(),
                    SIGNAL(remoteSendingStateChanged(Tp::StreamedMediaStream::SendingState)),
                    SLOT(onRSSChanged(Tp::StreamedMediaStream::SendingState))));
    QVERIFY(connect(stream->requestDirection(false, true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (!mSDCStreamReturn || !mSSCStreamReturn || (static_cast<int>(mChangedLSS) == -1)) {
        // wait direction and state changed signal
        qDebug() << "re-entering mainloop to wait for stream direction change and state change";
        QCOMPARE(mLoop->exec(), 0);
    }
    // If this fails, we also got a remote state change signal, although we shouldn't have
    QCOMPARE(static_cast<int>(mChangedRSS), -1);
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Tp::MediaStreamDirectionReceive);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionReceive);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Tp::MediaStreamStateConnected);
}

void TestStreamedMediaChan::testHold()
{
    mContacts = mConn->contacts(QStringList() << QLatin1String("bob"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureLocalHoldState),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureLocalHoldState));

    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonNone));

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
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(LocalHoldStatePendingHold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));

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
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(LocalHoldStatePendingUnhold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(LocalHoldStateUnheld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));
}

void TestStreamedMediaChan::testHoldNoUnhold()
{
    mContacts = mConn->contacts(QStringList() << QLatin1String("bob (no unhold)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureLocalHoldState),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureLocalHoldState));

    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonNone));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(localHoldStateChanged(Tp::LocalHoldState, Tp::LocalHoldStateReason)),
                    SLOT(onLocalHoldStateChanged(Tp::LocalHoldState, Tp::LocalHoldStateReason))));
    // Request hold
    QPointer<PendingOperation> holdOp = mChan->requestHold(true);
    while (mLocalHoldStates.size() != 2 || (holdOp && !holdOp.data()->isFinished())) {
        mLoop->processEvents();
    }
    QCOMPARE(!holdOp || holdOp.data()->isValid(), true);
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(LocalHoldStatePendingHold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));

    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();

    qDebug() << "requesting failing unhold";

    // Request unhold (fail)
    QVERIFY(connect(mChan->requestHold(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);
    QCOMPARE(mLocalHoldStates.size(), 0);
    QCOMPARE(mLocalHoldStateReasons.size(), 0);
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));
}

void TestStreamedMediaChan::testHoldInabilityUnhold()
{
    mContacts = mConn->contacts(
            QStringList() << QLatin1String("bob (inability to unhold)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureLocalHoldState),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureLocalHoldState));

    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateUnheld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonNone));

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
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(LocalHoldStatePendingHold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));

    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();

    // Request unhold (fail - back to hold)
    QVERIFY(connect(mChan->requestHold(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    while (mLocalHoldStates.size() != 3) {
        QCOMPARE(mLoop->exec(), 0);
    }

    LocalHoldState state = static_cast<LocalHoldState>(mLocalHoldStates.dequeue());
    LocalHoldStateReason stateReason = static_cast<LocalHoldStateReason>(mLocalHoldStateReasons.dequeue());
    QCOMPARE(state, LocalHoldStatePendingUnhold);
    QCOMPARE(stateReason, LocalHoldStateReasonRequested);

    state = static_cast<LocalHoldState>(mLocalHoldStates.dequeue());
    stateReason = static_cast<LocalHoldStateReason>(mLocalHoldStateReasons.dequeue());
    QCOMPARE(state, LocalHoldStatePendingHold);
    QCOMPARE(stateReason, LocalHoldStateReasonRequested);

    state = static_cast<LocalHoldState>(mLocalHoldStates.dequeue());
    stateReason = static_cast<LocalHoldStateReason>(mLocalHoldStateReasons.dequeue());
    QCOMPARE(state, LocalHoldStateHeld);
    QCOMPARE(stateReason, LocalHoldStateReasonRequested);

    QCOMPARE(mChan->localHoldState(), LocalHoldStateHeld);
    QCOMPARE(mChan->localHoldStateReason(), LocalHoldStateReasonRequested);
}

void TestStreamedMediaChan::testDTMF()
{
    mContacts = mConn->contacts(QStringList() << QLatin1String("john"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    StreamedMediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);

    QCOMPARE(mChan->streams().size(), 1);
    QVERIFY(mChan->streams().contains(stream));

    // start dtmf
    QVERIFY(connect(stream->startDTMFTone(DTMFEventDigit0),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // stop dtmf
    QVERIFY(connect(stream->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // stop dtmf again (should succeed)
    QVERIFY(connect(stream->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeVideo);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    // start dtmf (must fail)
    QVERIFY(connect(stream->startDTMFTone(DTMFEventDigit0),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);

    // stop dtmf (must fail)
    QVERIFY(connect(stream->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);
}

void TestStreamedMediaChan::testDTMFNoContinuousTone()
{
    mContacts = mConn->contacts(
            QStringList() << QLatin1String("john (no continuous tone)"));
    QCOMPARE(mContacts.size(), 1);
    ContactPtr otherContact = mContacts.first();
    QVERIFY(otherContact);

    mChan = StreamedMediaChannelPtr::qObjectCast(
            mConn->createChannel(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA, otherContact));
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    StreamedMediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);

    QCOMPARE(mChan->streams().size(), 1);
    QVERIFY(mChan->streams().contains(stream));

    // start dtmf
    QVERIFY(connect(stream->startDTMFTone(DTMFEventDigit0),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // stop dtmf (must fail)
    QVERIFY(connect(stream->stopDTMFTone(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 1);
}

void TestStreamedMediaChan::cleanup()
{
    mChan.reset();
    mContacts.clear();
    mRequestStreamsReturn.clear();
    mStreamRemovedReturn.reset();
    mChangedCurrent.clear();
    mChangedLP.clear();
    mChangedRP.clear();
    mChangedRemoved.clear();
    mDetails = Channel::GroupMemberChangeDetails();
    mSDCStreamReturn.reset();
    mSSCStreamReturn.reset();
    cleanupImpl();
}

void TestStreamedMediaChan::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamedMediaChan)
#include "_gen/streamed-media-chan.cpp.moc.hpp"
