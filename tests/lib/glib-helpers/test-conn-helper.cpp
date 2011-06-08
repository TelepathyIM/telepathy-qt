#include "tests/lib/glib-helpers/test-conn-helper.h"
#include "tests/lib/glib-helpers/_gen/test-conn-helper.moc.hpp"

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/PendingReady>

TestConnHelper::TestConnHelper(Test *parent,
        GType gType, const QString &account, const QString &protocol)
    : QObject(parent)
{
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());
    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create();

    init(parent, channelFactory, contactFactory,
         gType,
         "account", account.toLatin1().constData(),
         "protocol", protocol.toLatin1().constData(),
         NULL);
}

TestConnHelper::TestConnHelper(Test *parent,
        GType gType, const char *firstPropertyName, ...)
    : QObject(parent)
{
    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());
    Tp::ContactFactoryPtr contactFactory = Tp::ContactFactory::create();

    va_list varArgs;
    va_start(varArgs, firstPropertyName);
    init(parent, channelFactory, contactFactory,
         gType, firstPropertyName, varArgs);
    va_end(varArgs);
}

TestConnHelper::TestConnHelper(Test *parent,
        const Tp::ChannelFactoryConstPtr &channelFactory,
        const Tp::ContactFactoryConstPtr &contactFactory,
        GType gType, const QString &account, const QString &protocol)
    : QObject(parent)
{
    init(parent, channelFactory, contactFactory,
         gType,
         "account", account.toLatin1().constData(),
         "protocol", protocol.toLatin1().constData(),
         NULL);
}

TestConnHelper::TestConnHelper(Test *parent,
        const Tp::ChannelFactoryConstPtr &channelFactory,
        const Tp::ContactFactoryConstPtr &contactFactory,
        GType gType, const char *firstPropertyName, ...)
    : QObject(parent)
{
    va_list varArgs;
    va_start(varArgs, firstPropertyName);
    init(parent, channelFactory, contactFactory,
         gType, firstPropertyName, varArgs);
    va_end(varArgs);
}

TestConnHelper::~TestConnHelper()
{
    disconnect();

    if (mService != 0) {
        g_object_unref(mService);
        mService = 0;
    }
}

void TestConnHelper::init(Test *parent,
        const Tp::ChannelFactoryConstPtr &channelFactory,
        const Tp::ContactFactoryConstPtr &contactFactory,
        GType gType, const char *firstPropertyName, ...)
{
    va_list varArgs;
    va_start(varArgs, firstPropertyName);
    init(parent, channelFactory, contactFactory,
         gType, firstPropertyName, varArgs);
    va_end(varArgs);
}

void TestConnHelper::init(Test *parent,
        const Tp::ChannelFactoryConstPtr &channelFactory,
        const Tp::ContactFactoryConstPtr &contactFactory,
        GType gType, const char *firstPropertyName, va_list varArgs)
{
    mParent = parent;
    mLoop = parent->mLoop;

    mService = g_object_new_valist(gType, firstPropertyName, varArgs);
    QVERIFY(mService != 0);

    gchar *connBusName;
    gchar *connPath;
    GError *error = 0;
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mService),
                "testcm", &connBusName, &connPath, &error));
    QVERIFY(error == 0);
    QVERIFY(connBusName != 0);
    QVERIFY(connPath != 0);

    mClient = Tp::Connection::create(QLatin1String(connBusName), QLatin1String(connPath),
            channelFactory, contactFactory);
    QCOMPARE(mClient->isReady(), false);

    g_free(connBusName);
    g_free(connPath);
}

QString TestConnHelper::objectPath() const
{
    return mClient->objectPath();
}

bool TestConnHelper::isValid() const
{
    return mClient->isValid();
}

bool TestConnHelper::isReady(const Tp::Features &features) const
{
    return mClient->isReady(features);
}

bool TestConnHelper::enableFeatures(const Tp::Features &features)
{
    mLoop->processEvents();

    QObject::connect(mClient->becomeReady(features),
            SIGNAL(finished(Tp::PendingOperation*)),
            mParent,
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    return ((mLoop->exec() == 0) && mClient->isReady(features));
}

bool TestConnHelper::connect(const Tp::Features &features)
{
    mLoop->processEvents();

    QObject::connect(mClient->lowlevel()->requestConnect(features),
            SIGNAL(finished(Tp::PendingOperation*)),
            mParent,
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    return ((mLoop->exec() == 0) &&
            (mClient->status() == Tp::ConnectionStatusConnected) && mClient->isReady(features));
}

bool TestConnHelper::disconnect()
{
    if (!mClient->isValid()) {
        return false;
    }

    mLoop->processEvents();

    QObject::connect(mClient.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(expectConnInvalidated()));
    tp_base_connection_change_status(TP_BASE_CONNECTION(mService),
            TP_CONNECTION_STATUS_DISCONNECTED, TP_CONNECTION_STATUS_REASON_REQUESTED);
    return ((mLoop->exec() == 0) &&
            !mClient->isValid() && (mClient->status() == Tp::ConnectionStatusDisconnected));
}

bool TestConnHelper::disconnectWithDBusError(
        const char *errorName, GHashTable *details, Tp::ConnectionStatusReason reason)
{
    if (!mClient->isValid()) {
        return false;
    }

    mLoop->processEvents();

    QObject::connect(mClient.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(expectConnInvalidated()));
    tp_base_connection_disconnect_with_dbus_error(
            TP_BASE_CONNECTION(mService), errorName, details, (TpConnectionStatusReason) reason);
    return ((mLoop->exec() == 0) &&
            !mClient->isValid() && (mClient->status() == Tp::ConnectionStatusDisconnected));
}

void TestConnHelper::expectConnInvalidated()
{
    mLoop->exit(0);
}
