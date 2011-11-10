#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/dbus-tube-chan.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/IncomingDBusTubeChannel>
#include <TelepathyQt/OutgoingDBusTubeChannel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingDBusTubeConnection>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/DBusTubeChannel>

#include <telepathy-glib/telepathy-glib.h>

#include <stdio.h>

using namespace Tp;

namespace
{

struct TestContext
{
    bool withContact;
    TpSocketAddressType addressType;
    TpSocketAccessControl accessControl;
};

TestContext contexts[] = {
  { FALSE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { FALSE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_CREDENTIALS },

  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_CREDENTIALS },

  { FALSE, (TpSocketAddressType) NUM_TP_SOCKET_ADDRESS_TYPES, (TpSocketAccessControl) NUM_TP_SOCKET_ACCESS_CONTROLS }
};

void destroySocketControlList(gpointer data)
{
    g_array_free((GArray *) data, TRUE);
}

}

class TestDBusTubeChan : public Test
{
    Q_OBJECT

public:
    TestDBusTubeChan(QObject *parent = 0)
        : Test(parent),
          mConn(0), mChanService(0),
          mGotRemoteConnection(false),
          mGotConnectionClosed(false),
          mOfferFinished(false), mRequiresCredentials(false), mCredentialByte(0)
    { }

protected Q_SLOTS:
    void onBusNamesChanged(const QHash<ContactPtr,QString> &added,
            const QList<ContactPtr> &removed);
    void onOfferFinished(Tp::PendingOperation *op);
    void expectPendingTubeConnectionFinished(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreation();
    void testAcceptTwice();
    void testAcceptSuccess();
    void testAcceptFail();
    void testOfferSuccess();
    void testOutgoingBusNameMonitoring();
    void testExtractBusNameMonitoring();

    void cleanup();
    void cleanupTestCase();

private:

    void createTubeChannel(bool requested, TpSocketAddressType addressType,
            TpSocketAccessControl accessControl, bool withContact);

    TestConnHelper *mConn;
    TpTestsDBusTubeChannel *mChanService;
    DBusTubeChannelPtr mChan;

    uint mCurrentContext;

    QHash<ContactPtr, QString> mCurrentBusNames;
    bool mGotRemoteConnection;
    bool mGotConnectionClosed;
    bool mOfferFinished;
    bool mRequiresCredentials;
    uchar mCredentialByte;

    uint mExpectedHandle;
    QString mExpectedService;
};

void TestDBusTubeChan::onBusNamesChanged(const QHash<ContactPtr, QString> &added,
                                         const QList<ContactPtr> &removed)
{
    qDebug() << "Bus names changed!";
    for (QHash<ContactPtr, QString>::const_iterator i = added.constBegin();
         i != added.constEnd(); ++i) {
        mCurrentBusNames.insert(i.key(), i.value());
        mGotRemoteConnection = true;
        qDebug() << "Adding " << i.key()->id();

        QCOMPARE(i.value(), mExpectedService);
        QCOMPARE(i.key()->handle().first(), mExpectedHandle);
    }
    Q_FOREACH (const ContactPtr &contact, removed) {
        QVERIFY(mCurrentBusNames.contains(contact));
        mCurrentBusNames.remove(contact);
        mGotConnectionClosed = true;
        qDebug() << "Removing " << contact->id();

        QCOMPARE(contact->handle().first(), mExpectedHandle);
    }

    QCOMPARE(mChan->busNames().size(), mCurrentBusNames.size());

    mLoop->quit();
}

void TestDBusTubeChan::onOfferFinished(Tp::PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    mOfferFinished = true;
    mLoop->exit(0);
}

void TestDBusTubeChan::expectPendingTubeConnectionFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingDBusTubeConnection *pdt = qobject_cast<PendingDBusTubeConnection*>(op);
    mRequiresCredentials = pdt->requiresCredentials();
    mCredentialByte = pdt->credentialByte();

    // Check the addresses match
    QCOMPARE(mChan->address(), pdt->address());

    mLoop->exit(0);
}

