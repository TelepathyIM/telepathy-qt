#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/AbstractClientObserver>
#include <TelepathyQt/Channel>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelDispatchOperation>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/ClientHandlerInterface>
#include <TelepathyQt/ClientInterfaceRequestsInterface>
#include <TelepathyQt/ClientObserverInterface>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactSearchChannel>
#include <TelepathyQt/Debug>
#include <TelepathyQt/FileTransferChannel>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/IncomingStreamTubeChannel>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/OutgoingStreamTubeChannel>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/StreamedMediaChannel>
#include <TelepathyQt/StreamTubeChannel>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo/chan.h>
#include <tests/lib/test.h>

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
    void requestAdded(const Tp::ChannelRequestPtr &request);
    void requestRemoved(const Tp::ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage);
    void channelClosed();
};

class TestClientFactories : public Test
{
    Q_OBJECT

public:
    TestClientFactories(QObject *parent = 0)
        : Test(parent),
          mConnService(0), mBaseConnService(0), mContactRepo(0),
          mText1ChanService(0)
    { }

    void testObserveChannelsCommon(const AbstractClientPtr &clientObject,
            const QString &clientBusName, const QString &clientObjectPath);

protected Q_SLOTS:
    void expectSignalEmission();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testFactoryAccess();
    void testRegister();
    void testCapabilities();
    void testObserveChannels();
    void testAddDispatchOperation();
    void testRequests();
    void testHandleChannels();
    void testChannelFactoryAccessors();

    void cleanup();
    void cleanupTestCase();

private:
    TpTestsContactsConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;
    ExampleEchoChannel *mText1ChanService;
    ExampleEchoChannel *mText2ChanService;

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    ConnectionPtr mConn;
    QString mText1ChanPath;
    QString mText2ChanPath;
    QString mConnName;
    QString mConnPath;

    ClientRegistrarPtr mClientRegistrar;
    QString mChannelDispatcherBusName;
    QString mChannelRequestPath;
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
};

void TestClientFactories::expectSignalEmission()
{
    mLoop->exit(0);
}

void TestClientFactories::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("client-factories");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    QDBusConnection bus = QDBusConnection::sessionBus();

    ChannelFactoryPtr chanFact = ChannelFactory::create(bus);

    chanFact->addFeaturesForTextChats(TextChannel::FeatureChatState |
            TextChannel::FeatureMessageQueue);
    chanFact->addCommonFeatures(Channel::FeatureCore);

    QCOMPARE(chanFact->commonFeatures().size(), 1);

    QCOMPARE(chanFact->featuresForTextChats().size(), 3);
    QVERIFY(chanFact->featuresForTextChats().contains(TextChannel::FeatureMessageQueue));
    QVERIFY(chanFact->featuresForTextChats().contains(Channel::FeatureCore));
    QVERIFY(!chanFact->featuresForTextChats().contains(TextChannel::FeatureMessageSentSignal));

    mAM = AccountManager::create(AccountFactory::create(bus, Account::FeatureCore),
            ConnectionFactory::create(bus,
                Connection::FeatureCore | Connection::FeatureSimplePresence),
            chanFact);
    PendingReady *amReadyOp = mAM->becomeReady();
    QVERIFY(amReadyOp != NULL);
    QVERIFY(connect(amReadyOp,
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = ConnectionPtr::qObjectCast(mAccount->connectionFactory()->proxy(mConnName, mConnPath,
                ChannelFactory::create(bus), ContactFactory::create())->proxy());
    QCOMPARE(mConn->isReady(), false);

    PendingReady *mConnReady = mConn->lowlevel()->requestConnect();
    QVERIFY(mConnReady != NULL);
    QVERIFY(connect(mConnReady,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(ConnectionStatusConnected));

    // create a Channel by magic, rather than doing D-Bus round-trips for it

    mContactRepo = tp_base_connection_get_handles(mBaseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "someone@localhost", 0, 0);

    mText1ChanPath = mConnPath + QLatin1String("/TextChannel1");
    QByteArray chanPath(mText1ChanPath.toLatin1());
    mText1ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mText2ChanPath = mConnPath + QLatin1String("/TextChannel2");
    chanPath = mText2ChanPath.toLatin1();
    mText2ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mClientRegistrar = ClientRegistrar::create(mAM);

    // Fake ChannelRequest

    mChannelDispatcherBusName = TP_QT_IFACE_CHANNEL_DISPATCHER;
    mChannelRequestPath = QLatin1String("/org/freedesktop/Telepathy/ChannelRequest/Request1");

    QObject *request = new QObject(this);

    mUserActionTime = QDateTime::currentDateTime().toTime_t();
    new ChannelRequestAdaptor(QDBusObjectPath(mAccount->objectPath()),
            mUserActionTime,
            QString(),
            QualifiedPropertyValueMapList(),
            QStringList(),
            request);
    QVERIFY(bus.registerService(mChannelDispatcherBusName));
    QVERIFY(bus.registerObject(mChannelRequestPath, request));

    // Fake ChannelDispatchOperation

    mCDOPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatchOperation/Operation1");

    QObject *cdo = new QObject(this);

    // Initialize this here so we can actually set it in possibleHandlers
    mClientObject1Path = QLatin1String("/org/freedesktop/Telepathy/Client/foo");

    ChannelDetailsList channelDetailsList;
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath),
        ChannelClassSpec::textChat().allProperties() };
    channelDetailsList.append(channelDetails);

    mCDO = new ChannelDispatchOperationAdaptor(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()), channelDetailsList,
            QStringList() << mClientObject1BusName, cdo);
    QVERIFY(bus.registerObject(mCDOPath, cdo));
}

