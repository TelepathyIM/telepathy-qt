#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/stream-tube-chan.h>
#include <tests/lib/glib/echo/chan.h>

#include <TelepathyQt/AbstractClient>
#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ClientHandlerInterface>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/StreamTubeChannel>
#include <TelepathyQt/StreamTubeClient>
#include <TelepathyQt/StreamTubeServer>

#include <telepathy-glib/telepathy-glib.h>

#include <cstring>

#include <QTcpServer>
#include <QTcpSocket>

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
        Q_EMIT Failed(QLatin1String(TP_QT_ERROR_CANCELLED), QLatin1String("Cancelled"));
    }

Q_SIGNALS: // Signals
    void Failed(const QString &error, const QString &message);
    void Succeeded();
    void SucceededWithChannel(const QDBusObjectPath &connPath, const QVariantMap &connProps,
            const QDBusObjectPath &chanPath, const QVariantMap &chanProps);

private Q_SLOTS:
    void fail()
    {
        Q_EMIT Failed(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE), QLatin1String("Not available"));
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

GHashTable *createSupportedSocketTypesHash(bool supportMonitoring, bool unixOnly)
{
    GHashTable *ret;
    GArray *tab;
    TpSocketAccessControl ac;

    ret = g_hash_table_new_full(NULL, NULL, NULL, destroySocketControlList);

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

    if (unixOnly) {
        return ret;
    }

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
    void onServerTubeClosed(const Tp::AccountPtr &, const Tp::OutgoingStreamTubeChannelPtr &,
            const QString &, const QString &);
    void onNewServerConnection(const QHostAddress &, quint16, const Tp::AccountPtr &,
            const Tp::ContactPtr &, const Tp::OutgoingStreamTubeChannelPtr &);
    void onServerConnectionClosed(const QHostAddress &, quint16, const Tp::AccountPtr &,
            const Tp::ContactPtr &, const QString &, const QString &,
            const Tp::OutgoingStreamTubeChannelPtr &);

    void onTubeOffered(const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &);
    void onClientTubeClosed(const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &,
            const QString &, const QString &);
    void onClientAcceptedAsTcp(const QHostAddress &, quint16, const QHostAddress &, quint16,
            const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &);
    void onClientAcceptedAsUnix(const QString &, bool reqsCreds, uchar credByte,
            const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &);
    void onNewClientConnection(const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &,
            uint connectionId);
    void onClientConnectionClosed(const Tp::AccountPtr &, const Tp::IncomingStreamTubeChannelPtr &,
            uint, const QString &, const QString &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegistration();
    void testBasicTcpExport();
    void testFailedExport();
    void testServerConnMonitoring();
    void testSSTHErrorPaths();

    void testClientBasicTcp();
    void testClientTcpGeneratorIgnore();
    void testClientTcpUnsupported();

    void testClientBasicUnix();
    void testClientUnixCredsIgnore();
    // the unix AF unsupported codepaths are the same, so no need to test separately
    void testClientConnMonitoring();

    void cleanup();
    void cleanupTestCase();

private:
    QMap<QString, ClientHandlerInterface *> ourHandlers();

    QPair<QString, QVariantMap> createTubeChannel(bool requested, HandleType type,
            bool supportMonitoring, bool unixOnly = false);

    AccountManagerPtr mAM;
    AccountPtr mAcc;
    TestConnHelper *mConn;
    QList<TpTestsStreamTubeChannel *> mChanServices;

    OutgoingStreamTubeChannelPtr mRequestedTube;
    QDateTime mRequestTime;
    ChannelRequestHints mRequestHints;

    OutgoingStreamTubeChannelPtr mServerClosedTube;
    QString mServerCloseError, mServerCloseMessage;

    QHostAddress mNewServerConnectionAddress, mClosedServerConnectionAddress;
    quint16 mNewServerConnectionPort, mClosedServerConnectionPort;
    ContactPtr mNewServerConnectionContact, mClosedServerConnectionContact;
    OutgoingStreamTubeChannelPtr mNewServerConnectionTube, mServerConnectionCloseTube;
    QString mServerConnectionCloseError, mServerConnectionCloseMessage;

    IncomingStreamTubeChannelPtr mOfferedTube;

    IncomingStreamTubeChannelPtr mClientClosedTube;
    QString mClientCloseError, mClientCloseMessage;

    QHostAddress mClientTcpAcceptAddr, mClientTcpAcceptSrcAddr;
    quint16 mClientTcpAcceptPort, mClientTcpAcceptSrcPort;
    IncomingStreamTubeChannelPtr mClientTcpAcceptTube;

    QString mClientUnixAcceptAddr;
    bool mClientUnixReqsCreds;
    IncomingStreamTubeChannelPtr mClientUnixAcceptTube;

    IncomingStreamTubeChannelPtr mNewClientConnectionTube, mClosedClientConnectionTube;
    uint mNewClientConnectionId, mClosedClientConnectionId;
    QString mClientConnectionCloseError, mClientConnectionCloseMessage;
};

QPair<QString, QVariantMap> TestStreamTubeHandlers::createTubeChannel(bool requested,
        HandleType handleType,
        bool supportMonitoring,
        bool unixOnly)
{
    mLoop->processEvents();

    /* Create service-side tube channel object */
    QString chanPath = QString(QLatin1String("%1/Channel%2%3%4"))
        .arg(mConn->objectPath())
        .arg(requested)
        .arg(static_cast<uint>(handleType))
        .arg(supportMonitoring);
    QVariantMap chanProps;

    chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".ChannelType"),
            TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE);
    chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".Requested"), requested);
    chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".TargetHandleType"),
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
        chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".TargetID"),
                QString::fromLatin1("bob"));
    } else {
        handle = tp_handle_ensure(roomRepo, "#test", NULL, NULL);
        type = TP_TESTS_TYPE_ROOM_STREAM_TUBE_CHANNEL;
        chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".TargetID"),
                QString::fromLatin1("#test"));
    }

    chanProps.insert(TP_QT_IFACE_CHANNEL + QString::fromLatin1(".TargetHandle"), handle);

    TpHandle alfHandle = tp_handle_ensure(contactRepo, "alf", NULL, NULL);

    GHashTable *sockets = createSupportedSocketTypesHash(supportMonitoring, unixOnly);

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

        if (!ifaces.contains(TP_QT_IFACE_CLIENT_HANDLER)) {
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
        return;
    }

    // We always set the user action time in the past, so if that's carried over correctly, it won't
    // be any more recent than the current time

    if (mRequestTime >= QDateTime::currentDateTime()) {
        qWarning() << "user action time later than expected";
        mLoop->exit(2);
        return;
    }

    mRequestedTube = tube;
    mRequestTime = userActionTime;
    mRequestHints = hints;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onServerTubeClosed(
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
        return;
    }

    mServerClosedTube = tube;
    mServerCloseError = error;
    mServerCloseMessage = message;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onClientTubeClosed(
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube,
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
        return;
    }

    mClientClosedTube = tube;
    mClientCloseError = error;
    mClientCloseMessage = message;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onNewServerConnection(
        const QHostAddress &sourceAddress,
        quint16 sourcePort,
        const Tp::AccountPtr &acc,
        const Tp::ContactPtr &contact,
        const Tp::OutgoingStreamTubeChannelPtr &tube)
{
    qDebug() << "new conn" << qMakePair(sourceAddress, sourcePort) << "on tube" << tube->objectPath();
    qDebug() << "from contact" << contact->id();

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    if (!tube->connectionsForSourceAddresses().contains(qMakePair(sourceAddress, sourcePort))) {
        qWarning() << "the signaled tube doesn't report having that particular connection";
        mLoop->exit(2);
        return;
    }

    mNewServerConnectionAddress = sourceAddress;
    mNewServerConnectionPort = sourcePort;
    mNewServerConnectionContact = contact;
    mNewServerConnectionTube = tube;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onServerConnectionClosed(
        const QHostAddress &sourceAddress,
        quint16 sourcePort,
        const Tp::AccountPtr &acc,
        const Tp::ContactPtr &contact,
        const QString &error,
        const QString &message,
        const Tp::OutgoingStreamTubeChannelPtr &tube)
{
    qDebug() << "conn" << qMakePair(sourceAddress, sourcePort) << "closed on tube" << tube->objectPath();
    qDebug() << "with error" << error << ':' << message;

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mClosedServerConnectionAddress = sourceAddress;
    mClosedServerConnectionPort = sourcePort;
    mClosedServerConnectionContact = contact;
    mServerConnectionCloseError = error;
    mServerConnectionCloseMessage = message;
    mServerConnectionCloseTube = tube;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onTubeOffered(
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube)
{
    qDebug() << "tube" << tube->objectPath() << "offered to account" << acc->objectPath();

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mOfferedTube = tube;
    mLoop->exit(0);
}

void TestStreamTubeHandlers::onClientAcceptedAsTcp(
        const QHostAddress &listenAddress,
        quint16 listenPort,
        const QHostAddress &sourceAddress,
        quint16 sourcePort,
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube)
{
    qDebug() << "tube" << tube->objectPath() << "accepted at" << qMakePair(listenAddress, listenPort);

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mClientTcpAcceptAddr = listenAddress;
    mClientTcpAcceptPort = listenPort;
    mClientTcpAcceptSrcAddr = sourceAddress;
    mClientTcpAcceptSrcPort = sourcePort;
    mClientTcpAcceptTube = tube;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onClientAcceptedAsUnix(
        const QString &listenAddress,
        bool reqsCreds,
        uchar credByte,
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube)
{
    qDebug() << "tube" << tube->objectPath() << "accepted at" << listenAddress;
    qDebug() << "reqs creds:" << reqsCreds << "cred byte:" << credByte;

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mClientUnixAcceptAddr = listenAddress;
    mClientUnixReqsCreds = reqsCreds;
    mClientUnixAcceptTube = tube;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onNewClientConnection(
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube,
        uint id)
{
    qDebug() << "new conn" << id << "on tube" << tube->objectPath();

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mNewClientConnectionTube = tube;
    mNewClientConnectionId = id;

    mLoop->exit(0);
}

void TestStreamTubeHandlers::onClientConnectionClosed(
        const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube,
        uint id,
        const QString &error,
        const QString &message)
{
    qDebug() << "conn" << id << "closed on tube" << tube->objectPath();
    qDebug() << "with error" << error << ':' << message;

    // We don't use a shared factory here so the proxies will be different, but the object path
    // should be the same
    if (acc->objectPath() != mAcc->objectPath()) {
        qWarning() << "account" << acc->objectPath() << "is not the expected" << mAcc->objectPath();
        mLoop->exit(1);
        return;
    }

    mClosedClientConnectionTube = tube;
    mClosedClientConnectionId = id;
    mClientConnectionCloseError = error;
    mClientConnectionCloseMessage = message;

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
    StreamTubeServerPtr preferredHandlerServer = StreamTubeServer::create(QStringList());

    StreamTubeClientPtr browser =
        StreamTubeClient::create(QStringList() << QLatin1String("http"), QStringList(),
                QLatin1String("Debian.Iceweasel"));
    StreamTubeClientPtr collaborationTool =
        StreamTubeClient::create(QStringList() << QLatin1String("sketch") << QLatin1String("ftp"),
                QStringList() << QLatin1String("sketch"), QString(), false, true);
    StreamTubeClientPtr invalidBecauseNoServicesClient =
        StreamTubeClient::create(QStringList());

    QVERIFY(!httpServer.isNull());
    QVERIFY(!whiteboardServer.isNull());
    QVERIFY(!activatedServer.isNull());
    QVERIFY(!preferredHandlerServer.isNull());
    QVERIFY(!browser.isNull());
    QVERIFY(!collaborationTool.isNull());
    QVERIFY(invalidBecauseNoServicesClient.isNull());

    QCOMPARE(activatedServer->clientName(), QLatin1String("vsftpd"));
    QCOMPARE(browser->clientName(), QLatin1String("Debian.Iceweasel"));

    class CookieGenerator : public StreamTubeServer::ParametersGenerator
    {
    public:
        CookieGenerator() : serial(0) {}

        QVariantMap nextParameters(const AccountPtr &account, const OutgoingStreamTubeChannelPtr &tube,
                const ChannelRequestHints &hints)
        {
            QVariantMap params;
            params.insert(QLatin1String("cookie-y"),
                    QString(QLatin1String("e982mrh2mr2h+%1")).arg(serial++));
            return params;
        }

    private:
        uint serial;
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
    preferredHandlerServer->exportTcpSocket(QHostAddress::LocalHost, 6681);

    browser->setToAcceptAsTcp();
    collaborationTool->setToAcceptAsUnix(true);

    QVERIFY(httpServer->isRegistered());
    QVERIFY(whiteboardServer->isRegistered());
    QVERIFY(activatedServer->isRegistered());
    QVERIFY(preferredHandlerServer->isRegistered());
    QVERIFY(browser->isRegistered());
    QVERIFY(collaborationTool->isRegistered());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());

    QVERIFY(handlers.contains(httpServer->clientName()));
    QVERIFY(handlers.contains(whiteboardServer->clientName()));
    QVERIFY(handlers.contains(QLatin1String("vsftpd")));
    QVERIFY(handlers.contains(preferredHandlerServer->clientName()));
    QVERIFY(handlers.contains(QLatin1String("Debian.Iceweasel")));
    QVERIFY(handlers.contains(collaborationTool->clientName()));

    QCOMPARE(handlers.size(), 6);

    // The only-to-be-used-through-preferredHandler server should have an empty filter, but still be
    // registered and introspectable
    ChannelClassList filter;
    QVERIFY(waitForProperty(handlers.value(preferredHandlerServer->clientName())->
                requestPropertyHandlerChannelFilter(), &filter));
    QVERIFY(filter.isEmpty());

    // We didn't specify bypassApproval = true, so it should be false. for all we know we could be
    // sent some fairly NSFW stuff on a HTTP tube :>
    bool bypass;
    QVERIFY(waitForProperty(handlers.value(browser->clientName())->requestPropertyBypassApproval(),
                &bypass));
    QVERIFY(!bypass);

    // here we did, though, because we want our coworkers to be able to save our ass by launching a
    // brainstorming UI on top of our again rather NSFW browsing habits while we're having a cup
    QVERIFY(waitForProperty(handlers.value(collaborationTool->clientName())->
                requestPropertyBypassApproval(), &bypass));
    QVERIFY(bypass);
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
    hints.insert(QLatin1String("tp-qt-test-request-hint-herring-color-rgba"), uint(0xff000000));

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
    QVERIFY(bus.registerService(TP_QT_CHANNEL_DISPATCHER_BUS_NAME));
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
    QList<StreamTubeServer::Tube> serverTubes = server->tubes();
    QCOMPARE(serverTubes.size(), 1);
    QCOMPARE(serverTubes.first().account()->objectPath(), mAcc->objectPath());
    QCOMPARE(serverTubes.first().channel(), mRequestedTube);

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
                SLOT(onServerTubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString))));
    mRequestedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mServerClosedTube, mRequestedTube);
    QCOMPARE(mServerCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testFailedExport()
{
    StreamTubeServerPtr server =
        StreamTubeServer::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("vsftpd"));
    server->exportTcpSocket(QHostAddress::LocalHost, 22);
    QVERIFY(server->isRegistered());

    QVERIFY(connect(server.data(),
                SIGNAL(tubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints)),
                SLOT(onTubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints))));
    QVERIFY(connect(server.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString)),
                SLOT(onServerTubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString))));

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(server->clientName());
    QVERIFY(handler != 0);

    // To trigger the Offer error codepath, give it a channel which only supports Unix sockets
    // although we're exporting a TCP one - which is always supported in real CMs
    QPair<QString, QVariantMap> chan = createTubeChannel(true, HandleTypeContact, false, true);
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };

    // We should initially get tubeRequested just fine
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mRequestedTube.isNull());
    QCOMPARE(mRequestedTube->objectPath(), chan.first);

    // THEN we should get a tube close because the offer fails
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mServerClosedTube, mRequestedTube);
    QCOMPARE(mServerCloseError, QString(TP_QT_ERROR_NOT_IMPLEMENTED)); // == AF unsupported by "CM"
}

