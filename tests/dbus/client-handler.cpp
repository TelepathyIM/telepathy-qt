#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AbstractClientHandler>
#include <TelepathyQt4/Channel>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingClientOperation>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/test.h>

using namespace Tp;

class MyHandler : public AbstractClientHandler
{
public:
    MyHandler(const ChannelClassList &channelFilter,
            bool bypassApproval = false,
            bool listenRequests = false)
        : AbstractClientHandler(channelFilter, listenRequests),
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

    QList<ChannelPtr> handledChannels() const
    {
        return mHandledChannels;
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
        mHandledChannels.append(channels);
        operation->setFinished();
    }

    void addRequest(const ChannelRequestPtr &request)
    {
        mAddRequestRequest = request;
    }

    void removeRequest(const ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage)
    {
        mRemoveRequestRequest = request;
        mRemoveRequestErrorName = errorName;
        mRemoveRequestErrorMessage = errorMessage;
    }

    bool mBypassApproval;
    AccountPtr mHandleChannelsAccount;
    ConnectionPtr mHandleChannelsConnection;
    QList<ChannelPtr> mHandleChannelsChannels;
    QList<ChannelRequestPtr> mHandleChannelsRequestsSatisfied;
    QDateTime mHandleChannelsUserActionTime;
    QVariantMap mHandleChannelsHandlerInfo;
    QList<ChannelPtr> mHandledChannels;
    ChannelRequestPtr mAddRequestRequest;
    ChannelRequestPtr mRemoveRequestRequest;
    QString mRemoveRequestErrorName;
    QString mRemoveRequestErrorMessage;
};

class TestClientHandler : public Test
{
    Q_OBJECT

public:
    TestClientHandler(QObject *parent = 0)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRegister();

    void cleanup();
    void cleanupTestCase();

private:
    ClientRegistrarPtr mClientRegistrar;
};

void TestClientHandler::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("client-handler");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mClientRegistrar = ClientRegistrar::create("foo");
}

void TestClientHandler::init()
{
    initImpl();
}

void TestClientHandler::testRegister()
{
    ChannelClassList filters;
    QMap<QString, QDBusVariant> filter;
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                  QDBusVariant(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    filter.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                  QDBusVariant(Tp::HandleTypeContact));
    filters.append(filter);
    SharedPtr<MyHandler> myHandler = SharedPtr<MyHandler>(
            new MyHandler(filters));
    mClientRegistrar->registerClient(AbstractClientPtr::dynamicCast(myHandler));

    QDBusConnection bus = mClientRegistrar->dbusConnection();
    QDBusConnectionInterface *iface = bus.interface();
    QStringList registeredServicesNames = iface->registeredServiceNames();
    QVERIFY(registeredServicesNames.contains(
                "org.freedesktop.Telepathy.Client.foo"));
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
