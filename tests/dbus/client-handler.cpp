#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/AbstractClientHandler>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/ClientHandlerInterface>
#include <TelepathyQt4/ClientInterfaceRequestsInterface>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingClientOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/contacts-conn.h>
#include <tests/lib/echo/chan.h>
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
    Q_PROPERTY(QualifiedPropertyValueMapList Requests READ Requests)
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

class MyHandler : public QObject, public AbstractClientHandler
{
    Q_OBJECT

public:
    static AbstractClientPtr create(const ChannelClassList &channelFilter,
            bool bypassApproval = false,
            bool wantsRequestNotification = false)
    {
        return AbstractClientPtr::dynamicCast(SharedPtr<MyHandler>(
                    new MyHandler(channelFilter,
                        bypassApproval, wantsRequestNotification)));
    }

    MyHandler(const ChannelClassList &channelFilter,
            bool bypassApproval = false,
            bool wantsRequestNotification = false)
        : AbstractClientHandler(channelFilter, wantsRequestNotification),
          mBypassApproval(bypassApproval)
    {
    }

    ~MyHandler()
    {
    }

    bool bypassApproval() const
    {
        return mBypassApproval;
    }

    void handleChannels(PendingClientOperation *operation,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const QVariantMap &handlerInfo)
    {
        mHandleChannelsAccount = account;
        mHandleChannelsConnection = connection;
        mHandleChannelsChannels = channels;
        mHandleChannelsRequestsSatisfied = requestsSatisfied;
        mHandleChannelsUserActionTime = userActionTime;
        mHandleChannelsHandlerInfo = handlerInfo;

        foreach (const ChannelPtr &channel, channels) {
            connect(channel.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *,
                                       const QString &, const QString &)),
                    SIGNAL(channelClosed()));
        }

        operation->setFinished();
        connect(operation,
                SIGNAL(finished(Tp::PendingOperation*)),
                SIGNAL(handleChannelsFinished()));
    }

    void addRequest(const ChannelRequestPtr &request)
    {
        mAddRequestRequest = request;
        emit requestAdded(request);
    }

    void removeRequest(const ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage)
    {
        mRemoveRequestRequest = request;
        mRemoveRequestErrorName = errorName;
        mRemoveRequestErrorMessage = errorMessage;
        emit requestRemoved(request, errorName, errorMessage);
    }

    bool mBypassApproval;
    AccountPtr mHandleChannelsAccount;
    ConnectionPtr mHandleChannelsConnection;
    QList<ChannelPtr> mHandleChannelsChannels;
    QList<ChannelRequestPtr> mHandleChannelsRequestsSatisfied;
    QDateTime mHandleChannelsUserActionTime;
    QVariantMap mHandleChannelsHandlerInfo;
    ChannelRequestPtr mAddRequestRequest;
    ChannelRequestPtr mRemoveRequestRequest;
    QString mRemoveRequestErrorName;
    QString mRemoveRequestErrorMessage;

Q_SIGNALS:
    void requestAdded(const Tp::ChannelRequestPtr &request);
    void requestRemoved(const Tp::ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage);
    void handleChannelsFinished();
    void channelClosed();
};

class TestClientHandler : public Test
{
    Q_OBJECT

public:
    TestClientHandler(QObject *parent = 0)
        : Test(parent),
          mConnService(0), mBaseConnService(0), mContactRepo(0),
          mText1ChanService(0)
    { }

protected Q_SLOTS:
    void expectSignalEmission();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegister();
    void testRequests();
    void testHandleChannels();

    void cleanup();
    void cleanupTestCase();

private:
    ContactsConnection *mConnService;
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
    QString mChannelRequestBusName;
    QString mChannelRequestPath;
    AbstractClientPtr mClientObject1;
    QString mClientObject1BusName;
    QString mClientObject1Path;
    AbstractClientPtr mClientObject2;
    QString mClientObject2BusName;
    QString mClientObject2Path;
    uint mUserActionTime;
};