void TestDBusTubeChan::createTubeChannel(bool requested,
        TpSocketAddressType addressType,
        TpSocketAccessControl accessControl,
        bool withContact)
{
    mChan.reset();
    mLoop->processEvents();
    tp_clear_object(&mChanService);

    /* Create service-side tube channel object */
    QString chanPath = QString(QLatin1String("%1/Channel")).arg(mConn->objectPath());

    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandleRepoIface *roomRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_ROOM);
    TpHandle handle;
    GType type;
    if (withContact) {
        handle = tp_handle_ensure(contactRepo, "bob", NULL, NULL);
        type = TP_TESTS_TYPE_CONTACT_DBUS_TUBE_CHANNEL;
    } else {
        handle = tp_handle_ensure(roomRepo, "#test", NULL, NULL);
        type = TP_TESTS_TYPE_ROOM_DBUS_TUBE_CHANNEL;
    }

    TpHandle alfHandle = tp_handle_ensure(contactRepo, "alf", NULL, NULL);

    GArray *acontrols;
    TpSocketAccessControl a;
    if (accessControl != TP_SOCKET_ACCESS_CONTROL_LOCALHOST) {
        acontrols = g_array_sized_new (FALSE, FALSE, sizeof (guint), 2);

        a = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
        acontrols = g_array_append_val (acontrols, a);
        a = TP_SOCKET_ACCESS_CONTROL_CREDENTIALS;
        acontrols = g_array_append_val (acontrols, a);
    } else {
        acontrols = g_array_sized_new (FALSE, FALSE, sizeof (guint), 1);
        a = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
        acontrols = g_array_append_val (acontrols, a);
    }

    mChanService = TP_TESTS_DBUS_TUBE_CHANNEL(g_object_new(
            type,
            "connection", mConn->service(),
            "handle", handle,
            "requested", requested,
            "object-path", chanPath.toLatin1().constData(),
            "supported-access-controls", acontrols,
            "initiator-handle", alfHandle,
            NULL));

    /* Create client-side tube channel object */
    GHashTable *props;
    g_object_get(mChanService, "channel-properties", &props, NULL);

    if (requested) {
        mChan = OutgoingDBusTubeChannel::create(mConn->client(), chanPath, QVariantMap());
    } else {
        mChan = IncomingDBusTubeChannel::create(mConn->client(), chanPath, QVariantMap());
    }

    g_hash_table_unref(props);
    g_array_unref(acontrols);

    if (withContact)
        tp_handle_unref(contactRepo, handle);
    else
        tp_handle_unref(roomRepo, handle);
}

void TestDBusTubeChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("dbus-tube-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestDBusTubeChan::init()
{
    initImpl();

    mCurrentContext = -1;

    mGotRemoteConnection = false;
    mGotConnectionClosed = false;
    mOfferFinished = false;
    mRequiresCredentials = false;
    mCredentialByte = 0;

    mExpectedHandle = -1;
    mExpectedService = QString();
}

void TestDBusTubeChan::testCreation()
{
    /* Outgoing tube */
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, true);
    QVERIFY(connect(mChan->becomeReady(OutgoingDBusTubeChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(OutgoingDBusTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), false);
    QCOMPARE(mChan->state(), TubeChannelStateNotOffered);
    QCOMPARE(mChan->parameters().isEmpty(), true);
    QCOMPARE(mChan->serviceName(), QLatin1String("com.test.Test"));
    QCOMPARE(mChan->supportsCredentials(), false);
    QCOMPARE(mChan->busNames().isEmpty(), true);
    QCOMPARE(mChan->address(), QString());

    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(IncomingDBusTubeChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingDBusTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), false);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);
    QCOMPARE(mChan->parameters().isEmpty(), false);
    QCOMPARE(mChan->parameters().size(), 1);
    QCOMPARE(mChan->parameters().contains(QLatin1String("badger")), true);
    QCOMPARE(mChan->parameters().value(QLatin1String("badger")), QVariant(42));
    QCOMPARE(mChan->serviceName(), QLatin1String("com.test.Test"));
    QCOMPARE(mChan->supportsCredentials(), false);
    QCOMPARE(mChan->busNames().isEmpty(), true);
    QCOMPARE(mChan->address(), QString());
}

void TestDBusTubeChan::testAcceptTwice()
{
    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(IncomingDBusTubeChannel::FeatureCore |
                        DBusTubeChannel::FeatureBusNameMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingDBusTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), true);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

    IncomingDBusTubeChannelPtr chan = IncomingDBusTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->acceptTube(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->state(), TubeChannelStateOpen);

    /* try to re-accept the tube */
    QVERIFY(connect(chan->acceptTube(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->state(), TubeChannelStateOpen);
}

void TestDBusTubeChan::testAcceptSuccess()
{
    /* incoming tube */
    for (int i = 0; contexts[i].addressType != NUM_TP_SOCKET_ADDRESS_TYPES; i++) {
        /* as we run several tests here, let's init/cleanup properly */
        init();

        qDebug() << "Testing context:" << i;
        mCurrentContext = i;

        createTubeChannel(false, contexts[i].addressType,
                contexts[i].accessControl, contexts[i].withContact);
        QVERIFY(connect(mChan->becomeReady(IncomingDBusTubeChannel::FeatureCore |
                            DBusTubeChannel::FeatureBusNameMonitoring),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(IncomingDBusTubeChannel::FeatureCore), true);
        QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), true);
        QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

        QVERIFY(connect(mChan.data(),
                    SIGNAL(busNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>)),
                    SLOT(onBusNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>))));

        bool requiresCredentials = ((contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) ?
            true : false);

        IncomingDBusTubeChannelPtr chan = IncomingDBusTubeChannelPtr::qObjectCast(mChan);
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QVERIFY(connect(chan->acceptTube(requiresCredentials),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectPendingTubeConnectionFinished(Tp::PendingOperation *))));
        } else {
            // Should never happen
            QVERIFY(false);
        }
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->state(), TubeChannelStateOpen);
        QCOMPARE(mRequiresCredentials, requiresCredentials);

        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            qDebug() << "Connecting to bus" << mChan->address();

            QDBusConnection conn = QDBusConnection::connectToPeer(mChan->address(), mChan->serviceName());

            QCOMPARE(conn.isConnected(), true);
            qDebug() << "Connected to host";
        } else {
            QVERIFY(false);
        }

        /* as we run several tests here, let's init/cleanup properly */
        cleanup();
    }
}

