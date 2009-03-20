#include <QtCore/QDebug>
#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingContacts>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/StreamedMediaChannel>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/callable/conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestStreamedMediaChan : public Test
{
    Q_OBJECT

public:
    TestStreamedMediaChan(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0)
    { }

protected Q_SLOTS:
    void expectRequestContactsFinished(Telepathy::Client::PendingOperation *);
    void expectCreateChannelFinished(Telepathy::Client::PendingOperation *);
    void expectRequestStreamsFinished(Telepathy::Client::PendingOperation *);
    void onGroupMembersChanged(
            const Telepathy::Client::Contacts &,
            const Telepathy::Client::Contacts &,
            const Telepathy::Client::Contacts &,
            const Telepathy::Client::Contacts &,
            const Telepathy::Client::Channel::GroupMemberChangeDetails &);
    void onStreamRemoved(const Telepathy::Client::MediaStreamPtr &);
    void onStreamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
            Telepathy::MediaStreamDirection,
            Telepathy::MediaStreamPendingSend);
    void onStreamStateChanged(const Telepathy::Client::MediaStreamPtr &,
            Telepathy::MediaStreamState);
    void onNewChannels(const Telepathy::ChannelDetailsList &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testOutgoingCall();
    void testIncomingCall();

    void cleanup();
    void cleanupTestCase();

private:
    ExampleCallableConnection *mConnService;

    Connection *mConn;
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
    Telepathy::MediaStreamDirection mSDCDirectionReturn;
    Telepathy::MediaStreamPendingSend mSDCPendingReturn;
    MediaStreamPtr mSSCStreamReturn;
    Telepathy::MediaStreamState mSSCStateReturn;
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
    mChan = pc->channel();
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
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamRemoved(const MediaStreamPtr &stream)
{
    qDebug() << "stream" << stream.data() << "removed";
    mStreamRemovedReturn = stream;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamDirectionChanged(const MediaStreamPtr &stream,
        Telepathy::MediaStreamDirection direction,
        Telepathy::MediaStreamPendingSend pendingSend)
{
    qDebug() << "stream" << stream.data() << "direction changed to" << direction;
    mSDCStreamReturn = stream;
    mSDCDirectionReturn = direction;
    mSDCPendingReturn = pendingSend;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onStreamStateChanged(const MediaStreamPtr &stream,
        Telepathy::MediaStreamState state)
{
    qDebug() << "stream" << stream.data() << "state changed to" << state;
    mSSCStreamReturn = stream;
    mSSCStateReturn = state;
    mLoop->exit(0);
}

void TestStreamedMediaChan::onNewChannels(const Telepathy::ChannelDetailsList &channels)
{
    qDebug() << "new channels";
    foreach (const Telepathy::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !requested) {
            mChan = StreamedMediaChannelPtr(new StreamedMediaChannel(mConn,
                        details.channel.path(),
                        details.properties));
            mLoop->exit(0);
        }
    }
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

    mConn = new Connection(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->requestConnect(Connection::FeatureSelfContact),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));
}

void TestStreamedMediaChan::init()
{
    initImpl();

    mRequestStreamsReturn.clear();
    mChangedCurrent.clear();
    mChangedLP.clear();
    mChangedRP.clear();
    mChangedRemoved.clear();
    mStreamRemovedReturn.reset();
    mSDCStreamReturn.reset();
    mSDCDirectionReturn = (Telepathy::MediaStreamDirection) -1;
    mSDCPendingReturn = (Telepathy::MediaStreamPendingSend) -1;
    mSSCStateReturn = (Telepathy::MediaStreamState) -1;
    mSSCStreamReturn.reset();
}

void TestStreamedMediaChan::testOutgoingCall()
{
    QVERIFY(connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << "alice"),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestContactsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mRequestContactsReturn.size() == 1);
    ContactPtr otherContact = mRequestContactsReturn.first();
    QVERIFY(otherContact);

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Telepathy::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   otherContact->handle()[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan->isReady(StreamedMediaChannel::FeatureStreams));

    QCOMPARE(mChan->streams().size(), 0);
    QCOMPARE(mChan->groupContacts().size(), 1);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    // RequestStreams with bad type must fail
    QVERIFY(connect(mChan->requestStream(otherContact, (Telepathy::MediaStreamType) -1),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QCOMPARE(mRequestStreamsReturn.size(), 0);

    // Request audio stream
    QVERIFY(connect(mChan->requestStreams(otherContact,
                        QList<Telepathy::MediaStreamType>() << Telepathy::MediaStreamTypeAudio),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    MediaStreamPtr stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Telepathy::MediaStreamTypeAudio);
    QCOMPARE(stream->state(), Telepathy::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Telepathy::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Telepathy::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 1);
    QVERIFY(mChan->streams().contains(stream));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Contacts &,
                            const Telepathy::Client::Channel::GroupMemberChangeDetails &))));
    // wait the contact to appear on RP
    if (mChan->groupRemotePendingContacts().size() == 0) {
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChangedRP.size(), 1);
        QVERIFY(mChan->groupRemotePendingContacts().contains(otherContact));
        QCOMPARE(mChan->awaitingRemoteAnswer(), true);
    }
    // wait the contact to accept the call
    if (mChan->groupContacts().size() == 1) {
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChangedCurrent.size(), 1);
    }
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->awaitingRemoteAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(otherContact));

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Telepathy::MediaStreamTypeVideo),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Telepathy::MediaStreamTypeVideo);
    QCOMPARE(stream->state(), Telepathy::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Telepathy::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Telepathy::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Telepathy::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Telepathy::Client::MediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Telepathy::Client::MediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStream(stream),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (mChan->streams().size() == 2) {
        // wait stream removed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mStreamRemovedReturn, stream);
    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeAudio).size(), 0);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeVideo).size(), 1);

    // test stream direction/state changed
    stream = mChan->streamsForType(Telepathy::MediaStreamTypeVideo).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
                                                  Telepathy::MediaStreamDirection,
                                                  Telepathy::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
                                                  Telepathy::MediaStreamDirection,
                                                  Telepathy::MediaStreamPendingSend))));
    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Telepathy::Client::MediaStreamPtr &,
                                              Telepathy::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Telepathy::Client::MediaStreamPtr &,
                                              Telepathy::MediaStreamState))));
    QVERIFY(connect(stream->requestDirection(Telepathy::MediaStreamDirectionReceive),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (!mSDCStreamReturn) {
        // wait direction and state changed signal
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Telepathy::MediaStreamDirectionReceive);
    QVERIFY(stream->direction() & Telepathy::MediaStreamDirectionReceive);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Telepathy::MediaStreamStateConnected);
}

