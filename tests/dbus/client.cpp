#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo/chan.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/AbstractClientObserver>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/ClientHandlerInterface>
#include <TelepathyQt/ClientInterfaceRequestsInterface>
#include <TelepathyQt/ClientObserverInterface>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingReady>

#include <telepathy-glib/debug.h>

using namespace Tp;
using namespace Tp::Client;

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
"    <method name=\"Proceed\" />\n"
"    <method name=\"Cancel\" />\n"
"    <signal name=\"Failed\" >\n"
"      <arg name=\"Error\" type=\"s\" />\n"
"      <arg name=\"Message\" type=\"s\" />\n"
"    </signal>\n"
"    <signal name=\"Succeeded\" />"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Account READ Account)
    Q_PROPERTY(qulonglong UserActionTime READ UserActionTime)
    Q_PROPERTY(QString PreferredHandler READ PreferredHandler)
    Q_PROPERTY(Tp::QualifiedPropertyValueMapList Requests READ Requests)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    ChannelRequestAdaptor(QDBusObjectPath account,
            qulonglong userActionTime,
            QString preferredHandler,
            QualifiedPropertyValueMapList requests,
            QStringList interfaces,
            QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mAccount(account), mUserActionTime(userActionTime),
          mPreferredHandler(preferredHandler), mRequests(requests),
          mInterfaces(interfaces)
    {
    }

    virtual ~ChannelRequestAdaptor()
    {
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

public Q_SLOTS: // Methods
    void Proceed()
    {
    }

    void Cancel()
    {
    }

Q_SIGNALS: // Signals
    void Failed(const QString &error, const QString &message);
    void Succeeded();

private:
    QDBusObjectPath mAccount;
    qulonglong mUserActionTime;
    QString mPreferredHandler;
    QualifiedPropertyValueMapList mRequests;
    QStringList mInterfaces;
};

// Totally incomplete mini version of ChannelDispatchOperation
class ChannelDispatchOperationAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelDispatchOperation")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelDispatchOperation\" >\n"
"    <property name=\"Account\" type=\"o\" access=\"read\" />\n"
"    <property name=\"Connection\" type=\"o\" access=\"read\" />\n"
"    <property name=\"Channels\" type=\"a(oa{sv})\" access=\"read\" />\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"PossibleHandlers\" type=\"as\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Account READ Account)
    Q_PROPERTY(QDBusObjectPath Connection READ Connection)
    Q_PROPERTY(Tp::ChannelDetailsList Channels READ Channels)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(QStringList PossibleHandlers READ PossibleHandlers)

public:
    ChannelDispatchOperationAdaptor(const QDBusObjectPath &acc, const QDBusObjectPath &conn,
            const ChannelDetailsList &channels, const QStringList &possibleHandlers,
            QObject *parent)
        : QDBusAbstractAdaptor(parent), mAccount(acc), mConn(conn), mChannels(channels),
          mPossibleHandlers(possibleHandlers)
    {
    }

    virtual ~ChannelDispatchOperationAdaptor()
    {
    }

public: // Properties
    inline QDBusObjectPath Account() const
    {
        return mAccount;
    }

    inline QDBusObjectPath Connection() const
    {
        return mConn;
    }

    inline ChannelDetailsList Channels() const
    {
        return mChannels;
    }

    inline QStringList Interfaces() const
    {
        return mInterfaces;
    }

    inline QStringList PossibleHandlers() const
    {
        return mPossibleHandlers;
    }

public Q_SLOTS:
    inline void Claim()
    {
        // do nothing = no fail
    }

private:
    QDBusObjectPath mAccount, mConn;
    ChannelDetailsList mChannels;
    QStringList mInterfaces;
    QStringList mPossibleHandlers;
};

