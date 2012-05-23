#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>

#include <TelepathyQt/Connection>
#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingStreamTubeConnection>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/StreamTubeChannel>

#include <telepathy-glib/telepathy-glib.h>

#include <QHostAddress>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTcpServer>
#include <QTcpSocket>

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
          mConn(0), mChanService(0), mLocalConnectionId(-1), mRemoteConnectionId(-1),
          mGotLocalConnection(false), mGotRemoteConnection(false),
          mGotSocketConnection(false), mGotConnectionClosed(false),
          mOfferFinished(false), mRequiresCredentials(false), mCredentialByte(0)
    { }

protected Q_SLOTS:
    void onNewLocalConnection(uint connectionId);
    void onNewRemoteConnection(uint connectionId);
    void onNewSocketConnection();
    void onConnectionClosed(uint connectionId, const QString &errorName,
            const QString &errorMesssage);
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
    void testOutgoingConnectionMonitoring();

    void cleanup();
    void cleanupTestCase();

private:
    void testCheckRemoteConnectionsCommon();

    void createTubeChannel(bool requested, TpSocketAddressType addressType,
            TpSocketAccessControl accessControl, bool withContact);

    TestConnHelper *mConn;
    TpTestsStreamTubeChannel *mChanService;
    StreamTubeChannelPtr mChan;

    uint mCurrentContext;

    uint mLocalConnectionId;
    uint mRemoteConnectionId;
    bool mGotLocalConnection;
    bool mGotRemoteConnection;
    bool mGotSocketConnection;
    bool mGotConnectionClosed;
    bool mOfferFinished;
    bool mRequiresCredentials;
    uchar mCredentialByte;

    QHostAddress mExpectedAddress;
    uint mExpectedPort;
    uint mExpectedHandle;
    QString mExpectedId;
};

void TestStreamTubeChan::onNewLocalConnection(uint connectionId)
{
    qDebug() << "Got local connection with id:" << connectionId;
    mLocalConnectionId = connectionId;
    mGotLocalConnection = true;
    QVERIFY(mChan->connections().contains(connectionId));

    mLoop->exit(0);
}

void TestStreamTubeChan::onNewRemoteConnection(uint connectionId)
{
    qDebug() << "Got remote connection with id:" << connectionId;
    mRemoteConnectionId = connectionId;
    mGotRemoteConnection = true;
    QVERIFY(mChan->connections().contains(connectionId));

    testCheckRemoteConnectionsCommon();
}

void TestStreamTubeChan::onNewSocketConnection()
{
    qDebug() << "Got new socket connection";
    mGotSocketConnection = true;
    mLoop->exit(0);
}

void TestStreamTubeChan::onConnectionClosed(uint connectionId,
        const QString &errorName, const QString &errorMesssage)
{
    qDebug() << "Got connetion closed for connection" << connectionId;
    mGotConnectionClosed = true;
    QVERIFY(!mChan->connections().contains(connectionId));

    if (mChan->isRequested()) {
        testCheckRemoteConnectionsCommon();
    }

    mLoop->exit(0);
}

void TestStreamTubeChan::onOfferFinished(Tp::PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    mOfferFinished = true;
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

    mCurrentContext = -1;

    mLocalConnectionId = -1;
    mRemoteConnectionId = -1;
    mGotLocalConnection = false;
    mGotRemoteConnection = false;
    mGotConnectionClosed = false;
    mGotSocketConnection = false;
    mOfferFinished = false;
    mRequiresCredentials = false;
    mCredentialByte = 0;

    mExpectedAddress = QHostAddress();
    mExpectedPort = -1;
    mExpectedHandle = -1;
    mExpectedId = QString();
}

void TestStreamTubeChan::testCheckRemoteConnectionsCommon()
{
    OutgoingStreamTubeChannelPtr chan = OutgoingStreamTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(chan);

    QCOMPARE(chan->contactsForConnections().isEmpty(), false);
    QCOMPARE(chan->contactsForConnections().contains(mRemoteConnectionId), true);
    QCOMPARE(chan->contactsForConnections().value(mRemoteConnectionId)->handle()[0],
            mExpectedHandle);
    QCOMPARE(chan->contactsForConnections().value(mRemoteConnectionId)->id(),
            mExpectedId);

    if (contexts[mCurrentContext].accessControl == TP_SOCKET_ACCESS_CONTROL_PORT) {
        // qDebug() << "+++ conn for source addresses" << chan->connectionsForSourceAddresses();
        QCOMPARE(chan->connectionsForSourceAddresses().isEmpty(), false);
        QCOMPARE(chan->connectionsForCredentials().isEmpty(), true);
        QPair<QHostAddress, quint16> srcAddr(mExpectedAddress, mExpectedPort);
        QCOMPARE(chan->connectionsForSourceAddresses().contains(srcAddr), true);
        QCOMPARE(chan->connectionsForSourceAddresses().value(srcAddr), mRemoteConnectionId);
    } else if (contexts[mCurrentContext].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) {
        // qDebug() << "+++ conn for credentials" << chan->connectionsForCredentials();
        QCOMPARE(chan->connectionsForCredentials().isEmpty(), false);
        QCOMPARE(chan->connectionsForSourceAddresses().isEmpty(), true);
        QCOMPARE(chan->connectionsForCredentials().contains(mCredentialByte), true);
        QCOMPARE(chan->connectionsForCredentials().value(mCredentialByte), mRemoteConnectionId);
    }

    mLoop->exit(0);
}

