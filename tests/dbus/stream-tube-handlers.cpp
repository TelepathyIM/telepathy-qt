#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>
#include <tests/lib/glib/echo/chan.h>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ClientHandlerInterface>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingReady>
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

class ChannelRequestAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelRequest")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelRequest\" >\n"
"    <property name=\"Account\" type=\"o\" access=\"read\" />\n"
"    <property name=\"UserActionTime\" type=\"x\" access=\"read\" />\n"
"    <property name=\"PreferredHandler\" type=\"s\" access=\"read\" />\n"
"    <property name=\"Requests\" type=\"aa{sv}\" access=\"read\" />\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"Hints\" type=\"a{sv}\" access=\"read\" />\n"
"    <method name=\"Proceed\" />\n"
"    <method name=\"Cancel\" />\n"
"    <signal name=\"Failed\" >\n"
"      <arg name=\"Error\" type=\"s\" />\n"
"      <arg name=\"Message\" type=\"s\" />\n"
"    </signal>\n"
"    <signal name=\"Succeeded\" />\n"
"    <signal name=\"SucceededWithChannel\" >\n"
"      <arg name=\"Connection\" type=\"o\" />\n"
"      <arg name=\"ConnectionProperties\" type=\"a{sv}\" />\n"
"      <arg name=\"Channel\" type=\"o\" />\n"
"      <arg name=\"ChannelProperties\" type=\"a{sv}\" />\n"
"    </signal>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Account READ Account)
    Q_PROPERTY(qulonglong UserActionTime READ UserActionTime)
    Q_PROPERTY(QString PreferredHandler READ PreferredHandler)
    Q_PROPERTY(QualifiedPropertyValueMapList Requests READ Requests)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(QVariantMap Hints READ Hints)

public:
    ChannelRequestAdaptor(QDBusObjectPath account,
            qulonglong userActionTime,
            QString preferredHandler,
            QualifiedPropertyValueMapList requests,
            QStringList interfaces,
            bool shouldFail,
            bool proceedNoop,
            QVariantMap hints,
            QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mAccount(account), mUserActionTime(userActionTime),
          mPreferredHandler(preferredHandler), mRequests(requests),
          mInterfaces(interfaces), mShouldFail(shouldFail),
          mProceedNoop(proceedNoop), mHints(hints)
    {
    }

    virtual ~ChannelRequestAdaptor()
    {
    }

    void setChan(const QString &connPath, const QVariantMap &connProps,
            const QString &chanPath, const QVariantMap &chanProps)
    {
        mConnPath = connPath;
        mConnProps = connProps;
        mChanPath = chanPath;
        mChanProps = chanProps;
    }

public: // Properties
    inline QDBusObjectPath Account() const
    {
        return mAccount;
    }

    inline qulonglong UserActionTime() const
    {
        return mUserActionTime;
    }

    inline QString PreferredHandler() const
    {
        return mPreferredHandler;
    }

    inline QualifiedPropertyValueMapList Requests() const
    {
        return mRequests;
    }

    inline QStringList Interfaces() const
    {
        return mInterfaces;
    }

    inline QVariantMap Hints() const
    {
        return mHints;
    }

public Q_SLOTS: // Methods
    void Proceed()
    {
        if (mProceedNoop) {
            return;
        }

        if (mShouldFail) {
            QTimer::singleShot(0, this, SLOT(fail()));
        } else {
            QTimer::singleShot(0, this, SLOT(succeed()));
        }
    }

    void Cancel()
    {
        Q_EMIT Failed(QLatin1String(TELEPATHY_ERROR_CANCELLED), QLatin1String("Cancelled"));
    }

Q_SIGNALS: // Signals
    void Failed(const QString &error, const QString &message);
    void Succeeded();
    void SucceededWithChannel(const QDBusObjectPath &connPath, const QVariantMap &connProps,
            const QDBusObjectPath &chanPath, const QVariantMap &chanProps);

private Q_SLOTS:
    void fail()
    {
        Q_EMIT Failed(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE), QLatin1String("Not available"));
    }

    void succeed()
    {
        if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) {
            Q_EMIT SucceededWithChannel(QDBusObjectPath(mConnPath), mConnProps,
                    QDBusObjectPath(mChanPath), mChanProps);
        }

        Q_EMIT Succeeded();
    }