class MyClient : public QObject,
                 public AbstractClientObserver,
                 public AbstractClientApprover,
                 public AbstractClientHandler
{
    Q_OBJECT

public:
    static AbstractClientPtr create(const ChannelClassSpecList &channelFilter,
            const AbstractClientHandler::Capabilities &capabilities,
            bool bypassApproval = false,
            bool wantsRequestNotification = false)
    {
        return AbstractClientPtr::dynamicCast(SharedPtr<MyClient>(
                    new MyClient(channelFilter, capabilities,
                        bypassApproval, wantsRequestNotification)));
    }

    MyClient(const ChannelClassSpecList &channelFilter,
             const AbstractClientHandler::Capabilities &capabilities,
             bool bypassApproval = false,
             bool wantsRequestNotification = false)
        : AbstractClientObserver(channelFilter),
          AbstractClientApprover(channelFilter),
          AbstractClientHandler(channelFilter, capabilities, wantsRequestNotification),
          mBypassApproval(bypassApproval)
    {
    }

    ~MyClient()
    {
    }

    void observeChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const AbstractClientObserver::ObserverInfo &observerInfo)
    {
        mObserveChannelsAccount = account;
        mObserveChannelsConnection = connection;
        mObserveChannelsChannels = channels;
        mObserveChannelsDispatchOperation = dispatchOperation;
        mObserveChannelsRequestsSatisfied = requestsSatisfied;
        mObserveChannelsObserverInfo = observerInfo;

        context->setFinished();
        QTimer::singleShot(0, this, SIGNAL(observeChannelsFinished()));
    }

    void addDispatchOperation(const MethodInvocationContextPtr<> &context,
            const ChannelDispatchOperationPtr &dispatchOperation)
    {
        mAddDispatchOperationChannels = dispatchOperation->channels();
        mAddDispatchOperationDispatchOperation = dispatchOperation;

        QVERIFY(connect(dispatchOperation->claim(AbstractClientHandlerPtr(this)),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SIGNAL(claimFinished())));

        context->setFinished();
        QTimer::singleShot(0, this, SIGNAL(addDispatchOperationFinished()));
    }

    bool bypassApproval() const
    {
        return mBypassApproval;
    }

    void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const AbstractClientHandler::HandlerInfo &handlerInfo)
    {
        mHandleChannelsAccount = account;
        mHandleChannelsConnection = connection;
        mHandleChannelsChannels = channels;
        mHandleChannelsRequestsSatisfied = requestsSatisfied;
        mHandleChannelsUserActionTime = userActionTime;
        mHandleChannelsHandlerInfo = handlerInfo;

        Q_FOREACH (const ChannelPtr &channel, channels) {
            connect(channel.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SIGNAL(channelClosed()));
        }

        context->setFinished();
        QTimer::singleShot(0, this, SIGNAL(handleChannelsFinished()));
    }

    void addRequest(const ChannelRequestPtr &request)
    {
        mAddRequestRequest = request;
        Q_EMIT requestAdded(request);
    }

    void removeRequest(const ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage)
    {
        mRemoveRequestRequest = request;
        mRemoveRequestErrorName = errorName;
        mRemoveRequestErrorMessage = errorMessage;
        Q_EMIT requestRemoved(request, errorName, errorMessage);
    }

    AccountPtr mObserveChannelsAccount;
    ConnectionPtr mObserveChannelsConnection;
    QList<ChannelPtr> mObserveChannelsChannels;
    ChannelDispatchOperationPtr mObserveChannelsDispatchOperation;
    QList<ChannelRequestPtr> mObserveChannelsRequestsSatisfied;
    AbstractClientObserver::ObserverInfo mObserveChannelsObserverInfo;

    QList<ChannelPtr> mAddDispatchOperationChannels;
    ChannelDispatchOperationPtr mAddDispatchOperationDispatchOperation;

    bool mBypassApproval;
    AccountPtr mHandleChannelsAccount;
    ConnectionPtr mHandleChannelsConnection;
    QList<ChannelPtr> mHandleChannelsChannels;
    QList<ChannelRequestPtr> mHandleChannelsRequestsSatisfied;
    QDateTime mHandleChannelsUserActionTime;
    AbstractClientHandler::HandlerInfo mHandleChannelsHandlerInfo;
    ChannelRequestPtr mAddRequestRequest;
    ChannelRequestPtr mRemoveRequestRequest;
    QString mRemoveRequestErrorName;
    QString mRemoveRequestErrorMessage;

Q_SIGNALS:
    void observeChannelsFinished();
    void addDispatchOperationFinished();
    void handleChannelsFinished();
    void claimFinished();
    void requestAdded(const Tp::ChannelRequestPtr &request);
    void requestRemoved(const Tp::ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage);
    void channelClosed();
};

class TestClient : public Test
{
    Q_OBJECT

public:
    TestClient(QObject *parent = 0)
        : Test(parent),
          mConn(0), mContactRepo(0),
          mText1ChanService(0), mText2ChanService(0), mCDO(0),
          mClaimFinished(false)
    { }