void TestStreamedMediaChan::testIncomingCall()
{
    mConn->setSelfPresence("away", "preparing for a test");
    QVERIFY(connect(mConn->requestsInterface(),
                    SIGNAL(NewChannels(const Telepathy::ChannelDetailsList&)),
                    SLOT(onNewChannels(const Telepathy::ChannelDetailsList&))));
    mConn->setSelfPresence("available", "call me?");
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan);
    QCOMPARE(mChan->streams().size(), 0);

    QVERIFY(connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
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
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->groupContacts().size(), 2);
    QCOMPARE(mChan->groupLocalPendingContacts().size(), 0);
    QCOMPARE(mChan->groupRemotePendingContacts().size(), 0);
    QCOMPARE(mChan->awaitingLocalAnswer(), false);
    QVERIFY(mChan->groupContacts().contains(mConn->selfContact()));

    QCOMPARE(mChan->streams().size(), 1);
    MediaStreamPtr stream = mChan->streams().first();
    QCOMPARE(stream->channel(), mChan.data());
    QCOMPARE(stream->type(), Telepathy::MediaStreamTypeAudio);

    // RequestStreams with bad type must fail
    QVERIFY(connect(mChan->requestStream(otherContact, (Telepathy::MediaStreamType) -1),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 2);
    QCOMPARE(mRequestStreamsReturn.size(), 0);

    // Request video stream
    QVERIFY(connect(mChan->requestStream(otherContact, Telepathy::MediaStreamTypeVideo),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectRequestStreamsFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mRequestStreamsReturn.size(), 1);
    stream = mRequestStreamsReturn.first();
    QCOMPARE(stream->contact(), otherContact);
    QCOMPARE(stream->type(), Telepathy::MediaStreamTypeVideo);
    QCOMPARE(stream->state(), Telepathy::MediaStreamStateDisconnected);
    QCOMPARE(stream->direction(), Telepathy::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), Telepathy::MediaStreamPendingRemoteSend);

    QCOMPARE(mChan->streams().size(), 2);
    QVERIFY(mChan->streams().contains(stream));

    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeAudio).size(), 1);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeVideo).size(), 1);

    // test stream removal
    stream = mChan->streamsForType(Telepathy::MediaStreamTypeAudio).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamRemoved(const Telepathy::Client::MediaStreamPtr &)),
                    SLOT(onStreamRemoved(const Telepathy::Client::MediaStreamPtr &))));
    QVERIFY(connect(mChan->removeStreams(MediaStreams() << stream),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (mChan->streams().size() == 2) {
        // wait stream removed signal
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mStreamRemovedReturn, stream);
    QCOMPARE(mChan->streams().size(), 1);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeAudio).size(), 0);
    QCOMPARE(mChan->streamsForType(Telepathy::MediaStreamTypeVideo).size(), 1);

    // test stream direction/state changed
    stream = mChan->streamsForType(Telepathy::MediaStreamTypeVideo).first();
    QVERIFY(stream);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
                                                  Telepathy::MediaStreamDirection,
                                                  Telepathy::MediaStreamPendingSend)),
                    SLOT(onStreamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
                                                  Telepathy::MediaStreamDirection,
                                                  Telepathy::MediaStreamPendingSend))));
    QVERIFY(connect(mChan.data(),
                    SIGNAL(streamStateChanged(const Telepathy::Client::MediaStreamPtr &,
                                              Telepathy::MediaStreamState)),
                    SLOT(onStreamStateChanged(const Telepathy::Client::MediaStreamPtr &,
                                              Telepathy::MediaStreamState))));
    QVERIFY(connect(stream->requestDirection(false, true),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    if (!mSDCStreamReturn) {
        // wait direction and state changed signal
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mSDCStreamReturn, stream);
    QVERIFY(mSDCDirectionReturn & Telepathy::MediaStreamDirectionSend);
    QVERIFY(stream->direction() & Telepathy::MediaStreamDirectionSend);
    QCOMPARE(stream->pendingSend(), mSDCPendingReturn);
    QCOMPARE(mSSCStreamReturn, stream);
    QCOMPARE(mSSCStateReturn, Telepathy::MediaStreamStateConnected);
}

void TestStreamedMediaChan::cleanup()
{
    mChan.reset();
    cleanupImpl();
}

void TestStreamedMediaChan::cleanupTestCase()
{
    if (mConn != 0) {
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamedMediaChan)
#include "_gen/streamed-media-chan.cpp.moc.hpp"
