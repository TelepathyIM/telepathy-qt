#include "tests/lib/glib-helpers/test-conn-helper.h"
#include "tests/lib/glib-helpers/_gen/test-conn-helper.moc.hpp"

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>

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

QList<Tp::ContactPtr> TestConnHelper::contacts(const QStringList &ids,
        const Tp::Features &features)
{
    mLoop->processEvents();

    QList<Tp::ContactPtr> ret;
    Tp::PendingContacts *pc = mClient->contactManager()->contactsForIdentifiers(ids, features);
    mContactFeatures = features;
    QObject::connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectContactsForIdentifiersFinished(Tp::PendingOperation*)));
    if (mLoop->exec() == 0) {
        ret = mContacts;
    }
    mContactFeatures.clear();
    mContacts.clear();
    return ret;
}

QList<Tp::ContactPtr> TestConnHelper::contacts(const Tp::UIntList &handles,
        const Tp::Features &features)
{
    mLoop->processEvents();

    QList<Tp::ContactPtr> ret;
    Tp::PendingContacts *pc = mClient->contactManager()->contactsForHandles(handles, features);
    mContactFeatures = features;
    QObject::connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectContactsForHandlesFinished(Tp::PendingOperation*)));
    if (mLoop->exec() == 0) {
        ret = mContacts;
    }
    mContactFeatures.clear();
    mContacts.clear();
    return ret;
}

QList<Tp::ContactPtr> TestConnHelper::upgradeContacts(const QList<Tp::ContactPtr> &contacts,
        const Tp::Features &features)
{
    mLoop->processEvents();

    QList<Tp::ContactPtr> ret;
    Tp::PendingContacts *pc = mClient->contactManager()->upgradeContacts(contacts, features);
    mContactFeatures = features;
    QObject::connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectUpgradeContactsFinished(Tp::PendingOperation*)));
    if (mLoop->exec() == 0) {
        ret = mContacts;
    }
    mContactFeatures.clear();
    mContacts.clear();
    return ret;
}

Tp::ChannelPtr TestConnHelper::createChannel(const QVariantMap &request)
{
    mLoop->processEvents();

    Tp::ChannelPtr ret;
    Tp::PendingChannel *pc = mClient->lowlevel()->createChannel(request);
    QObject::connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectCreateChannelFinished(Tp::PendingOperation*)));
    if (mLoop->exec() == 0) {
        ret = mChannel;
    }
    mChannel.reset();
    return ret;
}

Tp::ChannelPtr TestConnHelper::createChannel(const QString &channelType, const Tp::ContactPtr &target)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   target->handle()[0]);
    return createChannel(request);
}

Tp::ChannelPtr TestConnHelper::createChannel(const QString &channelType,
        Tp::HandleType targetHandleType, uint targetHandle)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) targetHandleType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   targetHandle);
    return createChannel(request);
}

Tp::ChannelPtr TestConnHelper::createChannel(const QString &channelType,
        Tp::HandleType targetHandleType, const QString &targetID)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) targetHandleType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   targetID);
    return ensureChannel(request);
}

Tp::ChannelPtr TestConnHelper::ensureChannel(const QVariantMap &request)
{
    mLoop->processEvents();

    Tp::ChannelPtr ret;
    Tp::PendingChannel *pc = mClient->lowlevel()->ensureChannel(request);
    QObject::connect(pc,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectEnsureChannelFinished(Tp::PendingOperation*)));
    if (mLoop->exec() == 0) {
        ret = mChannel;
    }
    mChannel.reset();
    return ret;
}

Tp::ChannelPtr TestConnHelper::ensureChannel(const QString &channelType, const Tp::ContactPtr &target)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   target->handle()[0]);
    return ensureChannel(request);
}

Tp::ChannelPtr TestConnHelper::ensureChannel(const QString &channelType,
        Tp::HandleType targetHandleType, uint targetHandle)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) targetHandleType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"),
                   targetHandle);
    return ensureChannel(request);
}

Tp::ChannelPtr TestConnHelper::ensureChannel(const QString &channelType,
        Tp::HandleType targetHandleType, const QString &targetID)
{
    QVariantMap request;
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
                   channelType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
                   (uint) targetHandleType);
    request.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
                   targetID);
    return ensureChannel(request);
}

void TestConnHelper::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestConnHelper::expectContactsForIdentifiersFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts *>(op);
    QCOMPARE(pc->isForHandles(), false);
    QCOMPARE(pc->isForIdentifiers(), true);
    QCOMPARE(pc->isUpgrade(), false);

    expectPendingContactsFinished(pc);
}

void TestConnHelper::expectContactsForHandlesFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts *>(op);
    QCOMPARE(pc->isForHandles(), true);
    QCOMPARE(pc->isForIdentifiers(), false);
    QCOMPARE(pc->isUpgrade(), false);

    expectPendingContactsFinished(pc);
}

void TestConnHelper::expectUpgradeContactsFinished(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts *>(op);
    QCOMPARE(pc->isUpgrade(), true);

    expectPendingContactsFinished(pc);
}

void TestConnHelper::expectPendingContactsFinished(Tp::PendingContacts *pc)
{
    QCOMPARE(pc->manager(), mClient->contactManager());

    if (pc->isError()) {
        qWarning().nospace() << pc->errorName() << ": " << pc->errorMessage();
        mContacts.clear();
        mLoop->exit(1);
    } else {
        mContacts = pc->contacts();
        Q_FOREACH (const Tp::ContactPtr &contact, mContacts) {
            QVERIFY(contact->requestedFeatures().contains(mContactFeatures));
        }
        mLoop->exit(0);
    }
}

void TestConnHelper::expectCreateChannelFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning().nospace() << op->errorName() << ": " << op->errorMessage();
        mChannel.reset();
        mLoop->exit(1);
    } else {
        Tp::PendingChannel *pc = qobject_cast<Tp::PendingChannel *>(op);
        QCOMPARE(pc->yours(), true);
        mChannel = pc->channel();
        mLoop->exit(0);
    }
}

void TestConnHelper::expectEnsureChannelFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning().nospace() << op->errorName() << ": " << op->errorMessage();
        mChannel.reset();
        mLoop->exit(1);
    } else {
        Tp::PendingChannel *pc = qobject_cast<Tp::PendingChannel *>(op);
        QCOMPARE(pc->yours(), false);
        mChannel = pc->channel();
        mLoop->exit(0);
    }
}