void TestStreamTubeHandlers::testServerConnMonitoring()
{
    StreamTubeServerPtr server =
        StreamTubeServer::create(QStringList(), QStringList() << QLatin1String("multiftp"),
                QLatin1String("warezd"), true);

    server->exportTcpSocket(QHostAddress::LocalHost, 22);

    QVERIFY(server->isRegistered());
    QVERIFY(server->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(server->clientName());
    QVERIFY(handler != 0);

    QPair<QString, QVariantMap> chan = createTubeChannel(true, HandleTypeRoom, true);

    QVERIFY(connect(server.data(),
                SIGNAL(tubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints)),
                SLOT(onTubeRequested(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QDateTime,Tp::ChannelRequestHints))));
    QVERIFY(connect(server.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString)),
                SLOT(onServerTubeClosed(Tp::AccountPtr,Tp::OutgoingStreamTubeChannelPtr,QString,QString))));

    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mRequestedTube.isNull());
    QCOMPARE(mRequestedTube->objectPath(), chan.first);

    // There are no connections at this point
    QVERIFY(server->tcpConnections().isEmpty());

    // Let's run until the tube has been offered
    while (mRequestedTube->isValid() && mRequestedTube->state() != TubeChannelStateRemotePending) {
        mLoop->processEvents();
    }
    QVERIFY(mRequestedTube->isValid());

    // Still no connections
    QVERIFY(server->tcpConnections().isEmpty());

    // Now, some connections actually start popping up
    QVERIFY(connect(server.data(),
                SIGNAL(newTcpConnection(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,Tp::OutgoingStreamTubeChannelPtr)),
                SLOT(onNewServerConnection(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,Tp::OutgoingStreamTubeChannelPtr))));
    QVERIFY(connect(server.data(),
                SIGNAL(tcpConnectionClosed(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,QString,QString,Tp::OutgoingStreamTubeChannelPtr)),
                SLOT(onServerConnectionClosed(QHostAddress,quint16,Tp::AccountPtr,Tp::ContactPtr,QString,QString,Tp::OutgoingStreamTubeChannelPtr))));

    // Simulate the first peer connecting (makes the tube Open up)
    GValue *connParam = tp_g_value_slice_new_take_boxed(
            TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4,
            dbus_g_type_specialized_construct(TP_STRUCT_TYPE_SOCKET_ADDRESS_IPV4));

    QHostAddress expectedAddress = QHostAddress::LocalHost;
    quint16 expectedPort = 1;

    dbus_g_type_struct_set(connParam,
            0, expectedAddress.toString().toLatin1().constData(),
            1, expectedPort,
            G_MAXUINT);

    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    TpHandle handle = tp_handle_ensure(contactRepo, "first", NULL, NULL);

    tp_tests_stream_tube_channel_peer_connected_no_stream(mChanServices.back(), connParam, handle);

    // We should get a newTcpConnection signal now and tcpConnections() should include this new conn
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mNewServerConnectionAddress, expectedAddress);
    QCOMPARE(mNewServerConnectionPort, expectedPort);
    QCOMPARE(mNewServerConnectionContact->handle()[0], handle);
    QCOMPARE(mNewServerConnectionContact->id(), QLatin1String("first"));
    QCOMPARE(mNewServerConnectionTube, mRequestedTube);

    QHash<QPair<QHostAddress, quint16>, StreamTubeServer::RemoteContact > conns =
        server->tcpConnections();
    QCOMPARE(conns.size(), 1);
    QVERIFY(conns.contains(qMakePair(expectedAddress, expectedPort)));
    QCOMPARE(conns.value(qMakePair(expectedAddress, expectedPort)).account()->objectPath(), mAcc->objectPath());
    QCOMPARE(conns.value(qMakePair(expectedAddress, expectedPort)).contact(), mNewServerConnectionContact);

    // Now, close the first connection
    tp_tests_stream_tube_channel_last_connection_disconnected(mChanServices.back(),
            TP_ERROR_STR_DISCONNECTED);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClosedServerConnectionAddress, expectedAddress);
    QCOMPARE(mClosedServerConnectionPort, expectedPort);
    QCOMPARE(mClosedServerConnectionContact, mNewServerConnectionContact);
    QCOMPARE(mServerConnectionCloseError, QString(TP_QT_ERROR_DISCONNECTED));
    QVERIFY(server->tcpConnections().isEmpty());

    // Fire up two new connections
    handle = tp_handle_ensure(contactRepo, "second", NULL, NULL);
    expectedPort = 2;
    dbus_g_type_struct_set(connParam, 1, expectedPort, G_MAXUINT);
    tp_tests_stream_tube_channel_peer_connected_no_stream(mChanServices.back(), connParam, handle);

    handle = tp_handle_ensure(contactRepo, "third", NULL, NULL);
    expectedPort = 3;
    dbus_g_type_struct_set(connParam, 1, expectedPort, G_MAXUINT);
    tp_tests_stream_tube_channel_peer_connected_no_stream(mChanServices.back(), connParam, handle);

    // We should get two newTcpConnection signals now and tcpConnections() should include these
    // connections
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mNewServerConnectionAddress, expectedAddress);
    QCOMPARE(mNewServerConnectionPort, quint16(2));
    QCOMPARE(mNewServerConnectionContact->id(), QLatin1String("second"));
    QCOMPARE(mNewServerConnectionTube, mRequestedTube);
    QCOMPARE(server->tcpConnections().size(), 1);

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mNewServerConnectionAddress, expectedAddress);
    QCOMPARE(mNewServerConnectionPort, quint16(3));
    QCOMPARE(mNewServerConnectionContact->id(), QLatin1String("third"));
    QCOMPARE(mNewServerConnectionTube, mRequestedTube);
    QCOMPARE(server->tcpConnections().size(), 2);

    // Close one of them, and check that we receive the signal for it
    tp_tests_stream_tube_channel_last_connection_disconnected(mChanServices.back(),
            TP_ERROR_STR_DISCONNECTED);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClosedServerConnectionAddress, expectedAddress);
    QCOMPARE(mClosedServerConnectionPort, quint16(3));
    QCOMPARE(mClosedServerConnectionContact, mNewServerConnectionContact);
    QCOMPARE(mServerConnectionCloseError, QString(TP_QT_ERROR_DISCONNECTED));
    QCOMPARE(server->tcpConnections().size(), 1);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(mServerClosedTube.isNull());
    QCOMPARE(server->tubes().size(), 1);
    mClosedServerConnectionContact.reset();
    mRequestedTube->requestClose();

    while (mClosedServerConnectionContact.isNull() || mServerClosedTube.isNull()) {
        QVERIFY(mServerClosedTube.isNull()); // we should get the conn close first, only then tube close
        QCOMPARE(mLoop->exec(), 0);
    }

    QVERIFY(server->tubes().isEmpty());

    QVERIFY(server->tcpConnections().isEmpty());
    QCOMPARE(mClosedServerConnectionAddress, expectedAddress);
    QCOMPARE(mClosedServerConnectionPort, quint16(2));
    QCOMPARE(mClosedServerConnectionContact->id(), QLatin1String("second"));
    QCOMPARE(mServerConnectionCloseError, TP_QT_ERROR_ORPHANED); // parent tube died

    QCOMPARE(mServerClosedTube, mRequestedTube);
    QCOMPARE(mServerCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
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
    QByteArray chanPath(textChanPath.toLatin1());
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

void TestStreamTubeHandlers::testClientBasicTcp()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"));

    class FakeGenerator : public StreamTubeClient::TcpSourceAddressGenerator
    {
        public:
            FakeGenerator() : port(0) {}

            QPair<QHostAddress, quint16> nextSourceAddress(const AccountPtr &account,
                    const IncomingStreamTubeChannelPtr &tube) {
                return qMakePair(QHostAddress(QHostAddress::LocalHost), ++port);
            }

            quint16 port;
    } gen;

    client->setToAcceptAsTcp(&gen);
    QVERIFY(client->isRegistered());
    QCOMPARE(client->registrar()->registeredClients().size(), 1);
    QVERIFY(client->acceptsAsTcp());
    QVERIFY(!client->acceptsAsUnix());
    QCOMPARE(client->tcpGenerator(), static_cast<StreamTubeClient::TcpSourceAddressGenerator *>(&gen));
    QVERIFY(!client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, true);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onClientAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            0, // not an user action
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // Verify that the state recovery accessors return sensible values at this point
    QList<StreamTubeClient::Tube> clientTubes = client->tubes();
    QCOMPARE(clientTubes.size(), 1);
    QCOMPARE(clientTubes.first().account()->objectPath(), mAcc->objectPath());
    QCOMPARE(clientTubes.first().channel(), mOfferedTube);

    QVERIFY(client->connections().isEmpty());

    // Let's run until we've accepted the tube
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mOfferedTube->isValid());
    QCOMPARE(mClientTcpAcceptAddr, QHostAddress(QHostAddress::LocalHost));
    QVERIFY(mClientTcpAcceptPort != 0);
    QCOMPARE(mClientTcpAcceptSrcAddr, QHostAddress(QHostAddress::LocalHost));
    QCOMPARE(mClientTcpAcceptSrcPort, gen.port);
    QCOMPARE(mClientTcpAcceptTube, mOfferedTube);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));
    mOfferedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testClientTcpGeneratorIgnore()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"));

    class FakeGenerator : public StreamTubeClient::TcpSourceAddressGenerator
    {
        public:
            QPair<QHostAddress, quint16> nextSourceAddress(const AccountPtr &account,
                    const IncomingStreamTubeChannelPtr &tube) {
                return qMakePair(QHostAddress(QHostAddress::LocalHost), quint16(1111));
            }
    } gen;

    client->setToAcceptAsTcp(&gen);
    QVERIFY(client->isRegistered());
    QVERIFY(client->acceptsAsTcp());
    QVERIFY(!client->acceptsAsUnix());
    QCOMPARE(client->tcpGenerator(), static_cast<StreamTubeClient::TcpSourceAddressGenerator *>(&gen));
    QVERIFY(!client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, false);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onClientAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            0, // not an user action
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // Verify that the state recovery accessors return sensible values at this point
    QList<StreamTubeClient::Tube> clientTubes = client->tubes();
    QCOMPARE(clientTubes.size(), 1);
    QCOMPARE(clientTubes.first().account()->objectPath(), mAcc->objectPath());
    QCOMPARE(clientTubes.first().channel(), mOfferedTube);

    QVERIFY(client->connections().isEmpty());

    // Let's run until we've accepted the tube
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mOfferedTube->isValid());
    QCOMPARE(mClientTcpAcceptAddr, QHostAddress(QHostAddress::LocalHost));
    QVERIFY(mClientTcpAcceptPort != 0);
    QCOMPARE(mClientTcpAcceptSrcAddr, QHostAddress(QHostAddress::Any));
    QCOMPARE(mClientTcpAcceptSrcPort, quint16(0));
    QCOMPARE(mClientTcpAcceptTube, mOfferedTube);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));
    mOfferedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testClientTcpUnsupported()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"));

    client->setToAcceptAsTcp();
    QVERIFY(client->isRegistered());
    QVERIFY(client->acceptsAsTcp());
    QVERIFY(!client->acceptsAsUnix());
    QVERIFY(!client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, false, true);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            0, // not an user action
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // Now, run until Accept fails (because TCP is not supported by the fake service here) and
    // consequently the tube is closed
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_NOT_IMPLEMENTED)); // == AF unsupported
}