void TestDBusTubeChan::testAcceptFail()
{
    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(IncomingDBusTubeChannel::FeatureCore |
                        DBusTubeChannel::FeatureBusNameMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingDBusTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), true);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

    /* when accept is called the channel will be closed service side */
    tp_tests_dbus_tube_channel_set_close_on_accept (mChanService, TRUE);

    /* calling accept should fail */
    IncomingDBusTubeChannelPtr chan = IncomingDBusTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->acceptTube(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));

    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mChan->isValid(), false);

    /* trying to accept again should fail immediately */
    QVERIFY(connect(chan->acceptTube(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestDBusTubeChan::testOfferSuccess()
{
    /* incoming tube */
    for (int i = 0; contexts[i].addressType != NUM_TP_SOCKET_ADDRESS_TYPES; i++) {
        /* as we run several tests here, let's init/cleanup properly */
        init();

        qDebug() << "Testing context:" << i;
        mCurrentContext = i;

        createTubeChannel(true, contexts[i].addressType,
                contexts[i].accessControl, contexts[i].withContact);
        QVERIFY(connect(mChan->becomeReady(OutgoingDBusTubeChannel::FeatureCore |
                            DBusTubeChannel::FeatureBusNameMonitoring),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(OutgoingDBusTubeChannel::FeatureCore), true);
        QCOMPARE(mChan->isReady(DBusTubeChannel::FeatureBusNameMonitoring), true);
        QCOMPARE(mChan->state(), TubeChannelStateNotOffered);
        QCOMPARE(mChan->parameters().isEmpty(), true);

        mGotRemoteConnection = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(busNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>)),
                    SLOT(onBusNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>))));

        bool requiresCredentials = ((contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) ?
            true : false);

        mExpectedHandle = -1;
        mExpectedService = QString();

        mOfferFinished = false;
        OutgoingDBusTubeChannelPtr chan = OutgoingDBusTubeChannelPtr::qObjectCast(mChan);
        QVariantMap offerParameters;
        offerParameters.insert(QLatin1String("mushroom"), 44);
        qDebug() << "About to offer tube";
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QVERIFY(connect(chan->offerTube(offerParameters, requiresCredentials),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(onOfferFinished(Tp::PendingOperation *))));
        } else {
            QVERIFY(false);
        }

        qDebug() << "Tube offered";

        while (mChan->state() != TubeChannelStateRemotePending) {
            qDebug() << mLoop;
            mLoop->processEvents();
        }

        // A client now connects to the tube
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QDBusConnection conn = QDBusConnection::connectToPeer(mChan->address(), mChan->serviceName());

            QCOMPARE(conn.isConnected(), true);
        } else {
            QVERIFY(false);
        }

        qDebug() << "Connected";

        TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
        TpHandle bobHandle = tp_handle_ensure(contactRepo, "bob", NULL, NULL);
        gchar *bobService = g_strdup("org.bob.test");
        tp_tests_dbus_tube_channel_peer_connected_no_stream(mChanService,
                bobService, bobHandle);
        g_free(bobService);

        mExpectedHandle = bobHandle;
        mExpectedService = QLatin1String("org.bob.test");

        QCOMPARE(mChan->state(), TubeChannelStateRemotePending);

        qDebug() << "Waiting for offer finished";

        while (!mOfferFinished) {
            QCOMPARE(mLoop->exec(), 0);
        }

        qDebug() << "Offer finished";

        QCOMPARE(mChan->state(), TubeChannelStateOpen);
        QCOMPARE(mChan->parameters().isEmpty(), false);
        QCOMPARE(mChan->parameters().size(), 1);
        QCOMPARE(mChan->parameters().contains(QLatin1String("mushroom")), true);
        QCOMPARE(mChan->parameters().value(QLatin1String("mushroom")), QVariant(44));

        // This section makes sense just in a room environment
        if (!contexts[i].withContact) {
            if (!mGotRemoteConnection) {
                QCOMPARE(mLoop->exec(), 0);
            }

            QCOMPARE(mGotRemoteConnection, true);

            qDebug() << "Connected to host";

            mGotConnectionClosed = false;
            tp_tests_dbus_tube_channel_peer_disconnected(mChanService,
                    mExpectedHandle);
            QCOMPARE(mLoop->exec(), 0);
            QCOMPARE(mGotConnectionClosed, true);

            /* let the internal OutgoingDBusTubeChannel::onBusNamesChanged slot be called before
            * checking the data for that connection */
            mLoop->processEvents();

            QCOMPARE(chan->busNames().isEmpty(), true);
        }

        /* as we run several tests here, let's init/cleanup properly */
        cleanup();
    }
}

