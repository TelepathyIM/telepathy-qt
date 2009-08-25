#include <QtCore/QDebug>
#include <QtTest/QtTest>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/callable/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestStreamedMediaChan : public Test
{
    Q_OBJECT

public:
    TestStreamedMediaChan(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectRequestContactsFinished(Tp::PendingOperation *);
    void expectCreateChannelFinished(Tp::PendingOperation *);
    void expectRequestStreamsFinished(Tp::PendingOperation *);
    void onGroupMembersChanged(
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Contacts &,
            const Tp::Channel::GroupMemberChangeDetails &);
    void onStreamRemoved(const Tp::MediaStreamPtr &);
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
    ExampleCallableConnection *mConnService;

    ConnectionPtr mConn;
    QString mConnName;
    QString mConnPath;
    StreamedMediaChannelPtr mChan;
    QList<ContactPtr> mRequestContactsReturn;
    MediaStreams mRequestStreamsReturn;
    Contacts mChangedCurrent;
    Contacts mChangedLP;
    Contacts mChangedRP;
    Contacts mChangedRemoved;
    Channel::GroupMemberChangeDetails mDetails;
    MediaStreamPtr mStreamRemovedReturn;
    MediaStreamPtr mSDCStreamReturn;
    Tp::MediaStreamDirection mSDCDirectionReturn;
    Tp::MediaStreamPendingSend mSDCPendingReturn;
    MediaStreamPtr mSSCStreamReturn;
    Tp::MediaStreamState mSSCStateReturn;
    QQueue<uint> mLocalHoldStates;
    QQueue<uint> mLocalHoldStateReasons;
};

void TestStreamedMediaChan::expectRequestContactsFinished(PendingOperation *op)
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

void TestStreamedMediaChan::expectCreateChannelFinished(PendingOperation* op)
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
    mChan = StreamedMediaChannelPtr::dynamicCast(pc->channel());
    mLoop->exit(0);
}

void TestStreamedMediaChan::expectRequestStreamsFinished(PendingOperation *op)
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

    PendingMediaStreams *pms = qobject_cast<PendingMediaStreams*>(op);
    mRequestStreamsReturn = pms->streams();
    mLoop->exit(0);
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

void TestStreamedMediaChan::onStreamRemoved(const MediaStreamPtr &stream)
{
    qDebug() << "stream" << stream.data() << "removed";
    mStreamRemovedReturn = stream;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamDirectionChanged(const MediaStreamPtr &stream,
        Tp::MediaStreamDirection direction,
        Tp::MediaStreamPendingSend pendingSend)
{
    qDebug() << "stream" << stream.data() << "direction changed to" << direction;
    mSDCStreamReturn = stream;
    mSDCDirectionReturn = direction;
    mSDCPendingReturn = pendingSend;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamStateChanged(const MediaStreamPtr &stream,
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

void TestStreamedMediaChan::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    qDebug() << "new channels";
    foreach (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !requested) {
            mChan = StreamedMediaChannel::create(mConn,
                        details.channel.path(),
                        details.properties);
            mLoop->exit(0);
        }
    }
}

void TestStreamedMediaChan::onLocalHoldStateChanged(Tp::LocalHoldState localHoldState,
        Tp::LocalHoldStateReason localHoldStateReason)
{
    mLocalHoldStates.append(localHoldState);
    mLocalHoldStateReasons.append(localHoldStateReason);
    mLoop->exit(0);
}

void TestStreamedMediaChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("text-streamed-media");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CALLABLE_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CALLABLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QString::fromAscii(name);
    mConnPath = QString::fromAscii(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(Connection::FeatureSelfContact),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));
}

void TestStreamedMediaChan::init()
{
    initImpl();

    mRequestContactsReturn.clear();
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
    mSSCStreamReturn.reset();
    mLocalHoldStates.clear();
    mLocalHoldStateReasons.clear();
}