private:
    QDBusObjectPath mAccount;
    qulonglong mUserActionTime;
    QString mPreferredHandler;
    QualifiedPropertyValueMapList mRequests;
    QStringList mInterfaces;
    bool mShouldFail;
    bool mProceedNoop;
    QVariantMap mHints;
    QString mConnPath, mChanPath;
    QVariantMap mConnProps, mChanProps;
};

void destroySocketControlList(gpointer data)
{
    g_array_free(reinterpret_cast<GArray *>(data), TRUE);
}

GHashTable *createSupportedSocketTypesHash(bool supportMonitoring)
{
    GHashTable *ret;
    GArray *tab;
    TpSocketAccessControl ac;

    ret = g_hash_table_new_full(NULL, NULL, NULL, destroySocketControlList);

    // IPv4
    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    ac = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
    g_array_append_val(tab, ac);

    if (supportMonitoring) {
        ac = TP_SOCKET_ACCESS_CONTROL_PORT;
        g_array_append_val(tab, ac);
    }

    g_hash_table_insert(ret, GUINT_TO_POINTER(TP_SOCKET_ADDRESS_TYPE_IPV4), tab);

    // IPv6
    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    ac = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
    g_array_append_val(tab, ac);

    if (supportMonitoring) {
        ac = TP_SOCKET_ACCESS_CONTROL_PORT;
        g_array_append_val(tab, ac);
    }

    g_hash_table_insert(ret, GUINT_TO_POINTER(TP_SOCKET_ADDRESS_TYPE_IPV6), tab);

    // Named UNIX
    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    ac = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
    g_array_append_val(tab, ac);

    if (supportMonitoring) {
        ac = TP_SOCKET_ACCESS_CONTROL_CREDENTIALS;
        g_array_append_val(tab, ac);
    }

    g_hash_table_insert(ret, GUINT_TO_POINTER(TP_SOCKET_ADDRESS_TYPE_UNIX), tab);

    // Abstract UNIX
    tab = g_array_sized_new(FALSE, FALSE, sizeof(TpSocketAccessControl), 1);
    ac = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
    g_array_append_val(tab, ac);

    if (supportMonitoring) {
        ac = TP_SOCKET_ACCESS_CONTROL_CREDENTIALS;
        g_array_append_val(tab, ac);
    }

    g_hash_table_insert(ret, GUINT_TO_POINTER(TP_SOCKET_ADDRESS_TYPE_ABSTRACT_UNIX), tab);

    return ret;
}

}