void TestClientFactories::init()
{
    initImpl();
}

void TestClientFactories::testFactoryAccess()
{
    AccountFactoryConstPtr accFact = mClientRegistrar->accountFactory();
    QVERIFY(!accFact.isNull());
    QCOMPARE(accFact.data(), mAM->accountFactory().data());

    QCOMPARE(accFact->features(), Features(Account::FeatureCore));

    ConnectionFactoryConstPtr connFact = mClientRegistrar->connectionFactory();
    QVERIFY(!connFact.isNull());
    QCOMPARE(connFact.data(), mAM->connectionFactory().data());

    QCOMPARE(connFact->features(), Connection::FeatureCore | Connection::FeatureSimplePresence);

    ChannelFactoryConstPtr chanFact = mClientRegistrar->channelFactory();
    QVERIFY(!chanFact.isNull());
    QCOMPARE(chanFact.data(), mAM->channelFactory().data());

    ContactFactoryConstPtr contactFact = mClientRegistrar->contactFactory();
    QVERIFY(!contactFact.isNull());
    QCOMPARE(contactFact.data(), mAM->contactFactory().data());
}

void TestClientFactories::testRegister()
{
    // invalid client
    QVERIFY(!mClientRegistrar->registerClient(AbstractClientPtr(), QLatin1String("foo")));

    mClientCapabilities.setICEUDPNATTraversalToken();
    mClientCapabilities.setAudioCodecToken(QLatin1String("speex"));

    ChannelClassSpecList filters;
    filters.append(ChannelClassSpec::textChat());

    mClientObject1 = MyClient::create(filters, mClientCapabilities, false, true);
    QVERIFY(mClientRegistrar->registerClient(mClientObject1, QLatin1String("foo")));
    QVERIFY(mClientRegistrar->registeredClients().contains(mClientObject1));

    // no op - client already registered
    QVERIFY(mClientRegistrar->registerClient(mClientObject1, QLatin1String("foo")));

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

void TestClientFactories::testCapabilities()
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

void TestClientFactories::testRequests()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();
    ClientInterfaceRequestsInterface *handlerRequestsIface = new ClientInterfaceRequestsInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);

    MyClient *client = dynamic_cast<MyClient*>(mClientObject1.data());
    connect(client,
            SIGNAL(requestAdded(const Tp::ChannelRequestPtr &)),
            SLOT(expectSignalEmission()));

    QVariantMap requestProps;
    requestProps.insert(TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".Account"),
            QVariant::fromValue(QDBusObjectPath(mAccount->objectPath())));
    requestProps.insert(TP_QT_IFACE_CHANNEL_REQUEST +
                QLatin1String(".Interface.DomainSpecific.IntegerProp"),
            QVariant::fromValue(3));

    handlerRequestsIface->AddRequest(QDBusObjectPath(mChannelRequestPath), requestProps);

    if (!client->mAddRequestRequest) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(client->mAddRequestRequest->objectPath(),
             mChannelRequestPath);
    QCOMPARE(client->mAddRequestRequest->account().data(),
             mAccount.data());

    QVERIFY(client->mAddRequestRequest->immutableProperties().contains(
                TP_QT_IFACE_CHANNEL_REQUEST +
                    QLatin1String(".Interface.DomainSpecific.IntegerProp")));
    QCOMPARE(qdbus_cast<int>(client->mAddRequestRequest->immutableProperties().value(
                    TP_QT_IFACE_CHANNEL_REQUEST +
                        QLatin1String(".Interface.DomainSpecific.IntegerProp"))),
            3);

    QVERIFY(connect(client->mAddRequestRequest->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(client->mAddRequestRequest->immutableProperties().contains(
                TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".UserActionTime")));
    QCOMPARE(qdbus_cast<uint>(client->mAddRequestRequest->immutableProperties().value(
                    TP_QT_IFACE_CHANNEL_REQUEST + QLatin1String(".UserActionTime"))),
            mUserActionTime);

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
//    QCOMPARE(client->mRemoveRequestRequest->account().data(),
//             mAccount.data());
    QCOMPARE(client->mRemoveRequestErrorName,
             QString(QLatin1String(TP_QT_ERROR_NOT_AVAILABLE)));
    QCOMPARE(client->mRemoveRequestErrorMessage,
             QString(QLatin1String("Not available")));
}

