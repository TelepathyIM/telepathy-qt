#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStreamTubeConnection>
#include <TelepathyQt4/StreamTubeChannel>

#include <telepathy-glib/telepathy-glib.h>

#include <QHostAddress>
#include <QLocalSocket>
#include <QTcpSocket>

using namespace Tp;

namespace
{

struct TestContext
{
    bool withContact;
    TpSocketAddressType addressType;
    TpSocketAccessControl accessControl;
};

// FIXME: Enable IPv6 and Port access control tests
TestContext contexts[] = {
  { FALSE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { FALSE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  //{ FALSE, TP_SOCKET_ADDRESS_TYPE_IPV6, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { FALSE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_CREDENTIALS },
  //{ FALSE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_PORT },

  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  //{ TRUE, TP_SOCKET_ADDRESS_TYPE_IPV6, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_CREDENTIALS },
  //{ TRUE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_PORT },

  { FALSE, (TpSocketAddressType) NUM_TP_SOCKET_ADDRESS_TYPES, (TpSocketAccessControl) NUM_TP_SOCKET_ACCESS_CONTROLS }
};

void destroySocketControlList(gpointer data)
{
    g_array_free((GArray *) data, TRUE);
}

GHashTable *createSupportedSocketTypesHash(TpSocketAddressType addressType,
        TpSocketAccessControl accessControl)
{
    GHashTable *ret;
    GArray *tab;

    ret = g_hash_table_new_full(NULL, NULL, NULL, destroySocketControlList);

    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    g_array_append_val(tab, accessControl);

    g_hash_table_insert(ret, GUINT_TO_POINTER(addressType), tab);

    return ret;
}

}

class TestStreamTubeChan : public Test
{
    Q_OBJECT

public:
    TestStreamTubeChan(QObject *parent = 0)
        : Test(parent),
          mConn(0), mChanService(0), mGotConnection(false),
          mRequireCredentials(false), mCredentialByte(0)
    { }

protected Q_SLOTS:
    void onNewConnection(uint connectionId);
    void expectPendingTubeConnectionFinished(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreation();
    void testAcceptTwice();
    void testAcceptSuccess();

    void cleanup();
    void cleanupTestCase();

private:
    void createTubeChannel(bool requested, TpSocketAddressType addressType,
            TpSocketAccessControl accessControl, bool withContact);

    TestConnHelper *mConn;
    TpTestsStreamTubeChannel *mChanService;
    StreamTubeChannelPtr mChan;
    bool mGotConnection;
    bool mRequireCredentials;
    uchar mCredentialByte;
};

void TestStreamTubeChan::onNewConnection(uint connectionId)
{
    Q_UNUSED(connectionId);
    qDebug() << "Got connection with id:" << connectionId;
    mGotConnection = true;
    mLoop->exit(0);
}

void TestStreamTubeChan::expectPendingTubeConnectionFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingStreamTubeConnection *pstc = qobject_cast<PendingStreamTubeConnection*>(op);
    mRequireCredentials = pstc->requireCredentials();
    mCredentialByte = pstc->credentialByte();
    mLoop->exit(0);
}

void TestStreamTubeChan::createTubeChannel(bool requested,
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
        type = TP_TESTS_TYPE_CONTACT_STREAM_TUBE_CHANNEL;
    } else {
        handle = tp_handle_ensure(roomRepo, "#test", NULL, NULL);
        type = TP_TESTS_TYPE_ROOM_STREAM_TUBE_CHANNEL;
    }

    TpHandle alfHandle = tp_handle_ensure(contactRepo, "alf", NULL, NULL);

    GHashTable *sockets = createSupportedSocketTypesHash(addressType, accessControl);

    mChanService = TP_TESTS_STREAM_TUBE_CHANNEL(g_object_new(
            type,
            "connection", mConn->service(),
            "handle", handle,
            "requested", requested,
            "object-path", chanPath.toLatin1().constData(),
            "supported-socket-types", sockets,
            "initiator-handle", alfHandle,
            NULL));

    /* Create client-side tube channel object */
    GHashTable *props;
    g_object_get(mChanService, "channel-properties", &props, NULL);

    if (requested) {
        mChan = OutgoingStreamTubeChannel::create(mConn->client(), chanPath, QVariantMap());
    } else {
        mChan = IncomingStreamTubeChannel::create(mConn->client(), chanPath, QVariantMap());
    }

    g_hash_table_unref(props);
    g_hash_table_unref(sockets);

    if (withContact)
        tp_handle_unref(contactRepo, handle);
    else
        tp_handle_unref(roomRepo, handle);
}

void TestStreamTubeChan::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("stream-tube-chan");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestStreamTubeChan::init()
{
    initImpl();

    mGotConnection = false;
    mRequireCredentials = false;
    mCredentialByte = 0;
}

void TestStreamTubeChan::testCreation()
{
    /* Outgoing tube */
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, true);
    QVERIFY(connect(mChan->becomeReady(StreamTubeChannel::FeatureStreamTube),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureStreamTube), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), false);
    QCOMPARE(mChan->tubeState(), TubeChannelStateNotOffered);
    QCOMPARE(mChan->parameters().isEmpty(), false);
    QCOMPARE(mChan->parameters().size(), 1);
    QCOMPARE(mChan->parameters().contains(QLatin1String("badger")), true);
    QCOMPARE(mChan->parameters().value(QLatin1String("badger")), QVariant(42));
    QCOMPARE(mChan->service(), QLatin1String("test-service"));
    QCOMPARE(mChan->supportsIPv4SocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsIPv4SocketsWithSpecifiedAddress(), false);
    QCOMPARE(mChan->supportsIPv6SocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsIPv6SocketsWithSpecifiedAddress(), false);
    QCOMPARE(mChan->supportsUnixSocketsOnLocalhost(), true);
    QCOMPARE(mChan->supportsUnixSocketsWithCredentials(), false);
    QCOMPARE(mChan->supportsAbstractUnixSocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsAbstractUnixSocketsWithCredentials(), false);
    QCOMPARE(mChan->connections().isEmpty(), true);
    QCOMPARE(mChan->addressType(), SocketAddressTypeUnix);
    QCOMPARE(mChan->ipAddress().first.isNull(), true);
    QCOMPARE(mChan->localAddress(), QString());

    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(StreamTubeChannel::FeatureStreamTube),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureStreamTube), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), false);
    QCOMPARE(mChan->tubeState(), TubeChannelStateLocalPending);
    QCOMPARE(mChan->parameters().isEmpty(), false);
    QCOMPARE(mChan->parameters().size(), 1);
    QCOMPARE(mChan->parameters().contains(QLatin1String("badger")), true);
    QCOMPARE(mChan->parameters().value(QLatin1String("badger")), QVariant(42));
    QCOMPARE(mChan->service(), QLatin1String("test-service"));
    QCOMPARE(mChan->supportsIPv4SocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsIPv4SocketsWithSpecifiedAddress(), false);
    QCOMPARE(mChan->supportsIPv6SocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsIPv6SocketsWithSpecifiedAddress(), false);
    QCOMPARE(mChan->supportsUnixSocketsOnLocalhost(), true);
    QCOMPARE(mChan->supportsUnixSocketsWithCredentials(), false);
    QCOMPARE(mChan->supportsAbstractUnixSocketsOnLocalhost(), false);
    QCOMPARE(mChan->supportsAbstractUnixSocketsWithCredentials(), false);
    QCOMPARE(mChan->connections().isEmpty(), true);
    QCOMPARE(mChan->addressType(), SocketAddressTypeUnix);
    QCOMPARE(mChan->ipAddress().first.isNull(), true);
    QCOMPARE(mChan->localAddress(), QString());
}