    void testObserveChannelsCommon(const AbstractClientPtr &clientObject,
            const QString &clientBusName, const QString &clientObjectPath);

protected Q_SLOTS:
    void expectSignalEmission();
    void onClaimFinished();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegister();
    void testCapabilities();
    void testObserveChannels();
    void testAddDispatchOperation();
    void testRequests();
    void testHandleChannels();

    void cleanup();
    void cleanupTestCase();

private:
    AccountManagerPtr mAM;
    AccountPtr mAccount;
    TestConnHelper *mConn;
    TpHandleRepoIface *mContactRepo;

    ExampleEchoChannel *mText1ChanService;
    ExampleEchoChannel *mText2ChanService;
    QString mText1ChanPath;
    QString mText2ChanPath;

    ClientRegistrarPtr mClientRegistrar;
    QString mChannelDispatcherBusName;
    QString mChannelRequestPath;
    QVariantMap mHandlerInfo;
    ChannelDispatchOperationAdaptor *mCDO;
    QString mCDOPath;
    AbstractClientHandler::Capabilities mClientCapabilities;
    AbstractClientPtr mClientObject1;
    QString mClientObject1BusName;
    QString mClientObject1Path;
    AbstractClientPtr mClientObject2;
    QString mClientObject2BusName;
    QString mClientObject2Path;
    uint mUserActionTime;

    bool mClaimFinished;
};

void TestClient::expectSignalEmission()
{
    mLoop->exit(0);
}

void TestClient::onClaimFinished()
{
    mClaimFinished = true;
}

void TestClient::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("client");
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
    mAccount = pacc->account();

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);

    mContactRepo = tp_base_connection_get_handles(TP_BASE_CONNECTION(mConn->service()),
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "someone@localhost", 0, 0);

    // create a Channel by magic, rather than doing D-Bus round-trips for it
    mText1ChanPath = mConn->objectPath() + QLatin1String("/TextChannel1");
    QByteArray chanPath(mText1ChanPath.toLatin1());
    mText1ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mText2ChanPath = mConn->objectPath() + QLatin1String("/TextChannel2");
    chanPath = mText2ChanPath.toLatin1();
    mText2ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConn->service(),
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mClientRegistrar = ClientRegistrar::create();

    QDBusConnection bus = mClientRegistrar->dbusConnection();

    // Fake ChannelRequest

    mChannelDispatcherBusName = TP_QT_IFACE_CHANNEL_DISPATCHER;
    mChannelRequestPath = QLatin1String("/org/freedesktop/Telepathy/ChannelRequest/Request1");

    QObject *request = new QObject(this);

    mUserActionTime = QDateTime::currentDateTime().toTime_t();
    ChannelRequestAdaptor *channelRequest  = new ChannelRequestAdaptor(QDBusObjectPath(mAccount->objectPath()),
            mUserActionTime,
            QString(),
            QualifiedPropertyValueMapList(),
            QStringList(),
            request);
    QVERIFY(bus.registerService(mChannelDispatcherBusName));
    QVERIFY(bus.registerObject(mChannelRequestPath, request));

    ObjectImmutablePropertiesMap channelRequestProperties;
    QVariantMap currentChannelRequestProperties;

    currentChannelRequestProperties.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Account"), QVariant::fromValue(channelRequest->Account()));
    currentChannelRequestProperties.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".UserActionTime"), channelRequest->UserActionTime());
    currentChannelRequestProperties.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".PreferredHandler"), channelRequest->PreferredHandler());
    currentChannelRequestProperties.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Requests"), QVariant::fromValue(channelRequest->Requests()));
    currentChannelRequestProperties.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Interfaces"), QVariant::fromValue(channelRequest->Interfaces()));
    channelRequestProperties[QDBusObjectPath(mChannelRequestPath)] = currentChannelRequestProperties;

    mHandlerInfo.insert(QLatin1String("request-properties"), QVariant::fromValue(channelRequestProperties));

    // Fake ChannelDispatchOperation

    mCDOPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatchOperation/Operation1");

    QObject *cdo = new QObject(this);

    // Initialize this here so we can actually set it in possibleHandlers
    mClientObject1BusName = QLatin1String("org.freedesktop.Telepathy.Client.foo");

    ChannelDetailsList channelDetailsList;
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath), QVariantMap() };
    channelDetailsList.append(channelDetails);

    mCDO = new ChannelDispatchOperationAdaptor(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()), channelDetailsList,
            QStringList() << mClientObject1BusName, cdo);
    QVERIFY(bus.registerObject(mCDOPath, cdo));
}