void TestStreamTubeChan::testCreation()
{
    /* Outgoing tube */
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, true);
    QVERIFY(connect(mChan->becomeReady(OutgoingStreamTubeChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(OutgoingStreamTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), false);
    QCOMPARE(mChan->state(), TubeChannelStateNotOffered);
    QCOMPARE(mChan->parameters().isEmpty(), true);
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
    QVERIFY(connect(mChan->becomeReady(IncomingStreamTubeChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingStreamTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), false);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);
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
    QVERIFY(connect(mChan->becomeReady(IncomingStreamTubeChannel::FeatureCore |
                        StreamTubeChannel::FeatureConnectionMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingStreamTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

    IncomingStreamTubeChannelPtr chan = IncomingStreamTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->state(), TubeChannelStateOpen);

    /* try to re-accept the tube */
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->state(), TubeChannelStateOpen);
}

void TestStreamTubeChan::testAcceptSuccess()
{
    /* incoming tube */
    for (int i = 0; contexts[i].addressType != NUM_TP_SOCKET_ADDRESS_TYPES; i++) {
        /* as we run several tests here, let's init/cleanup properly */
        init();

        qDebug() << "Testing context:" << i;
        mCurrentContext = i;

        createTubeChannel(false, contexts[i].addressType,
                contexts[i].accessControl, contexts[i].withContact);
        QVERIFY(connect(mChan->becomeReady(IncomingStreamTubeChannel::FeatureCore |
                            StreamTubeChannel::FeatureConnectionMonitoring),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(IncomingStreamTubeChannel::FeatureCore), true);
        QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
        QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

        mLocalConnectionId = -1;
        mGotLocalConnection = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(newConnection(uint)),
                    SLOT(onNewLocalConnection(uint))));

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
        QCOMPARE(mChan->state(), TubeChannelStateOpen);
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
            QCOMPARE(mGotLocalConnection, true);
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
            QCOMPARE(mGotLocalConnection, true);
            qDebug() << "Connected to host";

            if (gSocket) {
                tp_clear_object(&gSocket);
            }

            delete socket;
        }

        mGotConnectionClosed = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(connectionClosed(uint,QString,QString)),
                    SLOT(onConnectionClosed(uint,QString,QString))));
        tp_tests_stream_tube_channel_last_connection_disconnected(mChanService,
                TP_ERROR_STR_DISCONNECTED);
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mGotConnectionClosed, true);

        /* as we run several tests here, let's init/cleanup properly */
        cleanup();
    }
}

