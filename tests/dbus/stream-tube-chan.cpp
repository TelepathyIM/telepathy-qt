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
  { FALSE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_PORT },

  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  //{ TRUE, TP_SOCKET_ADDRESS_TYPE_IPV6, TP_SOCKET_ACCESS_CONTROL_LOCALHOST },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_UNIX, TP_SOCKET_ACCESS_CONTROL_CREDENTIALS },
  { TRUE, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_PORT },

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

GSocket *createTcpClientGSocket(TpSocketAddressType socketType)
{
    Q_ASSERT(socketType != TP_SOCKET_ADDRESS_TYPE_UNIX);

    GSocketFamily family = (GSocketFamily) 0;
    switch (socketType) {
        case TP_SOCKET_ADDRESS_TYPE_UNIX:
            family = G_SOCKET_FAMILY_UNIX;
            break;

        case TP_SOCKET_ADDRESS_TYPE_IPV4:
            family = G_SOCKET_FAMILY_IPV4;
            break;

        case TP_SOCKET_ADDRESS_TYPE_IPV6:
            family = G_SOCKET_FAMILY_IPV6;
            break;

        default:
            Q_ASSERT(false);
    }

    /* Create socket to connect to the CM */
    GError *error = NULL;
    GSocket *clientSocket = g_socket_new(family, G_SOCKET_TYPE_STREAM,
            G_SOCKET_PROTOCOL_DEFAULT, &error);
    Q_ASSERT(clientSocket != NULL);

    if (socketType == TP_SOCKET_ADDRESS_TYPE_IPV4 ||
        socketType == TP_SOCKET_ADDRESS_TYPE_IPV6) {
        /* Bind local address */
        GSocketAddress *localAddress;
        GInetAddress *tmp;
        gboolean success;

        tmp = g_inet_address_new_loopback(family);
        localAddress = g_inet_socket_address_new(tmp, 0);

        success = g_socket_bind(clientSocket, localAddress,
                TRUE, &error);

        g_object_unref(tmp);
        g_object_unref(localAddress);

        Q_ASSERT(success);
    }

    return clientSocket;
}

}

class TestStreamTubeChan : public Test
{
    Q_OBJECT

public:
    TestStreamTubeChan(QObject *parent = 0)
        : Test(parent),
          mConn(0), mChanService(0), mGotConnection(false),
          mRequiresCredentials(false), mCredentialByte(0)
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
    bool mRequiresCredentials;
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
    mRequiresCredentials = pstc->requiresCredentials();
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
    mRequiresCredentials = false;
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

        mGotConnection = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(newConnection(uint)),
                    SLOT(onNewConnection(uint))));

        bool requiresCredentials = ((contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) ?
            true : false);

        GSocket *gSocket = 0;
        QHostAddress addr;
        quint16 port = 0;
        IncomingStreamTubeChannelPtr chan = IncomingStreamTubeChannelPtr::qObjectCast(mChan);
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            QVERIFY(connect(chan->acceptTubeAsUnixSocket(requiresCredentials),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectPendingTubeConnectionFinished(Tp::PendingOperation *))));
        } else {
            if (contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_PORT) {
                gSocket = createTcpClientGSocket(contexts[i].addressType);

                // There is no way to bind a QTcpSocket and using
                // QAbstractSocket::setSocketDescriptor does not work either, so using glib sockets
                // for this test. See http://bugreports.qt.nokia.com/browse/QTBUG-121
                GSocketAddress *localAddr;

                localAddr = g_socket_get_local_address(gSocket, NULL);
                QVERIFY(localAddr != NULL);
                addr = QHostAddress(QLatin1String(g_inet_address_to_string(
                        g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(localAddr)))));
                port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(localAddr));
                g_object_unref(localAddr);
            } else {
                addr = QHostAddress::Any;
                port = 0;
            }

            QVERIFY(connect(chan->acceptTubeAsTcpSocket(addr, port),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectPendingTubeConnectionFinished(Tp::PendingOperation *))));
        }
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->tubeState(), TubeChannelStateOpen);
        QCOMPARE(mRequiresCredentials, requiresCredentials);

        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            qDebug() << "Connecting to host" << mChan->localAddress();

            QLocalSocket *socket = new QLocalSocket(this);
            socket->connectToServer(mChan->localAddress());

            if (requiresCredentials) {
                qDebug() << "Sending credential byte" << mCredentialByte;
                socket->write(reinterpret_cast<const char*>(&mCredentialByte), 1);
            }

            QCOMPARE(mLoop->exec(), 0);
            QCOMPARE(mGotConnection, true);
            qDebug() << "Connected to host";

            delete socket;
        } else {
            qDebug().nospace() << "Connecting to host " << mChan->ipAddress().first << ":" <<
                mChan->ipAddress().second;

            QTcpSocket *socket = 0;

            if (contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_PORT) {
                GSocketAddress *remoteAddr = (GSocketAddress*) g_inet_socket_address_new(
                        g_inet_address_new_from_string(
                            mChan->ipAddress().first.toString().toLatin1().constData()),
                        mChan->ipAddress().second);
                g_socket_connect(gSocket, remoteAddr, NULL, NULL);
            } else {
                socket = new QTcpSocket();
                socket->connectToHost(mChan->ipAddress().first, mChan->ipAddress().second);
            }

            QCOMPARE(mLoop->exec(), 0);
            QCOMPARE(mGotConnection, true);
            qDebug() << "Connected to host";

            if (gSocket) {
                tp_clear_object(&gSocket);
            }

            delete socket;
        }
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