void TestClientFactories::testObserveChannelsCommon(const AbstractClientPtr &clientObject,
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
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath),
        ChannelClassSpec::textChat().allProperties() };
    channelDetailsList.append(channelDetails);
    observeIface->ObserveChannels(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            channelDetailsList,
            QDBusObjectPath(mCDOPath),
            ObjectPathList() << QDBusObjectPath(mChannelRequestPath),
            QVariantMap());
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client->mObserveChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client->mObserveChannelsAccount.data(), mAccount.data());
    QVERIFY(client->mObserveChannelsAccount->isReady(Account::FeatureCore));

    QCOMPARE(client->mObserveChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client->mObserveChannelsConnection.data(), mConn.data());
    QVERIFY(client->mObserveChannelsConnection->isReady(
                Connection::FeatureCore | Connection::FeatureSimplePresence));

    QCOMPARE(client->mObserveChannelsChannels.size(), 1);
    QCOMPARE(client->mObserveChannelsChannels.first()->objectPath(), mText1ChanPath);

    QVERIFY(!client->mObserveChannelsDispatchOperation.isNull());
    QCOMPARE(client->mObserveChannelsDispatchOperation->account().data(), mAccount.data());
    QCOMPARE(client->mObserveChannelsDispatchOperation->connection().data(), mConn.data());

    QCOMPARE(client->mObserveChannelsRequestsSatisfied.size(), 1);
    QCOMPARE(client->mObserveChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QVERIFY(client->mObserveChannelsRequestsSatisfied.first()->isReady());
    QCOMPARE(client->mObserveChannelsRequestsSatisfied.first()->account().data(),
             mAccount.data());
}

void TestClientFactories::testObserveChannels()
{
    testObserveChannelsCommon(mClientObject1,
            mClientObject1BusName, mClientObject1Path);
    testObserveChannelsCommon(mClientObject2,
            mClientObject2BusName, mClientObject2Path);
}