void TestClientHandler::expectSignalEmission()
{
    mLoop->exit(0);
}

void TestClientHandler::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("client-handler");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mAM = AccountManager::create();
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVariantMap parameters;
    parameters["account"] = "foobar";
    PendingAccount *pacc = mAM->createAccount("foo",
            "bar", "foobar", parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pacc->account());
    mAccount = pacc->account();

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = CONTACTS_CONNECTION(g_object_new(
            CONTACTS_TYPE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            0));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QString::fromAscii(name);
    mConnPath = QString::fromAscii(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
             static_cast<uint>(Connection::StatusConnected));

    // create a Channel by magic, rather than doing D-Bus round-trips for it

    mContactRepo = tp_base_connection_get_handles(mBaseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "someone@localhost", 0, 0);

    mText1ChanPath = mConnPath + QLatin1String("/TextChannel1");
    QByteArray chanPath(mText1ChanPath.toAscii());
    mText1ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    mText2ChanPath = mConnPath + QLatin1String("/TextChannel2");
    chanPath = mText2ChanPath.toAscii();
    mText2ChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    tp_handle_unref(mContactRepo, handle);

    mClientRegistrar = ClientRegistrar::create("foo");

    QDBusConnection bus = mClientRegistrar->dbusConnection();
    mChannelRequestBusName = "org.freedesktop.Telepathy.ChannelDispatcher";
    mChannelRequestPath = "/org/freedesktop/Telepathy/ChannelRequest/Request1";
    QObject *request = new QObject(this);
    mUserActionTime = QDateTime::currentDateTime().toTime_t();
    new ChannelRequestAdaptor(QDBusObjectPath(mAccount->objectPath()),
            mUserActionTime,
            QString(),
            QualifiedPropertyValueMapList(),
            QStringList(),
            request);
    QVERIFY(bus.registerService(mChannelRequestBusName));
    QVERIFY(bus.registerObject(mChannelRequestPath, request));
}

void TestClientHandler::init()
{
    initImpl();
}

void TestClientHandler::testRegister()
{
    // invalid client
    QVERIFY(!mClientRegistrar->registerClient(AbstractClientPtr()));

    ChannelClassList filters;
    QMap<QString, QDBusVariant> filter;
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                  QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                  QDBusVariant(Tp::HandleTypeContact));
    filters.append(filter);
    mClientObject1 = MyHandler::create(filters, false, true);
    QVERIFY(mClientRegistrar->registerClient(mClientObject1));
    QVERIFY(mClientRegistrar->registeredClients().contains(mClientObject1));

    // no op - client already registered
    QVERIFY(mClientRegistrar->registerClient(mClientObject1));

    filters.clear();
    filter.clear();
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                  QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA));
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                  QDBusVariant(Tp::HandleTypeContact));
    filters.append(filter);
    mClientObject2 = MyHandler::create(filters, true, true);
    QVERIFY(mClientRegistrar->registerClient(mClientObject2, true));
    QVERIFY(mClientRegistrar->registeredClients().contains(mClientObject2));

    // no op - client already registered
    QVERIFY(mClientRegistrar->registerClient(mClientObject2, true));

    QDBusConnection bus = mClientRegistrar->dbusConnection();
    QDBusConnectionInterface *busIface = bus.interface();
    QStringList registeredServicesNames = busIface->registeredServiceNames();
    QVERIFY(registeredServicesNames.filter(
                QRegExp("^" "org.freedesktop.Telepathy.Client.foo"
                        ".([_A-Za-z][_A-Za-z0-9]*)")).size() == 1);

    mClientObject1BusName = "org.freedesktop.Telepathy.Client.foo";
    mClientObject1Path = "/org/freedesktop/Telepathy/Client/foo";

    mClientObject2BusName = registeredServicesNames.filter(
            QRegExp("org.freedesktop.Telepathy.Client.foo._*")).first();
    mClientObject2Path = QString("/%1").arg(mClientObject2BusName);
    mClientObject2Path.replace('.', '/');
}