void TestClient::init()
{
    initImpl();
    mClaimFinished = false;
}

void TestClient::testRegister()
{
    // invalid client
    QVERIFY(!mClientRegistrar->registerClient(AbstractClientPtr(), QLatin1String("foo")));

    mClientCapabilities.setICEUDPNATTraversalToken();
    mClientCapabilities.setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
            QLatin1String("/audio/speex=true"));

    ChannelClassSpecList filters;
    filters.append(ChannelClassSpec::textChat());
    mClientObject1 = MyClient::create(filters, mClientCapabilities, false, true);
    MyClient *client = dynamic_cast<MyClient*>(mClientObject1.data());
    QVERIFY(!client->isRegistered());
    QVERIFY(mClientRegistrar->registerClient(mClientObject1, QLatin1String("foo")));
    QVERIFY(client->isRegistered());
    QVERIFY(mClientRegistrar->registeredClients().contains(mClientObject1));

    AbstractClientPtr clientObjectRedundant = MyClient::create(
            filters, mClientCapabilities, false, true);
    client = dynamic_cast<MyClient*>(clientObjectRedundant.data());
    QVERIFY(!client->isRegistered());
    // try to register using a name already registered and a different object, it should fail
    // and not report isRegistered
    QVERIFY(!mClientRegistrar->registerClient(clientObjectRedundant, QLatin1String("foo")));
    QVERIFY(!client->isRegistered());
    QVERIFY(!mClientRegistrar->registeredClients().contains(clientObjectRedundant));

    client = dynamic_cast<MyClient*>(mClientObject1.data());

    // no op - client already registered with same object and name
    QVERIFY(mClientRegistrar->registerClient(mClientObject1, QLatin1String("foo")));

    // unregister client
    QVERIFY(mClientRegistrar->unregisterClient(mClientObject1));
    QVERIFY(!client->isRegistered());

    // register again
    QVERIFY(mClientRegistrar->registerClient(mClientObject1, QLatin1String("foo")));
    QVERIFY(client->isRegistered());

    filters.clear();
    filters.append(ChannelClassSpec::streamedMediaCall());
    mClientObject2 = MyClient::create(filters, mClientCapabilities, true, true);
    QVERIFY(mClientRegistrar->registerClient(mClientObject2, QLatin1String("foo"), true));
    QVERIFY(mClientRegistrar->registeredClients().contains(mClientObject2));

    // no op - client already registered
    QVERIFY(mClientRegistrar->registerClient(mClientObject2, QLatin1String("foo"), true));

    QDBusConnection bus = mClientRegistrar->dbusConnection();
    QDBusConnectionInterface *busIface = bus.interface();
    QStringList registeredServicesNames = busIface->registeredServiceNames();
    QVERIFY(registeredServicesNames.filter(
                QRegExp(QLatin1String("^" "org.freedesktop.Telepathy.Client.foo"
                        ".([_A-Za-z][_A-Za-z0-9]*)"))).size() == 1);

    mClientObject1BusName = QLatin1String("org.freedesktop.Telepathy.Client.foo");
    mClientObject1Path = QLatin1String("/org/freedesktop/Telepathy/Client/foo");

    mClientObject2BusName = registeredServicesNames.filter(
            QRegExp(QLatin1String("org.freedesktop.Telepathy.Client.foo._*"))).first();
    mClientObject2Path = QString(QLatin1String("/%1")).arg(mClientObject2BusName);
    mClientObject2Path.replace(QLatin1String("."), QLatin1String("/"));
}

void TestClient::testCapabilities()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();
    QStringList normalizedClientCaps = mClientCapabilities.allTokens();
    normalizedClientCaps.sort();

    QStringList normalizedHandlerCaps;

    // object 1
    ClientHandlerInterface *handler1Iface = new ClientHandlerInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);

    QVERIFY(waitForProperty(handler1Iface->requestPropertyCapabilities(), &normalizedHandlerCaps));
    normalizedHandlerCaps.sort();
    QCOMPARE(normalizedHandlerCaps, normalizedClientCaps);

    // object 2
    ClientHandlerInterface *handler2Iface = new ClientHandlerInterface(bus,
            mClientObject2BusName, mClientObject2Path, this);

    QVERIFY(waitForProperty(handler2Iface->requestPropertyCapabilities(), &normalizedHandlerCaps));
    normalizedHandlerCaps.sort();
    QCOMPARE(normalizedHandlerCaps, normalizedClientCaps);
}