void TestClientFactories::testAddDispatchOperation()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();

    ClientApproverInterface *approverIface = new ClientApproverInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);
    MyClient *client = dynamic_cast<MyClient*>(mClientObject1.data());
    connect(client,
            SIGNAL(addDispatchOperationFinished()),
            SLOT(expectSignalEmission()));

    QVariantMap dispatchOperationProperties;
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Connection"),
            QVariant::fromValue(mCDO->Connection()));
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Account"),
            QVariant::fromValue(mCDO->Account()));
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".PossibleHandlers"),
            QVariant::fromValue(mCDO->PossibleHandlers()));
    dispatchOperationProperties.insert(
            TP_QT_IFACE_CHANNEL_DISPATCH_OPERATION + QLatin1String(".Interfaces"),
            QVariant::fromValue(mCDO->Interfaces()));

    approverIface->AddDispatchOperation(mCDO->Channels(), QDBusObjectPath(mCDOPath),
            dispatchOperationProperties);
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client->mAddDispatchOperationChannels.first()->objectPath(), mText1ChanPath);

    QCOMPARE(client->mAddDispatchOperationChannels.first()->connection().data(), mConn.data());
    QVERIFY(client->mAddDispatchOperationChannels.first()->connection()->isReady(
                Connection::FeatureCore | Connection::FeatureSimplePresence));

    QCOMPARE(client->mAddDispatchOperationDispatchOperation->channels().first().data(),
            client->mAddDispatchOperationChannels.first().data());

    QCOMPARE(client->mAddDispatchOperationDispatchOperation->objectPath(), mCDOPath);
    QVERIFY(client->mAddDispatchOperationDispatchOperation->isReady());
    QCOMPARE(client->mAddDispatchOperationDispatchOperation->account().data(), mAccount.data());
    QCOMPARE(client->mAddDispatchOperationDispatchOperation->connection().data(), mConn.data());

    QCOMPARE(client->mAddDispatchOperationDispatchOperation->possibleHandlers().size(), 1);
    QCOMPARE(client->mAddDispatchOperationDispatchOperation->possibleHandlers(),
            mCDO->PossibleHandlers());
}