void TestClientHandler::testRequests()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();
    ClientInterfaceRequestsInterface *handlerRequestsIface = new ClientInterfaceRequestsInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);

    MyHandler *handler = dynamic_cast<MyHandler*>(mClientObject1.data());
    connect(handler,
            SIGNAL(requestAdded(const Tp::ChannelRequestPtr &)),
            SLOT(expectSignalEmission()));
    handlerRequestsIface->AddRequest(QDBusObjectPath(mChannelRequestPath), QVariantMap());
    if (!handler->mAddRequestRequest) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(handler->mAddRequestRequest->objectPath(),
             mChannelRequestPath);

    connect(handler,
            SIGNAL(requestRemoved(const Tp::ChannelRequestPtr &,
                                  const QString &,
                                  const QString &)),
            SLOT(expectSignalEmission()));
    handlerRequestsIface->RemoveRequest(QDBusObjectPath(mChannelRequestPath),
            TELEPATHY_ERROR_NOT_AVAILABLE, "Not available");
    if (!handler->mRemoveRequestRequest) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(handler->mRemoveRequestRequest->objectPath(),
             mChannelRequestPath);
    QCOMPARE(handler->mRemoveRequestErrorName,
             QString(TELEPATHY_ERROR_NOT_AVAILABLE));
    QCOMPARE(handler->mRemoveRequestErrorMessage,
             QString("Not available"));
}

void TestClientHandler::testHandleChannels()
{
    QDBusConnection bus = mClientRegistrar->dbusConnection();

    // object 1
    ClientHandlerInterface *handler1Iface = new ClientHandlerInterface(bus,
            mClientObject1BusName, mClientObject1Path, this);
    MyHandler *handler1 = dynamic_cast<MyHandler*>(mClientObject1.data());
    connect(handler1,
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
            QVariantMap());
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(handler1->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(handler1->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(handler1->mHandleChannelsChannels.first()->objectPath(), mText1ChanPath);
    QCOMPARE(handler1->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QCOMPARE(handler1->mHandleChannelsUserActionTime.toTime_t(), mUserActionTime);

    Tp::ObjectPathList handledChannels = handler1Iface->HandledChannels();
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));

    // object 2
    ClientHandlerInterface *handler2Iface = new ClientHandlerInterface(bus,
            mClientObject2BusName, mClientObject2Path, this);
    MyHandler *handler2 = dynamic_cast<MyHandler*>(mClientObject2.data());
    connect(handler2,
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

    QCOMPARE(handler2->mHandleChannelsAccount->objectPath(), mAccount->objectPath());
    QCOMPARE(handler2->mHandleChannelsConnection->objectPath(), mConn->objectPath());
    QCOMPARE(handler2->mHandleChannelsChannels.first()->objectPath(), mText2ChanPath);
    QCOMPARE(handler2->mHandleChannelsRequestsSatisfied.first()->objectPath(), mChannelRequestPath);
    QCOMPARE(handler2->mHandleChannelsUserActionTime.toTime_t(), mUserActionTime);

    handledChannels = handler1Iface->HandledChannels();
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));
    handledChannels = handler2Iface->HandledChannels();
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText1ChanPath)));
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));

    mClientRegistrar->unregisterClient(mClientObject1);
    handledChannels = handler2Iface->HandledChannels();
    QVERIFY(handledChannels.contains(QDBusObjectPath(mText2ChanPath)));

    g_object_unref(mText2ChanService);
    connect(handler2,
            SIGNAL(channelClosed()),
            SLOT(expectSignalEmission()));
    QCOMPARE(mLoop->exec(), 0);
    handledChannels = handler2Iface->HandledChannels();
    QVERIFY(handledChannels.isEmpty());
}

void TestClientHandler::cleanup()
{
    cleanupImpl();
}

void TestClientHandler::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestClientHandler)
#include "_gen/client-handler.cpp.moc.hpp"