class TestStreamTubeHandlers : public Test
{
    Q_OBJECT

public:
    TestStreamTubeHandlers(QObject *parent = 0)
        : Test(parent)
    { }

protected Q_SLOTS:
    void onTubeRequested(const Tp::AccountPtr &, const Tp::OutgoingStreamTubeChannelPtr &,
            const QDateTime &, const Tp::ChannelRequestHints &);
    void onTubeClosed(const Tp::AccountPtr &, const Tp::OutgoingStreamTubeChannelPtr &,
            const QString &, const QString &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegistration();
    void testBasicTcpExport();
    void testSSTHErrorPaths();

    void cleanup();
    void cleanupTestCase();

private:
    QMap<QString, ClientHandlerInterface *> ourHandlers();

    QPair<QString, QVariantMap> createTubeChannel(bool requested, HandleType type,
            bool supportMonitoring);

    AccountManagerPtr mAM;
    AccountPtr mAcc;
    TestConnHelper *mConn;
    QList<TpTestsStreamTubeChannel *> mChanServices;

    OutgoingStreamTubeChannelPtr mRequestedTube;
    QDateTime mRequestTime;
    ChannelRequestHints mRequestHints;

    OutgoingStreamTubeChannelPtr mClosedTube;
    QString mCloseError, mCloseMessage;
};

// TODO: turn into creating one (of possibly many) channels
QPair<QString, QVariantMap> TestStreamTubeHandlers::createTubeChannel(bool requested,
        HandleType handleType,
        bool supportMonitoring)
{
    mLoop->processEvents();

    /* Create service-side tube channel object */
    QString chanPath = QString(QLatin1String("%1/Channel%2%3%4"))
        .arg(mConn->objectPath())
        .arg(requested)
        .arg(static_cast<uint>(handleType))
        .arg(supportMonitoring);
    QVariantMap chanProps;

    chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".ChannelType"),
            TP_QT4_IFACE_CHANNEL_TYPE_STREAM_TUBE);
    chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".Requested"), requested);
    chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".TargetHandleType"),
            static_cast<uint>(handleType));

    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandleRepoIface *roomRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_ROOM);
    TpHandle handle;
    GType type;
    if (handleType == HandleTypeContact) {
        handle = tp_handle_ensure(contactRepo, "bob", NULL, NULL);
        type = TP_TESTS_TYPE_CONTACT_STREAM_TUBE_CHANNEL;
        chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".TargetID"),
                QString::fromLatin1("bob"));
    } else {
        handle = tp_handle_ensure(roomRepo, "#test", NULL, NULL);
        type = TP_TESTS_TYPE_ROOM_STREAM_TUBE_CHANNEL;
        chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".TargetID"),
                QString::fromLatin1("#test"));
    }

    chanProps.insert(TP_QT4_IFACE_CHANNEL + QString::fromLatin1(".TargetHandle"), handle);

    TpHandle alfHandle = tp_handle_ensure(contactRepo, "alf", NULL, NULL);

    GHashTable *sockets = createSupportedSocketTypesHash(supportMonitoring);

    mChanServices.push_back(
            TP_TESTS_STREAM_TUBE_CHANNEL(g_object_new(
                    type,
                    "connection", mConn->service(),
                    "handle", handle,
                    "requested", requested,
                    "object-path", chanPath.toLatin1().constData(),
                    "supported-socket-types", sockets,
                    "initiator-handle", alfHandle,
                    NULL)));

    if (handleType == HandleTypeContact)
        tp_handle_unref(contactRepo, handle);
    else
        tp_handle_unref(roomRepo, handle);

    return qMakePair(chanPath, chanProps);
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