void TestStreamTubeHandlers::testClientBasicUnix()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"));

    client->setToAcceptAsUnix(true);
    QVERIFY(client->isRegistered());
    QCOMPARE(client->registrar()->registeredClients().size(), 1);
    QVERIFY(!client->acceptsAsTcp());
    QVERIFY(client->acceptsAsUnix());
    QCOMPARE(client->tcpGenerator(), static_cast<StreamTubeClient::TcpSourceAddressGenerator *>(0));
    QVERIFY(!client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, true);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeAcceptedAsUnix(QString,bool,uchar,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onClientAcceptedAsUnix(QString,bool,uchar,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            0, // not an user action
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // Verify that the state recovery accessors return sensible values at this point
    QList<StreamTubeClient::Tube> clientTubes = client->tubes();
    QCOMPARE(clientTubes.size(), 1);
    QCOMPARE(clientTubes.first().account()->objectPath(), mAcc->objectPath());
    QCOMPARE(clientTubes.first().channel(), mOfferedTube);

    QVERIFY(client->connections().isEmpty());

    // Let's run until we've accepted the tube
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mOfferedTube->isValid());
    QVERIFY(!mClientUnixAcceptAddr.isNull());
    QVERIFY(mClientUnixReqsCreds);
    QCOMPARE(mClientUnixAcceptTube, mOfferedTube);
    QCOMPARE(mOfferedTube->addressType(), SocketAddressTypeUnix);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));
    mOfferedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testClientUnixCredsIgnore()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"));

    client->setToAcceptAsUnix(true);
    QVERIFY(client->isRegistered());
    QVERIFY(!client->acceptsAsTcp());
    QVERIFY(client->acceptsAsUnix());
    QCOMPARE(client->tcpGenerator(), static_cast<StreamTubeClient::TcpSourceAddressGenerator *>(0));
    QVERIFY(!client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, false);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeAcceptedAsUnix(QString,bool,uchar,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onClientAcceptedAsUnix(QString,bool,uchar,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));

    // Invoke the handler, verifying that we're notified when that happens with the correct tube
    // details
    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            0, // not an user action
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // Verify that the state recovery accessors return sensible values at this point
    QList<StreamTubeClient::Tube> clientTubes = client->tubes();
    QCOMPARE(clientTubes.size(), 1);
    QCOMPARE(clientTubes.first().account()->objectPath(), mAcc->objectPath());
    QCOMPARE(clientTubes.first().channel(), mOfferedTube);

    QVERIFY(client->connections().isEmpty());

    // Let's run until we've accepted the tube
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mOfferedTube->isValid());
    QVERIFY(!mClientUnixAcceptAddr.isNull());
    QVERIFY(!mClientUnixReqsCreds);
    QCOMPARE(mClientUnixAcceptTube, mOfferedTube);
    QCOMPARE(mOfferedTube->addressType(), SocketAddressTypeUnix);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));
    mOfferedTube->requestClose();
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::testClientConnMonitoring()
{
    StreamTubeClientPtr client =
        StreamTubeClient::create(QStringList() << QLatin1String("ftp"), QStringList(),
                QLatin1String("ncftp"), true);

    client->setToAcceptAsTcp();
    QVERIFY(client->isRegistered());
    QVERIFY(client->acceptsAsTcp());
    QVERIFY(client->monitorsConnections());

    QMap<QString, ClientHandlerInterface *> handlers = ourHandlers();

    QVERIFY(!handlers.isEmpty());
    ClientHandlerInterface *handler = handlers.value(client->clientName());
    QVERIFY(handler != 0);

    ChannelClassList filter;
    QVERIFY(waitForProperty(handler->requestPropertyHandlerChannelFilter(), &filter));

    QCOMPARE(filter.size(), 1);
    QVERIFY(ChannelClassSpec(filter.first())
            == ChannelClassSpec::incomingStreamTube(QLatin1String("ftp")));

    QPair<QString, QVariantMap> chan = createTubeChannel(false, HandleTypeContact, true);

    QVERIFY(connect(client.data(),
                SIGNAL(tubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onTubeOffered(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));
    QVERIFY(connect(client.data(),
                SIGNAL(tubeAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr)),
                SLOT(onClientAcceptedAsTcp(QHostAddress,quint16,QHostAddress,quint16,
                        Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr))));

    ChannelDetails details = { QDBusObjectPath(chan.first), chan.second };
    handler->HandleChannels(
            QDBusObjectPath(mAcc->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            ChannelDetailsList() << details,
            ObjectPathList(),
            QDateTime::currentDateTime().toTime_t(),
            QVariantMap());

    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mOfferedTube.isNull());
    QCOMPARE(mOfferedTube->objectPath(), chan.first);

    // There are no connections at this point
    QVERIFY(client->connections().isEmpty());

    // Let's run until we've accepted the tube
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mOfferedTube->isValid());
    QCOMPARE(mClientTcpAcceptAddr, QHostAddress(QHostAddress::LocalHost));
    QVERIFY(mClientTcpAcceptPort != 0);
    QCOMPARE(mClientTcpAcceptSrcAddr, QHostAddress(QHostAddress::Any));
    QCOMPARE(mClientTcpAcceptSrcPort, quint16(0));
    QCOMPARE(mClientTcpAcceptTube, mOfferedTube);

    // Still no connections
    QVERIFY(client->connections().isEmpty());

    // Now, some connections actually start popping up
    QVERIFY(connect(client.data(),
                SIGNAL(newConnection(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,uint)),
                SLOT(onNewClientConnection(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,uint))));
    QVERIFY(connect(client.data(),
                SIGNAL(connectionClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,uint,QString,QString)),
                SLOT(onClientConnectionClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,uint,QString,QString))));

    QTcpSocket first;
    first.connectToHost(mClientTcpAcceptAddr, mClientTcpAcceptPort);
    first.waitForConnected();
    QVERIFY(first.isValid());
    QCOMPARE(first.state(), QAbstractSocket::ConnectedState);

    // We should get a newConnection signal now and connections() should include this new conn
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mNewClientConnectionTube, mOfferedTube);
    QHash<StreamTubeClient::Tube, QSet<uint> > conns = client->connections();
    QCOMPARE(conns.size(), 1);
    QCOMPARE(conns.values().first().size(), 1);
    QVERIFY(conns.values().first().contains(mNewClientConnectionId));
    uint firstId = mNewClientConnectionId;

    // Now, close the first connection
    tp_tests_stream_tube_channel_last_connection_disconnected(mChanServices.back(),
            TP_ERROR_STR_DISCONNECTED);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClosedClientConnectionId, firstId);
    QCOMPARE(mClientConnectionCloseError, QString(TP_QT_ERROR_DISCONNECTED));
    QVERIFY(client->connections().isEmpty());

    // Fire up two new connections
    QTcpSocket second;
    second.connectToHost(mClientTcpAcceptAddr, mClientTcpAcceptPort);
    second.waitForConnected();
    QVERIFY(second.isValid());
    QCOMPARE(second.state(), QAbstractSocket::ConnectedState);

    QTcpSocket third;
    third.connectToHost(mClientTcpAcceptAddr, mClientTcpAcceptPort);
    third.waitForConnected();
    QVERIFY(third.isValid());
    QCOMPARE(third.state(), QAbstractSocket::ConnectedState);

    // We should get two newConnection signals now and connections() should include these
    // connections
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mNewClientConnectionTube, mOfferedTube);
    QCOMPARE(client->connections().size(), 1);
    uint secondId = mNewClientConnectionId;

    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mNewClientConnectionTube, mOfferedTube);
    QCOMPARE(client->connections().size(), 1);
    uint thirdId = mNewClientConnectionId;

    conns = client->connections();
    QCOMPARE(conns.size(), 1);
    QCOMPARE(conns.values().first().size(), 2);
    QVERIFY(conns.values().first().contains(secondId));
    QVERIFY(conns.values().first().contains(thirdId));

    // Close one of them, and check that we receive the signal for it
    tp_tests_stream_tube_channel_last_connection_disconnected(mChanServices.back(),
            TP_ERROR_STR_DISCONNECTED);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mClosedClientConnectionId, thirdId);
    QCOMPARE(mClientConnectionCloseError, QString(TP_QT_ERROR_DISCONNECTED));
    QCOMPARE(mClosedServerConnectionContact, mNewServerConnectionContact);

    conns = client->connections();
    QCOMPARE(conns.size(), 1);
    QVERIFY(!conns.values().first().contains(thirdId));
    QVERIFY(conns.values().first().contains(secondId));
    QCOMPARE(conns.values().first().size(), 1);

    // Now, close the tube and verify we're signaled about that
    QVERIFY(connect(client.data(),
                SIGNAL(tubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString)),
                SLOT(onClientTubeClosed(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,QString))));
    mClosedClientConnectionId = 0xdeadbeefU;
    QVERIFY(mClientClosedTube.isNull());
    QCOMPARE(client->tubes().size(), 1);
    mOfferedTube->requestClose();

    while (mClosedClientConnectionId == 0xdeadbeefU || mClientClosedTube.isNull()) {
        QVERIFY(mClientClosedTube.isNull()); // we should get first conn close, then tube close
        QCOMPARE(mLoop->exec(), 0);
    }

    QVERIFY(client->tubes().isEmpty());

    QVERIFY(client->connections().isEmpty());
    QCOMPARE(mClosedClientConnectionId, secondId);
    QCOMPARE(mClientConnectionCloseError, TP_QT_ERROR_ORPHANED); // parent tube died

    QCOMPARE(mClientClosedTube, mOfferedTube);
    QCOMPARE(mClientCloseError, QString(TP_QT_ERROR_CANCELLED)); // == local close request
}

void TestStreamTubeHandlers::cleanup()
{
    cleanupImpl();

    mRequestHints = ChannelRequestHints();

    if (mRequestedTube && mRequestedTube->isValid()) {
        qDebug() << "waiting for the reqd tube to become invalidated";

        QVERIFY(connect(mRequestedTube.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        mRequestedTube->requestClose();
        QCOMPARE(mLoop->exec(), 0);
    }

    mRequestedTube.reset();
    mServerClosedTube.reset();
    mNewServerConnectionContact.reset();
    mClosedServerConnectionContact.reset();
    mNewServerConnectionTube.reset();
    mServerConnectionCloseTube.reset();

    if (mOfferedTube && mOfferedTube->isValid()) {
        qDebug() << "waiting for the ofrd tube to become invalidated";

        QVERIFY(connect(mOfferedTube.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        mOfferedTube->requestClose();
        QCOMPARE(mLoop->exec(), 0);
    }

    mOfferedTube.reset();
    mClientClosedTube.reset();
    mClientTcpAcceptTube.reset();
    mClientUnixAcceptTube.reset();
    mNewClientConnectionTube.reset();
    mClosedClientConnectionTube.reset();

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