void TestStreamedMediaChan::testOutgoingCall()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "alice"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

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

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    MediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);
    QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Tp::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Tp::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 1);
    QVERIFY(mChan->streams().contains(stream));

    // wait the contact to appear on RP
    while (mChan->groupRemotePendingContacts().size() == 0) {
        mLoop->processEvents();
    }
    QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
    QCOMPARE(mChan->awaitingRemoteAnswer(), true);

    // wait the contact to accept the call
    while (mChan->groupContacts().size() != 2) {
        mLoop->processEvents();
    }
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->awaitingRemoteAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(otherContact));

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeVideo);
    QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Tp::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Tp::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Tp::MediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStream(stream),
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

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Tp::MediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Tp::MediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend))));
    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Tp::MediaStreamPtr &,
                                              Tp::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Tp::MediaStreamPtr &,
                                              Tp::MediaStreamState))));
    QVERIFY(connect(stream->requestDirection(Tp::MediaStreamDirectionReceive),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (!mSDCStreamReturn || !mSSCStreamReturn) {
        // wait direction and state changed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Tp::MediaStreamDirectionReceive);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionReceive);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Tp::MediaStreamStateConnected);
}

void TestStreamedMediaChan::testOutgoingCallBusy()
{
    // This identifier contains the magic string (busy), which means the example
    // will simulate rejection of the call as busy rather than accepting it.
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "alice (busy)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

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

void TestStreamedMediaChan::testOutgoingCallNoAnswer()
{
    // This identifier contains the magic string (no answer), which means the example
    // will never answer.
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "alice (no answer)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    /* After the initial flurry of D-Bus messages, alice still hasn't answered */
    processDBusQueue(mConn.data());

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
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "alice (terminate)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

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

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Tp::MediaStreamType>() << Tp::MediaStreamTypeAudio),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // wait the contact to appear on RP
    while (mChan->groupRemotePendingContacts().size() == 0) {
        mLoop->processEvents();
    }
    QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
    QCOMPARE(mChan->awaitingRemoteAnswer(), true);

    // wait the contact to accept the call
    while (mChan->groupContacts().size() != 2) {
        mLoop->processEvents();
    }
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->awaitingRemoteAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(otherContact));

    // wait the contact to terminate the call
    QVERIFY(connect(mChan.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SLOT(onChanInvalidated(Tp::DBusProxy *,
                                           const QString &, const QString &))));
    QCOMPARE(mLoop->exec(), 0);
}


void TestStreamedMediaChan::testIncomingCall()
{
    mConn->setSelfPresence("away", "preparing for a test");
    QVERIFY(connect(mConn->requestsInterface(),
                    SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Tp::ChannelDetailsList&))));
    mConn->setSelfPresence("available", "call me?");
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
    QVERIFY(mChan->groupLocalPendingContacts().contains(mConn->selfContact()));

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
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    QCOMPARE(mChan->streams().size(), 1);
    MediaStreamPtr stream = mChan->streams().first();
    QCOMPARE(stream->channel(), mChan);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeAudio);

    // RequestStreams with bad type must fail
    QVERIFY(connect(mChan->requestStream(otherContact, (Tp::MediaStreamType) -1),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QCOMPARE(mRequestStreamsReturn.size(), 0);

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Tp::MediaStreamTypeVideo),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Tp::MediaStreamTypeVideo);
    QCOMPARE(stream->state(), Tp::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Tp::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Tp::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Tp::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Tp::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Tp::MediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStreams(MediaStreams() << stream),
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

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Tp::MediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Tp::MediaStreamPtr &,
                                                  Tp::MediaStreamDirection,
                                                  Tp::MediaStreamPendingSend))));
    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Tp::MediaStreamPtr &,
                                              Tp::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Tp::MediaStreamPtr &,
                                              Tp::MediaStreamState))));
    QVERIFY(connect(stream->requestDirection(false, true),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    while (!mSDCStreamReturn || !mSSCStreamReturn) {
        // wait direction and state changed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Tp::MediaStreamDirectionSend);
    QVERIFY(stream->direction() & Tp::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Tp::MediaStreamStateConnected);
}

void TestStreamedMediaChan::testHold()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "bob"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
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

void TestStreamedMediaChan::testHoldNoUnhold()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "bob (no unhold)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
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
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "bob (inability to unhold)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
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

void TestStreamedMediaChan::testDTMF()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "john"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    MediaStreamPtr stream = mRequestStreamsReturn.first();
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
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "john (no continuous tone)"),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
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
    MediaStreamPtr stream = mRequestStreamsReturn.first();
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
    cleanupImpl();
}

void TestStreamedMediaChan::cleanupTestCase()
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

QTEST_MAIN(TestStreamedMediaChan)
#include "_gen/streamed-media-chan.cpp.moc.hpp"
