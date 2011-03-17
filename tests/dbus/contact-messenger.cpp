#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo/chan.h>
#include <tests/lib/test.h>

using namespace Tp;
using namespace Tp::Client;

class CDMessagesAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelDispatcher.Interface.Messages")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelDispatcher.Interface.Messages.DRAFT\" >\n"
"    <method name=\"Send\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"TargetID\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Message\" type=\"aa{sv}\" direction=\"in\" />\n"
"      <arg name=\"Flags\" type=\"u\" direction=\"in\" />\n"
"      <arg name=\"Token\" type=\"s\" direction=\"out\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    CDMessagesAdaptor(const QDBusConnection &bus, QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mBus(bus)
    {
    }

    virtual ~CDMessagesAdaptor()
    {
    }

public Q_SLOTS: // Methods
    QString Send(const QDBusObjectPath &account,
            const QString &targetID, const MessagePartList &message,
            uint flags)
    {
        // TODO implement
        return QString();
    }

Q_SIGNALS:
    void Status(const QDBusObjectPath &account, const QString &token, uint status,
            const QString &error);

private:

    QDBusConnection mBus;
};

class TestContactMessenger : public Test
{
    Q_OBJECT

public:
    TestContactMessenger(QObject *parent = 0)
        : Test(parent),
          mCDMessagesAdaptor(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void cleanup();
    void cleanupTestCase();

private:

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    CDMessagesAdaptor *mCDMessagesAdaptor;
};

void TestContactMessenger::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-messenger");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    QDBusConnection bus = QDBusConnection::sessionBus();
    QString channelDispatcherBusName = QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCHER);
    QString channelDispatcherPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher");
    QObject *dispatcher = new QObject(this);
    mCDMessagesAdaptor = new CDMessagesAdaptor(bus, dispatcher);
    QVERIFY(bus.registerService(channelDispatcherBusName));
    QVERIFY(bus.registerObject(channelDispatcherPath, dispatcher));

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
    QVERIFY(connect(mAccount->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAccount->isReady(), true);

    QCOMPARE(mAccount->supportsRequestHints(), false);
    QCOMPARE(mAccount->requestsSucceedWithChannel(), false);
}

void TestContactMessenger::init()
{
    initImpl();
}

void TestContactMessenger::cleanup()
{
    cleanupImpl();
}

void TestContactMessenger::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactMessenger)
#include "_gen/contact-messenger.cpp.moc.hpp"