void TestClient::testRequests()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();
    ClientInterfaceRequestsInterface *handlerRequestsIface = new ClientInterfaceRequestsInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);

    MyClient *client = dynamic_cast<MyClient*>(mClientObject1.data());
    connect(client,
            SIGNAL(requestAdded(const Tp::ChannelRequestPtr &)),
            SLOT(expectSignalEmission()));
    handlerRequestsIface->AddRequest(QDBusObjectPath(mChannelRequestPath), QVariantMap());
    if (!client->mAddRequestRequest) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(client->mAddRequestRequest->objectPath(),
             mChannelRequestPath);

    connect(client,
            SIGNAL(requestRemoved(const Tp::ChannelRequestPtr &,
                                  const QString &,
                                  const QString &)),
            SLOT(expectSignalEmission()));
    handlerRequestsIface->RemoveRequest(QDBusObjectPath(mChannelRequestPath),
            QLatin1String(TP_QT_ERROR_NOT_AVAILABLE),
            QLatin1String("Not available"));
    if (!client->mRemoveRequestRequest) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(client->mRemoveRequestRequest->objectPath(),
             mChannelRequestPath);
    QCOMPARE(client->mRemoveRequestErrorName,
             QString(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE)));
    QCOMPARE(client->mRemoveRequestErrorMessage,
             QString(QLatin1String("Not available")));
}