void TestStreamTubeChan::testAcceptFail()
{
    /* incoming tube */
    createTubeChannel(false, TP_SOCKET_ADDRESS_TYPE_UNIX,
            TP_SOCKET_ACCESS_CONTROL_LOCALHOST, false);
    QVERIFY(connect(mChan->becomeReady(IncomingStreamTubeChannel::FeatureCore |
                        StreamTubeChannel::FeatureConnectionMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(IncomingStreamTubeChannel::FeatureCore), true);
    QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
    QCOMPARE(mChan->state(), TubeChannelStateLocalPending);

    /* when accept is called the channel will be closed service side */
    tp_tests_stream_tube_channel_set_close_on_accept (mChanService, TRUE);

    /* calling accept should fail */
    IncomingStreamTubeChannelPtr chan = IncomingStreamTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));

    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mChan->isValid(), false);

    /* trying to accept again should fail immediately */
    QVERIFY(connect(chan->acceptTubeAsUnixSocket(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestStreamTubeChan::testOfferSuccess()
{
    /* incoming tube */
    for (int i = 0; contexts[i].addressType != NUM_TP_SOCKET_ADDRESS_TYPES; i++) {
        /* as we run several tests here, let's init/cleanup properly */
        init();

        qDebug() << "Testing context:" << i;
        mCurrentContext = i;

        createTubeChannel(true, contexts[i].addressType,
                contexts[i].accessControl, contexts[i].withContact);
        QVERIFY(connect(mChan->becomeReady(OutgoingStreamTubeChannel::FeatureCore |
                            StreamTubeChannel::FeatureConnectionMonitoring),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(OutgoingStreamTubeChannel::FeatureCore), true);
        QCOMPARE(mChan->isReady(StreamTubeChannel::FeatureConnectionMonitoring), true);
        QCOMPARE(mChan->state(), TubeChannelStateNotOffered);
        QCOMPARE(mChan->parameters().isEmpty(), true);

        mRemoteConnectionId = -1;
        mGotRemoteConnection = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(newConnection(uint)),
                    SLOT(onNewRemoteConnection(uint))));

        bool requiresCredentials = ((contexts[i].accessControl == TP_SOCKET_ACCESS_CONTROL_CREDENTIALS) ?
            true : false);

        mExpectedAddress = QHostAddress();
        mExpectedPort = -1;
        mExpectedHandle = -1;
        mExpectedId = QString();

        mOfferFinished = false;
        mGotSocketConnection = false;
        QLocalServer *localServer = 0;
        QTcpServer *tcpServer = 0;
        OutgoingStreamTubeChannelPtr chan = OutgoingStreamTubeChannelPtr::qObjectCast(mChan);
        QVariantMap offerParameters;
        offerParameters.insert(QLatin1String("mushroom"), 44);
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            localServer = new QLocalServer(this);
            localServer->listen(QLatin1String(tmpnam(NULL)));
            connect(localServer, SIGNAL(newConnection()), SLOT(onNewSocketConnection()));

            QVERIFY(connect(chan->offerUnixSocket(localServer, offerParameters, requiresCredentials),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(onOfferFinished(Tp::PendingOperation *))));
        } else {
            tcpServer = new QTcpServer(this);
            tcpServer->listen(QHostAddress::Any, 0);
            connect(tcpServer, SIGNAL(newConnection()), SLOT(onNewSocketConnection()));

            QVERIFY(connect(chan->offerTcpSocket(tcpServer, offerParameters),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(onOfferFinished(Tp::PendingOperation *))));
        }

        while (mChan->state() != TubeChannelStateRemotePending) {
            mLoop->processEvents();
        }

        QCOMPARE(mGotSocketConnection, false);

        // A client now connects to the tube
        QLocalSocket *localSocket = 0;
        QTcpSocket *tcpSocket = 0;
        if (contexts[i].addressType == TP_SOCKET_ADDRESS_TYPE_UNIX) {
            qDebug() << "Connecting to host" << localServer->fullServerName();
            localSocket = new QLocalSocket(this);
            localSocket->connectToServer(localServer->fullServerName());
        } else {
            qDebug().nospace() << "Connecting to host" << tcpServer->serverAddress() <<
                ":" << tcpServer->serverPort();
            tcpSocket = new QTcpSocket(this);
            tcpSocket->connectToHost(tcpServer->serverAddress(), tcpServer->serverPort());
        }

        QCOMPARE(mGotSocketConnection, false);
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mGotSocketConnection, true);

        if (tcpSocket) {
            mExpectedAddress = tcpSocket->localAddress();
            mExpectedPort = tcpSocket->localPort();
        }

        /* simulate CM when peer connects */
        GValue *connParam = 0;
        mCredentialByte = 0;
        switch (contexts[i].accessControl) {
            case TP_SOCKET_ACCESS_CONTROL_LOCALHOST:
                connParam = tp_g_value_slice_new_static_string("");
                break;

            case TP_SOCKET_ACCESS_CONTROL_CREDENTIALS:
                {
                    mCredentialByte = g_random_int_range(0, G_MAXUINT8);

                    localSocket->write(reinterpret_cast<const char*>(&mCredentialByte), 1);
                    connParam = tp_g_value_slice_new_byte(mCredentialByte);
                }
                break;

            case TP_SOCKET_ACCESS_CONTROL_PORT:
                {
                    connParam = tp_g_value_slice_new_take_boxed(
                            TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4,
                            dbus_g_type_specialized_construct(TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4));
                    dbus_g_type_struct_set(connParam,
                            0, tcpSocket->localAddress().toString().toLatin1().constData(),
                            1, tcpSocket->localPort(),
                            G_MAXUINT);
                }
                break;

            default:
                Q_ASSERT(false);
        }

        TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
        TpHandle bobHandle = tp_handle_ensure(contactRepo, "bob", NULL, NULL);
        tp_tests_stream_tube_channel_peer_connected_no_stream(mChanService,
                connParam, bobHandle);

        tp_g_value_slice_free(connParam);

        mExpectedHandle = bobHandle;
        mExpectedId = QLatin1String("bob");

        QCOMPARE(mChan->state(), TubeChannelStateRemotePending);

        while (!mOfferFinished) {
            QCOMPARE(mLoop->exec(), 0);
        }

        QCOMPARE(mChan->state(), TubeChannelStateOpen);
        QCOMPARE(mChan->parameters().isEmpty(), false);
        QCOMPARE(mChan->parameters().size(), 1);
        QCOMPARE(mChan->parameters().contains(QLatin1String("mushroom")), true);
        QCOMPARE(mChan->parameters().value(QLatin1String("mushroom")), QVariant(44));

        if (!mGotRemoteConnection) {
            QCOMPARE(mLoop->exec(), 0);
        }

        QCOMPARE(mGotRemoteConnection, true);

        qDebug() << "Connected to host";

        mGotConnectionClosed = false;
        QVERIFY(connect(mChan.data(),
                    SIGNAL(connectionClosed(uint,QString,QString)),
                    SLOT(onConnectionClosed(uint,QString,QString))));
        tp_tests_stream_tube_channel_last_connection_disconnected(mChanService,
                TP_ERROR_STR_DISCONNECTED);
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mGotConnectionClosed, true);

        /* let the internal OutgoingStreamTubeChannel::onConnectionClosed slot be called before
         * checking the data for that connection */
        mLoop->processEvents();

        QCOMPARE(chan->contactsForConnections().isEmpty(), true);
        QCOMPARE(chan->connectionsForSourceAddresses().isEmpty(), true);
        QCOMPARE(chan->connectionsForCredentials().isEmpty(), true);

        delete localServer;
        delete localSocket;
        delete tcpServer;
        delete tcpSocket;

        /* as we run several tests here, let's init/cleanup properly */
        cleanup();
    }
}

