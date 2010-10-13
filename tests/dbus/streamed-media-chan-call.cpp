#include <QtCore/QDebug>
#include <QtTest/QtTest>

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/future/call/conn.h>
#include <tests/lib/test.h>

#define TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT"

using namespace Tp;

class TestStreamedMediaChanCall : public Test
{
    Q_OBJECT

public:
    TestStreamedMediaChanCall(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectRequestContactsFinished(Tp::PendingOperation *);
    void expectCreateChannelFinished(Tp::PendingOperation *);
    void expectRequestContentFinished(Tp::PendingOperation *);
    void expectRequestStreamsFinished(Tp::PendingOperation *);
    void expectSuccessfulRequestReceiving(Tp::PendingOperation *);
    void onContentRemoved(const Tp::MediaContentPtr &content);
    void onLocalSendingStateChanged(Tp::MediaStream::SendingState);
    void onRemoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::MediaStream::SendingState> &);
    void onStreamDirectionChanged(const Tp::MediaStreamPtr &,
            Tp::MediaStreamDirection,
            Tp::MediaStreamPendingSend);
    void onStreamStateChanged(const Tp::MediaStreamPtr &,
            Tp::MediaStreamState);
    void onChanInvalidated(Tp::DBusProxy *,
            const QString &, const QString &);
    void onNewChannels(const Tp::ChannelDetailsList &);
    void onLocalHoldStateChanged(Tp::LocalHoldState,
            Tp::LocalHoldStateReason);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testOutgoingCall();
    void testIncomingCall();
    void testHold();
    void testHoldNoUnhold();
    void testHoldInabilityUnhold();

    void cleanup();
    void cleanupTestCase();

private:
    ExampleCallConnection *mConnService;

    ConnectionPtr mConn;
    QString mConnName;
    QString mConnPath;
    StreamedMediaChannelPtr mChan;
    QList<ContactPtr> mRequestContactsReturn;
    MediaContentPtr mRequestContentReturn;
    MediaContentPtr mContentRemoved;
    MediaStream::SendingState mLSSCReturn;
    QQueue<uint> mLocalHoldStates;
    QQueue<uint> mLocalHoldStateReasons;

    MediaStreams mRequestStreamsReturn;
    MediaStreamPtr mSDCStreamReturn;
    Tp::MediaStreamDirection mSDCDirectionReturn;
    Tp::MediaStreamPendingSend mSDCPendingReturn;
    MediaStreamPtr mSSCStreamReturn;
    Tp::MediaStreamState mSSCStateReturn;

    // Remote sending state changed state-machine
    enum {
        RSSCStateInitial,
        RSSCStatePendingSend,
        RSSCStateSending,
        RSSCStateDone
    } mRSSCState;
    int mSuccessfulRequestReceivings;
};

void TestStreamedMediaChanCall::expectRequestContactsFinished(PendingOperation *op)
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

    PendingContacts *pc = qobject_cast<PendingContacts*>(op);
    mRequestContactsReturn = pc->contacts();
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::expectCreateChannelFinished(PendingOperation* op)
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

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = StreamedMediaChannelPtr::qObjectCast(pc->channel());
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::expectRequestContentFinished(PendingOperation *op)
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

    PendingMediaContent *pmc = qobject_cast<PendingMediaContent*>(op);
    mRequestContentReturn = pmc->content();
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::expectRequestStreamsFinished(PendingOperation *op)
{
    mRequestStreamsReturn.clear();

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

    qDebug() << "request streams finished successfully";

    PendingMediaStreams *pms = qobject_cast<PendingMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::onLocalSendingStateChanged(
        const MediaStream::SendingState state)
{
    qDebug() << "local sending state changed";
    mLSSCReturn = state;
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::expectSuccessfulRequestReceiving(PendingOperation *op)
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

void TestStreamedMediaChanCall::onContentRemoved(const Tp::MediaContentPtr &content)
{
    mContentRemoved = content;
    mLoop->exec(0);
}

void TestStreamedMediaChanCall::onRemoteSendingStateChanged(
        const QHash<Tp::ContactPtr, Tp::MediaStream::SendingState> &states)
{
    // There should be no further events
    QVERIFY(mRSSCState != RSSCStateDone);

    QCOMPARE(states.size(), 1);
    ContactPtr otherContact = states.keys().first();

    MediaContentPtr content = mChan->contentsForType(Tp::MediaStreamTypeVideo).first();
    QVERIFY(content);

    MediaStreamPtr stream = content->streams().first();
    QVERIFY(stream);

    if (mRSSCState == RSSCStateInitial) {
        QCOMPARE(states[otherContact], MediaStream::SendingStatePendingSend);
        mRSSCState = RSSCStatePendingSend;
    } else if (mRSSCState == RSSCStatePendingSend) {
        QCOMPARE(states[otherContact], MediaStream::SendingStateSending);
        mRSSCState = RSSCStateSending;

        QVERIFY(connect(stream->requestReceiving(otherContact, false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulRequestReceiving(Tp::PendingOperation*))));
    } else if (mRSSCState == RSSCStateSending) {
        QCOMPARE(states[otherContact], MediaStream::SendingStateNone);
        mRSSCState = RSSCStateDone;

        if (mSuccessfulRequestReceivings == 2) {
            mLoop->exit(0);
        }
    }

    qDebug() << "remote sending state changed to" << states[otherContact];
}

void TestStreamedMediaChanCall::onStreamDirectionChanged(const MediaStreamPtr &stream,
        Tp::MediaStreamDirection direction,
        Tp::MediaStreamPendingSend pendingSend)
{
    qDebug() << "stream" << stream.data() << "direction changed to" << direction;
    mSDCStreamReturn = stream;
    mSDCDirectionReturn = direction;
    mSDCPendingReturn = pendingSend;
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::onStreamStateChanged(const MediaStreamPtr &stream,
        Tp::MediaStreamState state)
{
    qDebug() << "stream" << stream.data() << "state changed to" << state;
    mSSCStreamReturn = stream;
    mSSCStateReturn = state;
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::onChanInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "chan invalidated:" << errorName << "-" << errorMessage;
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    qDebug() << "new channels";
    Q_FOREACH (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL) &&
            !requested) {
            mChan = StreamedMediaChannel::create(mConn,
                        details.channel.path(),
                        details.properties);
            mLoop->exit(0);
        }
    }
}

void TestStreamedMediaChanCall::onLocalHoldStateChanged(Tp::LocalHoldState localHoldState,
        Tp::LocalHoldStateReason localHoldStateReason)
{
    mLocalHoldStates.append(localHoldState);
    mLocalHoldStateReasons.append(localHoldStateReason);
    mLoop->exit(0);
}

void TestStreamedMediaChanCall::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("text-streamed-media-call");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CALL_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CALL_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            "simulation-delay", 1,
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(Connection::FeatureSelfContact),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));
}