void TestClient::testObserveChannelsCommon(const AbstractClientPtr &clientObject,
        const QString &clientBusName, const QString &clientObjectPath)
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();

    ClientObserverInterface *observeIface = new ClientObserverInterface(bus,
            clientBusName, clientObjectPath, this);
    MyClient *client = dynamic_cast<MyClient*>(clientObject.data());
    connect(client,
            SIGNAL(observeChannelsFinished()),
            SLOT(expectSignalEmission()));
    ChannelDetailsList channelDetailsList;
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath), QVariantMap() };
    channelDetailsList.append(channelDetails);

    QVariantMap observerInfo;
    ObjectImmutablePropertiesMap reqPropsMap;
    QVariantMap channelReqImmutableProps;
    channelReqImmutableProps.insert(
                TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Interface.DomainSpecific.IntegerProp"), 3);
    channelReqImmutableProps.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Account"),
            qVariantFromValue(QDBusObjectPath(mAccount->objectPath())));
    reqPropsMap.insert(QDBusObjectPath(mChannelRequestPath), channelReqImmutableProps);
    observerInfo.insert(QLatin1String("request-properties"), qVariantFromValue(reqPropsMap));
    observeIface->ObserveChannels(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            channelDetailsList,
            QDBusObjectPath("/"),
            ObjectPathList() << QDBusObjectPath(mChannelRequestPath),
            observerInfo);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client->mObserveChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client->mObserveChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client->mObserveChannelsChannels.first()->objectPath(), mText1ChanPath);
    QVERIFY(client->mObserveChannelsDispatchOperation.isNull());
    QCOMPARE(client->mObserveChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QCOMPARE(client->mObserveChannelsRequestsSatisfied.first()->immutableProperties().contains(
                TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Interface.DomainSpecific.IntegerProp")), true);
    QCOMPARE(qdbus_cast<int>(client->mObserveChannelsRequestsSatisfied.first()->immutableProperties().value(
                TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Interface.DomainSpecific.IntegerProp"))), 3);
}

void TestClient::testObserveChannels()
{
    testObserveChannelsCommon(mClientObject1,
            mClientObject1BusName, mClientObject1Path);
    testObserveChannelsCommon(mClientObject2,
            mClientObject2BusName, mClientObject2Path);
}

void TestClient::testAddDispatchOperation()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();

    ClientApproverInterface *approverIface = new ClientApproverInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);
    ClientHandlerInterface *handler1Iface = new ClientHandlerInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);
    MyClient *client = dynamic_cast<MyClient*>(mClientObject1.data());
    connect(client,
            SIGNAL(addDispatchOperationFinished()),
            SLOT(expectSignalEmission()));
    connect(client,
            SIGNAL(claimFinished()),
            SLOT(onClaimFinished()));

    QVariantMap dispatchOperationProperties;
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Connection"),
            QVariant::fromValue(QDBusObjectPath(mConn->objectPath())));
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Account"),
            QVariant::fromValue(QDBusObjectPath(mAccount->objectPath())));
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".PossibleHandlers"),
            QVariant::fromValue(ObjectPathList() << QDBusObjectPath(mClientObject1Path)
                << QDBusObjectPath(mClientObject2Path)));

    // Handler.HandledChannels should be empty here, CDO::claim(handler) will populate it on
    // success
    Tp::ObjectPathList handledChannels;
    QVERIFY(waitForProperty(handler1Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.isEmpty());

    approverIface->AddDispatchOperation(mCDO->Channels(), QDBusObjectPath(mCDOPath),
            dispatchOperationProperties);
    QCOMPARE(mLoop->exec(), 0);
    while (!mClaimFinished) {
        mLoop->processEvents();
    }

    QCOMPARE(client->mAddDispatchOperationChannels.first()->objectPath(), mText1ChanPath);
    QCOMPARE(client->mAddDispatchOperationDispatchOperation->objectPath(), mCDOPath);

    // Claim finished, Handler.HandledChannels should be populated now
    handledChannels.clear();
    QVERIFY(waitForProperty(handler1Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(!handledChannels.isEmpty());
    qSort(handledChannels);
    Tp::ObjectPathList expectedHandledChannels;
    Q_FOREACH (const ChannelDetails &details, mCDO->Channels()) {
        expectedHandledChannels << details.channel;
    }
    qSort(expectedHandledChannels);
    QCOMPARE(handledChannels, expectedHandledChannels);
}

void TestClient::testHandleChannels()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();

    // object 1
    ClientHandlerInterface *handler1Iface = new ClientHandlerInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);
    MyClient *client1 = dynamic_cast<MyClient*>(mClientObject1.data());
    connect(client1,
            SIGNAL(handleChannelsFinished()),
            SLOT(expectSignalEmission()));
    ChannelDetailsList channelDetailsList;
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath), QVariantMap() };
    channelDetailsList.append(channelDetails);

    handler1Iface->HandleChannels(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            channelDetailsList,
            ObjectPathList() << QDBusObjectPath(mChannelRequestPath),
            mUserActionTime,
            mHandlerInfo);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client1->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client1->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client1->mHandleChannelsChannels.first()->objectPath(), mText1ChanPath);
    QCOMPARE(client1->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QCOMPARE(client1->mHandleChannelsUserActionTime.toTime_t(), mUserActionTime);

    Tp::ObjectPathList handledChannels;
    QVERIFY(waitForProperty(handler1Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));

    // object 2
    ClientHandlerInterface *handler2Iface = new ClientHandlerInterface(bus,
            mClientObject2BusName, mClientObject2Path, this);
    MyClient *client2 = dynamic_cast<MyClient*>(mClientObject2.data());
    connect(client2,
            SIGNAL(handleChannelsFinished()),
            SLOT(expectSignalEmission()));
    channelDetailsList.clear();
    channelDetails.channel = QDBusObjectPath(mText2ChanPath);
    channelDetailsList.append(channelDetails);
    handler2Iface->HandleChannels(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            channelDetailsList,
            ObjectPathList() << QDBusObjectPath(mChannelRequestPath),
            mUserActionTime,
            mHandlerInfo);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client2->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client2->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client2->mHandleChannelsChannels.first()->objectPath(), mText2ChanPath);
    QCOMPARE(client2->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QCOMPARE(client2->mHandleChannelsUserActionTime.toTime_t(), mUserActionTime);

    QVERIFY(waitForProperty(handler1Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));

    QVERIFY(waitForProperty(handler2Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));

    // Handler.HandledChannels will now return all channels that are not invalidated/destroyed
    // even if the handler for such channels was already unregistered
    g_object_unref(mText1ChanService);
    connect(client1,
            SIGNAL(channelClosed()),
            SLOT(expectSignalEmission()));
    QCOMPARE(mLoop->exec(), 0);

    mClientRegistrar->unregisterClient(mClientObject1);
    QVERIFY(waitForProperty(handler2Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));

    g_object_unref(mText2ChanService);
    connect(client2,
            SIGNAL(channelClosed()),
            SLOT(expectSignalEmission()));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(waitForProperty(handler2Iface->requestPropertyHandledChannels(), &handledChannels));
    QVERIFY(handledChannels.isEmpty());
}

void TestClient::cleanup()
{
    cleanupImpl();
}

void TestClient::cleanupTestCase()
{
    if (mConn) {
        QCOMPARE(mConn->disconnect(), true);
        delete mConn;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestClient)
#include "_gen/client.cpp.moc.hpp"