void TestStreamTubeHandlers::onTubeRequested(
        const Tp::AccountPtr &acc,
        const Tp::OutgoingStreamTubeChannelPtr &tube,
        const QDateTime &userActionTime,
        const Tp::ChannelRequestHints &hints)
{
    qDebug() << "tube" << tube->objectPath() << "requested on account" << acc->objectPath();

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
    }

    // We always set the user action time in the past, so if that's carried over correctly, it won't
    // be any more recent than the current time

    if (mRequestTime >= QDateTime::currentDateTime()) {
        qWarning() << "user action time later than expected";
        mLoop->exit(2);
    }

    mRequestedTube = tube;
    mRequestTime = userActionTime;
    mRequestHints = hints;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onTubeClosed(
        const Tp::AccountPtr &acc,
        const Tp::OutgoingStreamTubeChannelPtr &tube,
        const QString &error,
        const QString &message)
{
    qDebug() << "tube" << tube->objectPath() << "closed on account" << acc->objectPath();
    qDebug() << "with error" << error << ':' << message;

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
    }

    mClosedTube = tube;
    mCloseError = error;
    mCloseMessage = message;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("stream-tube-handlers");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mAM = AccountManager::create();
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVariantMap parameters;
    parameters[QLatin1String("account")] = QLatin1String("foobar");
    PendingAccount *pacc = mAM->createAccount(QLatin1String("foo"),
            QLatin1String("bar"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pacc->account());
    mAcc= pacc->account();

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
    QCOMPARE(activatedServer->exportedParameters(), QVariantMap());

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

void TestStreamTubeHandlers::testBasicTcpExport()
{
    StreamTubeServerPtr server =
        StreamTubeServer::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("vsftpd"));

    QVariantMap params;
    params.insert(QLatin1String("username"), QString::fromLatin1("user"));
    params.insert(QLatin1String("password"), QString::fromLatin1("pass"));

    server->exportTcpSocket(QHostAddress::LocalHost, 22, params);

    QVERIFY(server->isRegistered());
    QCOMPARE(server->exportedTcpSocketAddress(),
            qMakePair(QHostAddress(QHostAddress::LocalHost), quint16(22)));
    QCOMPARE(server->exportedParameters(), params);
    QVERIFY(!server->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(server->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::outgoingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(true, HandleTypeContact, false);

    QVERIFY(connect(server.data(),
                SIGNAL(tubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints)),
                SLOT(onTubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints))));

    QDateTime userActionTime = QDateTime::currentDateTime().addDays(-1);
    userActionTime = userActionTime.addMSecs(-userActionTime.time().msec());

    QVariantMap hints;
    hints.insert(QLatin1String("tp-qt4-test-request-hint-herring-color-rgba"), uint(0xff000000));

    QObject *request = new QObject(this);
    QString requestPath =
        QLatin1String("/org/freedesktop/Telepathy/ChannelRequest/RequestForSimpleTcpExport");

    QDBusConnection bus = server->registrar()->dbusConnection();
    new ChannelRequestAdaptor(QDBusObjectPath(mAcc->objectPath()),
            userActionTime.toTime_t(),
            QString(),
            QualifiedPropertyValueMapList(),
            QStringList(),
            false,
            false,
            hints,
            request);
    QVERIFY(bus.registerService(TP_QT4_CHANNEL_DISPATCHER_BUS_NAME));
    QVERIFY(bus.registerObject(requestPath, request));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList() << QDBusObjectPath(requestPath),
            userActionTime.toTime_t(),
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mRequestedTube.isNull());
    QCOMPARE(mRequestedTube->objectPath(), chan.first);
    QCOMPARE(mRequestTime, userActionTime);
    QCOMPARE(mRequestHints.allHints(), hints);

    // Verify that the state recovery accessors return sensible values at this point
    QList<QPair<AccountPtr, OutgoingStreamTubeChannelPtr> > serverTubes = server->tubes();
    QCOMPARE(serverTubes.size(), 1);
    QCOMPARE(serverTubes.first().first->objectPath(), mAcc->objectPath());
    QCOMPARE(serverTubes.first().second, mRequestedTube);

    QVERIFY(server->tcpConnections().isEmpty());

    // Let's run until the tube has been offered
    while (mRequestedTube->isValid() && mRequestedTube->state() != TubeChannelStateRemotePending) {
        mLoop->processEvents();
    }

    QVERIFY(mRequestedTube->isValid());

    // Simulate a peer connecting (makes the tube Open up)
    GValue *connParam = tp_g_value_slice_new_static_string("ignored");
    tp_tests_stream_tube_channel_peer_connected_no_stream(mChanServices.back(),
            connParam, tp_base_channel_get_target_handle(TP_BASE_CHANNEL(mChanServices.back())));
    tp_g_value_slice_free(connParam);

    // The params are set once the tube is open (we've picked up the first peer connection)
    while (mRequestedTube->isValid() && mRequestedTube->state() != TubeChannelStateOpen) {
        mLoop->processEvents();
    }

    // Verify the params
    QVERIFY(mRequestedTube->isValid());
    QCOMPARE(mRequestedTube->parameters(), params);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(server.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString)),
                SLOT(onTubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString))));
    mRequestedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClosedTube, mRequestedTube);
    QCOMPARE(mCloseError, QString(TP_QT4_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testSSTHErrorPaths()
{
    // Create and look up a handler with an incorrectly set up channel factory
    ChannelFactoryPtr chanFactory = ChannelFactory::create(QDBusConnection::sessionBus());
    chanFactory->setSubclassForIncomingStreamTubes<Tp::Channel>();
    StreamTubeServerPtr server =
        StreamTubeServer::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("vsftpd"),
                false,
                AccountFactory::create(QDBusConnection::sessionBus()),
                ConnectionFactory::create(QDBusConnection::sessionBus()),
                chanFactory);
    server->exportTcpSocket(QHostAddress::LocalHost, 22);
    QVERIFY(server->isRegistered());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(server->clientName());
    QVERIFY(handler != 0);

    // Pass it a text channel, and with no satisfied requests
    QString textChanPath = mConn->objectPath() + QLatin1String("/TextChannel");
    QByteArray chanPath(textChanPath.toAscii());
    ExampleEchoChannel *textChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", TpHandle(1),
                NULL));

    ChannelDetails details = { QDBusObjectPath(textChanPath),
        ChannelClassSpec::textChat().allProperties() };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());
    processDBusQueue(mConn->client().data());

    // Now pass it an incoming stream tube chan, which will trigger the error paths for constructing
    // wrong subclasses for tubes
    QPair<QString, QVariantMap> tubeChan = createTubeChannel(false, HandleTypeContact, false);

    details.channel = QDBusObjectPath(tubeChan.first);
    details.properties = tubeChan.second;

    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());
    processDBusQueue(mConn->client().data());

    // Now pass it an outgoing stream tube chan (which we didn't set an incorrect subclass for), but
    // which doesn't actually exist so introspection fails

    details.channel = QDBusObjectPath(QString::fromLatin1("/does/not/exist"));
    details.properties = ChannelClassSpec::outgoingStreamTube(QLatin1String("ftp")).allProperties();

    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());
    processDBusQueue(mConn->client().data());

    // Now pass an actual outgoing tube chan and verify it's still signaled correctly after all
    // these incorrect invocations of the handler
    QVERIFY(connect(server.data(),
                SIGNAL(tubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints)),
                SLOT(onTubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints))));

    tubeChan = createTubeChannel(true, HandleTypeContact, false);

    details.channel = QDBusObjectPath(tubeChan.first);
    details.properties = tubeChan.second;

    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());
    processDBusQueue(mConn->client().data());

    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mRequestedTube->objectPath(), tubeChan.first);

    // TODO: if/when the QDBus bug about not being able to wait for local loop replies even with a
    // main loop is fixed, wait for the HandleChannels invocations to return properly. For now just
    // run 100 pings to the service, during which the codepaths get run almost certainly.
    for (int i = 0; i < 100; i++) {
        processDBusQueue(mConn->client().data());
    }

    g_object_unref(textChanService);
}

void TestStreamTubeHandlers::cleanup()
{
    cleanupImpl();

    mRequestHints = ChannelRequestHints();

    if (mRequestedTube && mRequestedTube->isValid()) {
        qDebug() << "waiting for the channel to become invalidated";

        QVERIFY(connect(mRequestedTube.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        mRequestedTube->requestClose();
        QCOMPARE(mLoop->exec(), 0);
    }

    mRequestedTube.reset();
    mClosedTube.reset();

    while (!mChanServices.empty()) {
        g_object_unref(mChanServices.back());
        mChanServices.pop_back();
    }

    mLoop->processEvents();
}

void TestStreamTubeHandlers::cleanupTestCase()
{
    mAM.reset();
    mAcc.reset();

    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStreamTubeHandlers)
#include "_gen/stream-tube-handlers.cpp.moc.hpp"