void TestStreamTubeChan::testOutgoingConnectionMonitoring()
{
    mCurrentContext = 3; // should point to the room, IPv4, AC port one
    createTubeChannel(true, TP_SOCKET_ADDRESS_TYPE_IPV4, TP_SOCKET_ACCESS_CONTROL_PORT, false);
    QVERIFY(connect(mChan->becomeReady(OutgoingStreamTubeChannel::FeatureCore |
                    StreamTubeChannel::FeatureConnectionMonitoring),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mChan.data(),
                SIGNAL(newConnection(uint)),
                SLOT(onNewRemoteConnection(uint))));
    QVERIFY(connect(mChan.data(),
                SIGNAL(connectionClosed(uint,QString,QString)),
                SLOT(onConnectionClosed(uint,QString,QString))));

    OutgoingStreamTubeChannelPtr chan = OutgoingStreamTubeChannelPtr::qObjectCast(mChan);
    QVERIFY(connect(chan->offerTcpSocket(QHostAddress(QHostAddress::LocalHost), 9), // DISCARD
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onOfferFinished(Tp::PendingOperation *))));

    while (mChan->state() != TubeChannelStateRemotePending) {
        mLoop->processEvents();
    }

    /* simulate CM when peer connects */
    GValue *connParam = tp_g_value_slice_new_take_boxed(
            TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4,
            dbus_g_type_specialized_construct(TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4));

    mExpectedAddress.setAddress(QLatin1String("127.0.0.1"));
    mExpectedPort = 12345;

    dbus_g_type_struct_set(connParam,
            0, mExpectedAddress.toString().toLatin1().constData(),
            1, static_cast<quint16>(mExpectedPort),
            G_MAXUINT);

    // Simulate a peer connection from someone we don't have a prebuilt contact for yet, and
    // immediately drop it
    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandle handle = tp_handle_ensure(contactRepo, "YouHaventSeenMeYet", NULL, NULL);

    mExpectedHandle = handle;
    mExpectedId = QLatin1String("youhaventseenmeyet");

    tp_tests_stream_tube_channel_peer_connected_no_stream(mChanService,
            connParam, handle);
    tp_tests_stream_tube_channel_last_connection_disconnected(mChanService,
            TP_ERROR_STR_DISCONNECTED);
    tp_g_value_slice_free(connParam);

    // Test that we get newConnection first and only then connectionClosed, unlike how the code has
    // been for a long time, queueing newConnection events and emitting connectionClosed directly
    while (!mOfferFinished || !mGotRemoteConnection) {
        QVERIFY(!mGotConnectionClosed || !mOfferFinished);
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mChan->connections().size(), 1);

    // The connectionClosed emission should finally exit the main loop
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mGotConnectionClosed);

    QCOMPARE(mChan->connections().size(), 0);
}

void TestStreamTubeChan::cleanup()
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

void TestStreamTubeChan::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamTubeChan)
#include "_gen/stream-tube-chan.cpp.moc.hpp"