void TestStreamTubeChan::testAcceptTwice()
{
    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(StreamTubeChannel::FeatureStreamTube |
                        StreamTubeChannel::FeatureConnectionMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureStreamTube), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
    QCOMPARE(mChan->tubeState(), TubeChannelStateLocalPending);

    IncomingStreamTubeChannelPtr chan = IncomingStreamTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->tubeState(), TubeChannelStateOpen);

    /* try to re-accept the tube */
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 1);
    QCOMPARE(mChan->tubeState(), TubeChannelStateOpen);
}

void TestStreamTubeChan::testAcceptSuccess()
{
    /* incoming tube */
    for (int i = 0; contexts[i].addressType != NUM_TP_SOCKET_ADDRESS_TYPES; i++) {
        qDebug() << "Testing context:" << i;
        createTubeChannel(false, contexts[i].addressType,
                contexts[i].accessControl, contexts[i].withContact);
        QVERIFY(connect(mChan->becomeReady(StreamTubeChannel::FeatureStreamTube |
                            StreamTubeChannel::FeatureConnectionMonitoring),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureStreamTube), true);
        QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
        QCOMPARE(mChan->tubeState(), TubeChannelStateLocalPending);

        QVERIFY(connect(mChan.data(),
                    SIGNAL(newConnection(uint)),
                    SLOT(onNewConnection(uint))));

        bool requireCredentials = ((contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) ?
            true : false);

        IncomingStreamTubeChannelPtr chan = IncomingStreamTubeChannelPtr::qObjectCast(mChan);
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QVERIFY(connect(chan->acceptTubeAsUnixSocket(requireCredentials),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectPendingTubeConnectionFinished(Tp::PendingOperation *))));
        } else {
            // FIXME: Pass the addr+port when calling acceptTubeAsTcpSocket for Port access control
            //if (contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_PORT) {
            //
            //} else {
            //
            //}
            QVERIFY(connect(chan->acceptTubeAsTcpSocket(),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectPendingTubeConnectionFinished(Tp::PendingOperation *))));
        }
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->tubeState(), TubeChannelStateOpen);
        QCOMPARE(mRequireCredentials, requireCredentials);
        qDebug() << "Connecting to host";
        QLocalSocket *localSocket = 0;
        QTcpSocket *tcpSocket = 0;
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QLocalSocket *socket = new QLocalSocket(this);
            socket->connectToServer(mChan->localAddress());
            if (requireCredentials) {
                qDebug() << "Sending credential byte" << mCredentialByte;
                socket->write(reinterpret_cast<const char*>(&mCredentialByte), 1);
            }
        } else {
            QTcpSocket *socket = new QTcpSocket(this);
            socket->connectToHost(mChan->ipAddress().first, mChan->ipAddress().second);
        }
        QCOMPARE(mLoop->exec(), 0);
        qDebug() << "Connected to host";
        QCOMPARE(mGotConnection, true);

        delete localSocket;
        delete tcpSocket;
    }
}

void TestStreamTubeChan::cleanup()
{
    cleanupImpl();

    mChan.reset();
    mLoop->processEvents();

    if (mChanService != 0) {
        g_object_unref(mChanService);
        mChanService = 0;
    }
}

void TestStreamTubeChan::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamTubeChan)
#include "_gen/stream-tube-chan.cpp.moc.hpp"