void TestDBusTubeChan::testOutgoingBusNameMonitoring()
{
    mCurrentContext = 0; // should point to room, localhost
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(OutgoingDBusTubeChannel::FeatureCore |
                    DBusTubeChannel::FeatureBusNameMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(busNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>)),
                    SLOT(onBusNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>))));

    OutgoingDBusTubeChannelPtr chan = OutgoingDBusTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->offerTube(QVariantMap()), // DISCARD
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onOfferFinished(Tp::PendingOperation *))));

    while (mChan->state() != TubeChannelStateRemotePending) {
        mLoop->processEvents();
    }

    // Simulate a peer connection from someone we don't have a prebuilt contact for yet, and
    // immediately drop it
    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandle handle = tp_handle_ensure(contactRepo, "YouHaventSeenMeYet", NULL, NULL);
    gchar *service = g_strdup("org.not.seen.yet");

    mExpectedHandle = handle;
    mExpectedService = QLatin1String("org.not.seen.yet");

    tp_tests_dbus_tube_channel_peer_connected_no_stream(mChanService,
            service, handle);
    tp_tests_dbus_tube_channel_peer_disconnected(mChanService, handle);

    // Test that we get the events in the right sequence
    while (!mOfferFinished || !mGotRemoteConnection) {
        QVERIFY(!mGotConnectionClosed || !mOfferFinished);
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mChan->busNames().size(), 1);

    // The busNamesChanged emission should finally exit the main loop
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mGotConnectionClosed);

    QCOMPARE(mChan->busNames().size(), 0);
}

void TestDBusTubeChan::testExtractBusNameMonitoring()
{
    mCurrentContext = 0; // should point to room, localhost
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(OutgoingDBusTubeChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mChan.data(),
                    SIGNAL(busNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>)),
                    SLOT(onBusNamesChanged(QHash<ContactPtr,QString>,QList<ContactPtr>))));

    OutgoingDBusTubeChannelPtr chan = OutgoingDBusTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->offerTube(QVariantMap()), // DISCARD
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onOfferFinished(Tp::PendingOperation *))));

    while (mChan->state() != TubeChannelStateRemotePending) {
        mLoop->processEvents();
    }

    // Simulate a peer connection from someone
    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandle handle = tp_handle_ensure(contactRepo, "YouHaventSeenMeYet", NULL, NULL);
    gchar *service = g_strdup("org.not.seen.yet");

    mExpectedHandle = handle;
    mExpectedService = QLatin1String("org.not.seen.yet");

    tp_tests_dbus_tube_channel_peer_connected_no_stream(mChanService,
            service, handle);

    while (mChan->state() != TubeChannelStateOpen) {
        mLoop->processEvents();
    }

    // Test that we didn't get a remote connection
    while (!mOfferFinished) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QVERIFY(!mGotConnectionClosed);
    QVERIFY(!mGotRemoteConnection);

    // This should also trigger a warning
    QCOMPARE(mChan->busNames().size(), 0);

    // Now, enable the feature, and let it extract participants
    QVERIFY(connect(mChan->becomeReady(OutgoingDBusTubeChannel::FeatureBusNameMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // This should now be fine
    QCOMPARE(mChan->busNames().size(), 1);
    // The name should match
    QCOMPARE(mChan->busNames().value(mChan->busNames().keys().first()), QLatin1String("org.not.seen.yet"));
    // And the signal shouldn't have been called
    QVERIFY(!mGotConnectionClosed);
    QVERIFY(!mGotRemoteConnection);
}

void TestDBusTubeChan::cleanup()
{
    cleanupImpl();

    if (mChan && mChan->isValid()) {
        qDebug() << "waiting for the channel to become invalidated";

        QVERIFY(connect(mChan.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        tp_base_channel_close(TP_BASE_CHANNEL(mChanService));
        QCOMPARE(mLoop->exec(), 0);
    }

    mChan.reset();

    if (mChanService != 0) {
        g_object_unref(mChanService);
        mChanService = 0;
    }

    mLoop->processEvents();
}

void TestDBusTubeChan::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestDBusTubeChan)
#include "_gen/dbus-tube-chan.cpp.moc.hpp"