void TestClientFactories::testHandleChannels()
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
    ChannelDetails channelDetails = { QDBusObjectPath(mText1ChanPath),
        ChannelClassSpec::textChat().allProperties() };
    channelDetailsList.append(channelDetails);
    handler1Iface->HandleChannels(QDBusObjectPath(mAccount->objectPath()),
            QDBusObjectPath(mConn->objectPath()),
            channelDetailsList,
            ObjectPathList() << QDBusObjectPath(mChannelRequestPath),
            mUserActionTime,
            QVariantMap());
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client1->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client1->mHandleChannelsAccount.data(), mAccount.data());
    QVERIFY(client1->mHandleChannelsAccount->isReady());

    QCOMPARE(client1->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client1->mHandleChannelsConnection.data(), mConn.data());
    QVERIFY(client1->mHandleChannelsConnection->isReady(
                Connection::FeatureCore | Connection::FeatureSimplePresence));

    QCOMPARE(client1->mHandleChannelsChannels.first()->objectPath(), mText1ChanPath);

    TextChannelPtr textChan = TextChannelPtr::qObjectCast(client1->mHandleChannelsChannels.first());

    QVERIFY(!textChan.isNull());

    QVERIFY(textChan->isReady());
    QVERIFY(textChan->isReady(Channel::FeatureCore));
    QVERIFY(textChan->isReady(TextChannel::FeatureMessageQueue));
    QVERIFY(textChan->isReady(TextChannel::FeatureChatState));

    QCOMPARE(client1->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QVERIFY(client1->mHandleChannelsRequestsSatisfied.first()->isReady());
    QCOMPARE(client1->mHandleChannelsRequestsSatisfied.first()->account().data(),
             mAccount.data());

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
            QVariantMap());
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(client2->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(client2->mHandleChannelsAccount.data(), mAccount.data());
    QVERIFY(client2->mHandleChannelsAccount->isReady());

    QCOMPARE(client2->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(client2->mHandleChannelsConnection.data(), mConn.data());
    QVERIFY(client2->mHandleChannelsConnection->isReady(
                Connection::FeatureCore | Connection::FeatureSimplePresence));

    QCOMPARE(client2->mHandleChannelsChannels.first()->objectPath(), mText2ChanPath);

    QCOMPARE(client2->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QVERIFY(client2->mHandleChannelsRequestsSatisfied.first()->isReady());
    QCOMPARE(client2->mHandleChannelsRequestsSatisfied.first()->account().data(),
             mAccount.data());

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

void TestClientFactories::testChannelFactoryAccessors()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    ChannelFactoryPtr chanFact = ChannelFactory::create(bus);

    QCOMPARE(chanFact->featuresForTextChats(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChat()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedTextChat()), Features());

    QCOMPARE(chanFact->featuresForTextChatrooms(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChatroom()), Features());

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), Features());

    QCOMPARE(chanFact->featuresForRoomLists(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::roomList()), Features());

    QCOMPARE(chanFact->featuresForOutgoingFileTransfers(), Features());
    QCOMPARE(chanFact->featuresForIncomingFileTransfers(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingFileTransfer()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingFileTransfer()), Features());

    QCOMPARE(chanFact->featuresForOutgoingStreamTubes(), Features());
    QCOMPARE(chanFact->featuresForIncomingStreamTubes(), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingStreamTube()), Features());
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingStreamTube()), Features());

    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::contactSearch()), Features());
    QCOMPARE(chanFact->featuresForContactSearches(), Features());

    Features commonFeatures;
    commonFeatures.insert(Channel::FeatureCore);
    chanFact->addCommonFeatures(commonFeatures);
    QCOMPARE(chanFact->featuresForTextChats(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChat()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedTextChat()), commonFeatures);

    QCOMPARE(chanFact->featuresForTextChatrooms(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChatroom()), commonFeatures);

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), commonFeatures);

    QCOMPARE(chanFact->featuresForRoomLists(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::roomList()), commonFeatures);

    QCOMPARE(chanFact->featuresForOutgoingFileTransfers(), commonFeatures);
    QCOMPARE(chanFact->featuresForIncomingFileTransfers(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingFileTransfer()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingFileTransfer()), commonFeatures);

    QCOMPARE(chanFact->featuresForOutgoingStreamTubes(), commonFeatures);
    QCOMPARE(chanFact->featuresForIncomingStreamTubes(), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingStreamTube()), commonFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingStreamTube()), commonFeatures);

    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::contactSearch()), commonFeatures);
    QCOMPARE(chanFact->featuresForContactSearches(), commonFeatures);

    Features textChatFeatures;
    textChatFeatures.insert(TextChannel::FeatureCore);
    textChatFeatures.insert(TextChannel::FeatureMessageQueue);
    chanFact->addFeaturesForTextChats(textChatFeatures);
    textChatFeatures |= commonFeatures;

    Features textChatroomFeatures;
    textChatroomFeatures.insert(TextChannel::FeatureCore);
    textChatroomFeatures.insert(TextChannel::FeatureMessageCapabilities);
    chanFact->addFeaturesForTextChatrooms(textChatroomFeatures);
    textChatroomFeatures |= commonFeatures;

    Features streamedMediaFeatures;
    streamedMediaFeatures.insert(StreamedMediaChannel::FeatureStreams);
    chanFact->addFeaturesForStreamedMediaCalls(streamedMediaFeatures);
    streamedMediaFeatures |= commonFeatures;

    // RoomListChannel has no feature, let's use FeatureConferenceInitialInviteeContacts just for
    // testing purposes
    Features roomListFeatures;
    roomListFeatures.insert(Channel::FeatureConferenceInitialInviteeContacts);
    chanFact->addFeaturesForRoomLists(roomListFeatures);
    roomListFeatures |= commonFeatures;

    Features outFtFeatures;
    outFtFeatures.insert(FileTransferChannel::FeatureCore);
    outFtFeatures.insert(OutgoingFileTransferChannel::FeatureCore);
    chanFact->addFeaturesForOutgoingFileTransfers(outFtFeatures);
    outFtFeatures |= commonFeatures;
    Features inFtFeatures;
    inFtFeatures.insert(FileTransferChannel::FeatureCore);
    inFtFeatures.insert(IncomingFileTransferChannel::FeatureCore);
    chanFact->addFeaturesForIncomingFileTransfers(inFtFeatures);
    inFtFeatures |= commonFeatures;

    Features outStubeFeatures;
    outStubeFeatures.insert(StreamTubeChannel::FeatureCore);
    outStubeFeatures.insert(OutgoingStreamTubeChannel::FeatureCore);
    chanFact->addFeaturesForOutgoingStreamTubes(outStubeFeatures);
    outStubeFeatures |= commonFeatures;
    Features inStubeFeatures = commonFeatures;
    outStubeFeatures.insert(StreamTubeChannel::FeatureCore);
    outStubeFeatures.insert(IncomingStreamTubeChannel::FeatureCore);
    chanFact->addFeaturesForIncomingStreamTubes(inStubeFeatures);
    inStubeFeatures |= commonFeatures;

    Features contactSearchFeatures;
    contactSearchFeatures.insert(ContactSearchChannel::FeatureCore);
    chanFact->addFeaturesForContactSearches(contactSearchFeatures);
    contactSearchFeatures |= commonFeatures;

    QCOMPARE(chanFact->featuresForTextChats(), textChatFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChat()), textChatFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedTextChat()), textChatFeatures);

    QCOMPARE(chanFact->featuresForTextChatrooms(), textChatroomFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::textChatroom()), textChatroomFeatures);

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), streamedMediaFeatures);

    QCOMPARE(chanFact->featuresForRoomLists(), roomListFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::roomList()), roomListFeatures);

    QCOMPARE(chanFact->featuresForOutgoingFileTransfers(), outFtFeatures);
    QCOMPARE(chanFact->featuresForIncomingFileTransfers(), inFtFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingFileTransfer()), outFtFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingFileTransfer()), inFtFeatures);

    QCOMPARE(chanFact->featuresForOutgoingStreamTubes(), outStubeFeatures);
    QCOMPARE(chanFact->featuresForIncomingStreamTubes(), inStubeFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::outgoingStreamTube()), outStubeFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::incomingStreamTube()), inStubeFeatures);

    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::contactSearch()), contactSearchFeatures);
    QCOMPARE(chanFact->featuresForContactSearches(), contactSearchFeatures);

    Features streamedMediaAudioFeatures;
    streamedMediaAudioFeatures.insert(StreamedMediaChannel::FeatureStreams);
    chanFact->addFeaturesFor(ChannelClassSpec::streamedMediaAudioCall(),
            streamedMediaAudioFeatures);
    streamedMediaAudioFeatures |= streamedMediaFeatures;

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), streamedMediaFeatures);

    Features unnamedStreamedMediaAudioFeatures;
    unnamedStreamedMediaAudioFeatures.insert(StreamedMediaChannel::FeatureLocalHoldState);
    chanFact->addFeaturesFor(ChannelClassSpec::unnamedStreamedMediaAudioCall(),
            unnamedStreamedMediaAudioFeatures);
    unnamedStreamedMediaAudioFeatures |= streamedMediaFeatures;

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), unnamedStreamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), unnamedStreamedMediaAudioFeatures);

    QVariantMap otherProps;
    otherProps.insert(QLatin1String("ping"), QLatin1String("pong"));
    Features specificUnnamedStreamedMediaFeatures;
    specificUnnamedStreamedMediaFeatures.insert(Feature(QLatin1String("TestClass"), 1234));
    chanFact->addFeaturesFor(ChannelClassSpec::unnamedStreamedMediaCall(otherProps),
            specificUnnamedStreamedMediaFeatures);
    specificUnnamedStreamedMediaFeatures |= streamedMediaFeatures;

    QCOMPARE(chanFact->featuresForStreamedMediaCalls(), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaAudioCall()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::streamedMediaVideoCallWithAudio()), streamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaCall(otherProps)), specificUnnamedStreamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaAudioCall()), unnamedStreamedMediaAudioFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCall()), streamedMediaFeatures);
    QCOMPARE(chanFact->featuresFor(ChannelClassSpec::unnamedStreamedMediaVideoCallWithAudio()), unnamedStreamedMediaAudioFeatures);
}

void TestClientFactories::cleanup()
{
    cleanupImpl();
}

void TestClientFactories::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestClientFactories)
#include "_gen/client-factories.cpp.moc.hpp"