void TestStreamedMediaChanCall::init()
{
    initImpl();

    mRequestContactsReturn.clear();
    mRequestContentReturn.reset();
    mLSSCReturn = (MediaStream::SendingState) -1;
    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();
    mContentRemoved.reset();
}

void TestStreamedMediaChanCall::testOutgoingCall()
{
    qDebug() << "requesting contact for alice";

    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << QLatin1String("alice")),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    qDebug() << "creating the channel";

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL ".InitialAudio"),
                   true);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan);

    qDebug() << "making the channel ready";

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureContents),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureContents));

    QCOMPARE(mChan->contents().size(), 1);
    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QCOMPARE(mChan->awaitingRemoteAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));
    QVERIFY(mChan->groupContacts().contains(otherContact));

    qDebug() << "calling requestContent with a bad type";
    // RequestContent with bad type must fail
    QVERIFY(connect(mChan->requestContent(QLatin1String("content1"), (Tp::MediaStreamType) -1),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QVERIFY(!mRequestContentReturn);

    qDebug() << "calling requestContent with Audio";
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 1);

    mRequestContentReturn.reset();
    QVERIFY(connect(mChan->requestContent(QLatin1String("content1"), Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(!mRequestContentReturn.isNull());
    QCOMPARE(mRequestContentReturn->name(), QString(QLatin1String("content1")));
    QCOMPARE(mRequestContentReturn->type(), Tp::MediaStreamTypeAudio);

    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 2);

    qDebug() << "calling requestContent with Video";
    mRequestContentReturn.reset();
    QVERIFY(connect(mChan->requestContent(QLatin1String("content2"), Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContentFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(!mRequestContentReturn.isNull());
    QCOMPARE(mRequestContentReturn->name(), QString(QLatin1String("content2")));
    QCOMPARE(mRequestContentReturn->type(), Tp::MediaStreamTypeVideo);

    // test content removal
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 2);

    MediaContentPtr content;
    content = mChan->contentsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(content);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(contentRemoved(const Tp::MediaContentPtr &)),
                    SLOT(onContentRemoved(const Tp::MediaContentPtr &))));
    QVERIFY(connect(mChan->removeContent(content),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (mContentRemoved.isNull()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mChan->contentsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mContentRemoved, content);

    // test content sending changed
    content = mChan->contentsForType(Tp::MediaStreamTypeVideo).first();
    MediaStreamPtr stream = content->streams().first();
    QVERIFY(content);

    qDebug() << "stopping sending";

    QCOMPARE(static_cast<int>(stream->pendingSend()), 0);
    QCOMPARE(stream->localSendingRequested(), false);
    QCOMPARE(stream->remoteSendingRequested(), false);
    QCOMPARE(stream->sending(), true);
    QCOMPARE(stream->receiving(), true);
    QCOMPARE(stream->contact(), otherContact);
    QVERIFY(stream->members().contains(otherContact));
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionReceive);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionSend);

    QVERIFY(connect(stream.data(),
                    SIGNAL(localSendingStateChanged(Tp::MediaStream::SendingState)),
                    SLOT(onLocalSendingStateChanged(Tp::MediaStream::SendingState))));
    QVERIFY(connect(stream->requestSending(false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(stream->sending(), false);
    QCOMPARE(stream->receiving(), true);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionReceive);
    QVERIFY(!(stream->direction() & Tp::MediaStreamDirectionSend));

    qDebug() << "stopping receiving";

    QVERIFY(connect(stream->requestReceiving(otherContact, false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "waiting until we're not sending and not receiving";

    while (stream->localSendingState() != MediaStream::SendingStateNone
            || stream->remoteSendingState(otherContact) != MediaStream::SendingStateNone) {
        qDebug() << "re-entering mainloop to wait for local and remote SSC -> None";
        // wait local and remote sending state change
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLSSCReturn, MediaStream::SendingStateNone);
    QCOMPARE(stream->sending(), false);
    QCOMPARE(stream->receiving(), false);
    QVERIFY(!(stream->direction() & Tp::MediaStreamDirectionReceive));
    QVERIFY(!(stream->direction() & Tp::MediaStreamDirectionSend));

    qDebug() << "re-enabling sending";

    mLSSCReturn = (MediaStream::SendingState) -1;

    QVERIFY(connect(stream->requestDirection(true, false),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (mLSSCReturn == (MediaStream::SendingState) -1) {
        qDebug() << "re-entering mainloop to wait for SSC -> Sending";
        // wait sending state change
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mLSSCReturn, MediaStream::SendingStateSending);

    qDebug() << "flushing D-Bus events";
    processDBusQueue(mChan.data());

    qDebug() << "enabling receiving";

    mRSSCState = RSSCStateInitial;
    mSuccessfulRequestReceivings = 0;

    // test content receiving changed
    QVERIFY(connect(stream.data(),
                    SIGNAL(remoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::MediaStream::SendingState> &)),
                    SLOT(onRemoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::MediaStream::SendingState> &))));
    QVERIFY(connect(stream->requestDirection(true, true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulRequestReceiving(Tp::PendingOperation*))));
    while (mSuccessfulRequestReceivings != 2 || mRSSCState != RSSCStateDone) {
        mLoop->processEvents();
    }

    QCOMPARE(static_cast<uint>(mRSSCState), static_cast<uint>(RSSCStateDone));
}

void TestStreamedMediaChanCall::testIncomingCall()
{
    mConn->setSelfPresence(QLatin1String("away"), QLatin1String("preparing for a test"));
    QVERIFY(connect(mConn->requestsInterface(),
                    SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Tp::ChannelDetailsList&))));
    mConn->setSelfPresence(QLatin1String("available"), QLatin1String("call me?"));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan);
    QCOMPARE(mChan->streams().size(), 0);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    ContactPtr otherContact = *mChan->groupContacts().begin();

    QVERIFY(connect(mChan->acceptCall(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    QCOMPARE(mChan->streams().size(), 1);
    MediaStreamPtr stream = mChan->streams().first();
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

    // call does not have the concept of removing streams, it will remove the content the stream
    // belongs
    QVERIFY(connect(mChan.data(),
                    SIGNAL(contentRemoved(const Tp::MediaContentPtr &)),
                    SLOT(onContentRemoved(const Tp::MediaContentPtr &))));
    QVERIFY(connect(mChan->removeStreams(MediaStreams() << stream),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (mContentRemoved.isNull()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mContentRemoved, stream->content());
}

void TestStreamedMediaChanCall::testHold()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << QLatin1String("bob")),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL ".InitialAudio"),
                   true);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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

void TestStreamedMediaChanCall::testHoldNoUnhold()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << QLatin1String("bob (no unhold)")),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL ".InitialAudio"),
                   true);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    while (mLocalHoldStates.size() != 2 || (holdOp && !holdOp->isFinished())) {
        mLoop->processEvents();
    }
    QCOMPARE(!holdOp || holdOp->isValid(), true);
    QCOMPARE(mLocalHoldStates.first(), static_cast<uint>(LocalHoldStatePendingHold));
    QCOMPARE(mLocalHoldStateReasons.first(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(mLocalHoldStates.last(), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(mLocalHoldStateReasons.last(), static_cast<uint>(LocalHoldStateReasonRequested));
    QCOMPARE(static_cast<uint>(mChan->localHoldState()), static_cast<uint>(LocalHoldStateHeld));
    QCOMPARE(static_cast<uint>(mChan->localHoldStateReason()), static_cast<uint>(LocalHoldStateReasonRequested));

    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();

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

void TestStreamedMediaChanCall::testHoldInabilityUnhold()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << QLatin1String("bob (inability to unhold)")),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    request.insert(QLatin1String(TP_FUTURE_INTERFACE_CHANNEL_TYPE_CALL ".InitialAudio"),
                   true);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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

void TestStreamedMediaChanCall::cleanup()
{
    mChan.reset();
    cleanupImpl();
}

void TestStreamedMediaChanCall::cleanupTestCase()
{
    if (mConn) {
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamedMediaChanCall)
#include "_gen/streamed-media-chan-call.cpp.moc.hpp"
