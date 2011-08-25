#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>

#include <TelepathyQt4/ClientHandlerInterface>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/StreamTubeChannel>
#include <TelepathyQt4/StreamTubeClient>
#include <TelepathyQt4/StreamTubeServer>

#include <telepathy-glib/telepathy-glib.h>

#include <cstring>

#include <QTcpServer>

using namespace Tp;
using namespace Tp::Client;

namespace
{

void destroySocketControlList(gpointer data)
{
    g_array_free((GArray *) data, TRUE);
}

// TODO: turn into something which supports everything, or everything except port/credentials ACs
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

class TestStreamTubeHandlers : public Test
{
    Q_OBJECT

public:
    TestStreamTubeHandlers(QObject *parent = 0)
        : Test(parent), mChanService(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegistration();

    void cleanup();
    void cleanupTestCase();

private:
    QMap<QString, ClientHandlerInterface *> ourHandlers();

    void createTubeChannel(bool requested, TpSocketAddressType addressType,
            TpSocketAccessControl accessControl, bool withContact);

    TestConnHelper *mConn;
    TpTestsStreamTubeChannel *mChanService;
    StreamTubeChannelPtr mChan;
};

// TODO: turn into creating one (of possibly many) channels, which support everything, or everything
// except Port and Creds ACs
void TestStreamTubeHandlers::createTubeChannel(bool requested,
        TpSocketAddressType addressType,
        TpSocketAccessControl accessControl,
        bool withContact)
{
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

    g_hash_table_unref(props);
    g_hash_table_unref(sockets);

    if (withContact)
        tp_handle_unref(contactRepo, handle);
    else
        tp_handle_unref(roomRepo, handle);
}

QMap<QString, ClientHandlerInterface *> TestStreamTubeHandlers::ourHandlers()
{
    QStringList registeredNames =
        QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QMap<QString, ClientHandlerInterface *> handlers;

    Q_FOREACH (QString name, registeredNames) {
        if (!name.startsWith(QLatin1String("org.freedesktop.Telepathy.Client."))) {
            continue;
        }

        if (QDBusConnection::sessionBus().interface()->serviceOwner(name).value() !=
                QDBusConnection::sessionBus().baseService()) {
            continue;
        }

        QString path = QLatin1Char('/') + name;
        path.replace(QLatin1Char('.'), QLatin1Char('/'));

        ClientInterface client(name, path);
        QStringList ifaces;
        if (!waitForProperty(client.requestPropertyInterfaces(), &ifaces)) {
            continue;
        }

        if (!ifaces.contains(TP_QT4_IFACE_CLIENT_HANDLER)) {
            continue;
        }

        handlers.insert(name.mid(std::strlen("org.freedesktop.Telepathy.Client.")),
                new ClientHandlerInterface(name, path, this));
    }

    return handlers;
}

void TestStreamTubeHandlers::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("stream-tube-handlers");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestStreamTubeHandlers::init()
{
    initImpl();
}

void TestStreamTubeHandlers::testRegistration()
{
    StreamTubeServerPtr httpServer =
        StreamTubeServer::create(QStringList() << QLatin1String("http"), QStringList());
    StreamTubeServerPtr whiteboardServer =
        StreamTubeServer::create(QStringList() << QLatin1String("sketch"),
                QStringList() << QLatin1String("sketch"), QString(), true);
    StreamTubeServerPtr activatedServer =
        StreamTubeServer::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("vsftpd"));

    StreamTubeClientPtr browser =
        StreamTubeClient::create(QStringList() << QLatin1String("http"), QStringList(),
                QLatin1String("Debian.Iceweasel"));
    StreamTubeClientPtr collaborationTool =
        StreamTubeClient::create(QStringList() << QLatin1String("sketch") << QLatin1String("ftp"),
                QStringList() << QLatin1String("sketch"));

    QCOMPARE(activatedServer->clientName(), QLatin1String("vsftpd"));
    QCOMPARE(browser->clientName(), QLatin1String("Debian.Iceweasel"));

    class CookieGenerator : public StreamTubeServer::ParametersGenerator
    {
    public:
        CookieGenerator() : serial(0) {}

        QVariantMap nextParameters(const AccountPtr &account, const OutgoingStreamTubeChannelPtr &tube,
                const ChannelRequestHints &hints) const
        {
            QVariantMap params;
            params.insert(QLatin1String("cookie-y"),
                    QString(QLatin1String("e982mrh2mr2h+%1")).arg(serial++));
            return params;
        }

    private:
        mutable uint serial; // mmm. I wonder if we should make nextParameters() non-const? that'd require giving a non const pointer when exporting too.
    } httpGenerator;

    QVariantMap whiteboardParams;
    whiteboardParams.insert(QLatin1String("password"),
            QString::fromLatin1("s3kr1t"));

    QTcpServer server;
    server.listen();

    httpServer->exportTcpSocket(QHostAddress::LocalHost, 80, &httpGenerator);
    whiteboardServer->exportTcpSocket(QHostAddress::LocalHost, 31552, whiteboardParams);
    activatedServer->exportTcpSocket(&server);

    browser->setToAcceptAsTcp();
    collaborationTool->setToAcceptAsUnix(true);

    QVERIFY(httpServer->isRegistered());
    QVERIFY(whiteboardServer->isRegistered());
    QVERIFY(activatedServer->isRegistered());
    QVERIFY(browser->isRegistered());
    QVERIFY(collaborationTool->isRegistered());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    QCOMPARE(handlers.size(), 5);

    QVERIFY(handlers.contains(httpServer->clientName()));
    QVERIFY(handlers.contains(whiteboardServer->clientName()));
    QVERIFY(handlers.contains(QLatin1String("vsftpd")));
    QVERIFY(handlers.contains(QLatin1String("Debian.Iceweasel")));
    QVERIFY(handlers.contains(collaborationTool->clientName()));
}

void TestStreamTubeHandlers::cleanup()
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

void TestStreamTubeHandlers::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamTubeHandlers)
#include "_gen/stream-tube-handlers.cpp.moc.hpp"
