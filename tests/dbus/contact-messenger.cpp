#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/Client>
#include <TelepathyQt4/ContactMessenger>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/MessageContentPart>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingSendMessage>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo2/chan.h>
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

    void setSimulatedSendError(const QString &error)
    {
        mSimulatedSendError = error;
    }

public Q_SLOTS: // Methods
    QString Send(const QDBusObjectPath &account,
            const QString &targetID, const MessagePartList &message,
            uint flags)
    {
        if (!mSimulatedSendError.isEmpty()) {
            dynamic_cast<QDBusContext *>(parent())->sendErrorReply(mSimulatedSendError,
                    QLatin1String("Let's pretend this interface and method don't exist, shall we?"));
            return QString();
        }

        // TODO implement
        return QString();
    }

Q_SIGNALS:
    void Status(const QDBusObjectPath &account, const QString &token, uint status,
            const QString &error);

private:

    QDBusConnection mBus;
    QString mSimulatedSendError;
};

class Dispatcher : public QObject, public QDBusContext
{
    Q_OBJECT;

public:
    Dispatcher(QObject *parent)
        : QObject(parent)
    {
    }

    ~Dispatcher()
    {
    }
};

class TestContactMessenger : public Test
{
    Q_OBJECT

public:
    TestContactMessenger(QObject *parent = 0)
        : Test(parent),
          mCDMessagesAdaptor(0),
          // service side (telepathy-glib)
          mConnService(0), mBaseConnService(0), mContactRepo(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testNoSupport();
    void testObserverRegistration();

    void cleanup();
    void cleanupTestCase();

private:

    QList<ClientObserverInterface *> ourObservers();

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    CDMessagesAdaptor *mCDMessagesAdaptor;

    TpTestsContactsConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;

    QString mConnName;
    QString mConnPath;
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
    Dispatcher *dispatcher = new Dispatcher(this);
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

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL));
    QVERIFY(mConnService != 0);
    mBaseConnService = TP_BASE_CONNECTION(mConnService);
    QVERIFY(mBaseConnService != 0);

    gchar *name, *connPath;
    GError *error = NULL;

    QVERIFY(tp_base_connection_register(mBaseConnService,
                "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);
}

void TestContactMessenger::init()
{
    initImpl();

    mCDMessagesAdaptor->setSimulatedSendError(QString());
}

void TestContactMessenger::testNoSupport()
{
    // We should give a descriptive error message if the CD doesn't actually support sending
    // messages using the new API. NotImplemented should probably be documented for the
    // sendMessage() methods as an indication that the CD implementation needs to be upgraded.

    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, QLatin1String("Ann"));
    QVERIFY(!messenger.isNull());

    mCDMessagesAdaptor->setSimulatedSendError(TP_QT4_DBUS_ERROR_UNKNOWN_METHOD);

    PendingSendMessage *pendingSend = messenger->sendMessage(QLatin1String("Hi!"));
    QVERIFY(pendingSend != NULL);

    QVERIFY(connect(pendingSend,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QVERIFY(pendingSend->isFinished());
    QVERIFY(!pendingSend->isValid());

    QCOMPARE(pendingSend->errorName(), TP_QT4_ERROR_NOT_IMPLEMENTED);

    // Let's try using the other sendMessage overload similarly as well

    Message m(ChannelTextMessageTypeAction, QLatin1String("is testing!"));

    pendingSend = messenger->sendMessage(m.parts());
    QVERIFY(pendingSend != NULL);

    QVERIFY(connect(pendingSend,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QVERIFY(pendingSend->isFinished());
    QVERIFY(!pendingSend->isValid());

    QCOMPARE(pendingSend->errorName(), TP_QT4_ERROR_NOT_IMPLEMENTED);
}

void TestContactMessenger::testObserverRegistration()
{
    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, QLatin1String("Ann"));

    // At this point, there should be a registered observer for the relevant channel class on our
    // unique name

    QList<ClientObserverInterface *> observers = ourObservers();
    QVERIFY(!observers.empty());

    Q_FOREACH(ClientObserverInterface *observer, observers) {
        // It shouldn't have recover == true, as it shouldn't be activatable at all, and hence recovery
        // doesn't make sense for it
        bool recover;
        QVERIFY(waitForProperty(observer->requestPropertyRecover(), &recover));
        QVERIFY(!recover);
    }

    // If we destroy our messenger (which is the last/only one for that ID), the observers should go
    // away, at least in a few mainloop iterations
    messenger.reset();

    for (int i = 0; i < 100; i++) {
        mLoop->processEvents();
    }

    QVERIFY(!ourObservers().empty());
}

void TestContactMessenger::cleanup()
{
    cleanupImpl();
}

void TestContactMessenger::cleanupTestCase()
{
    if (mConnService != 0) {
        mBaseConnService = 0;
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QList<ClientObserverInterface *> TestContactMessenger::ourObservers()
{
    QStringList registeredNames =
        QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QList<ClientObserverInterface *> observers;

    Q_FOREACH (QString name, registeredNames) {
        if (!name.startsWith(QLatin1String("org.freedesktop.Telepathy.Client."))) {
            continue;
        }

        if (QDBusConnection::sessionBus().interface()->serviceOwner(name).value() !=
                QDBusConnection::sessionBus().baseService()) {
            continue;
        }

        QString path = QLatin1Char('/') + name.replace(QLatin1Char('.'), QLatin1Char('/'));

        ClientInterface client(name, path);
        QStringList ifaces;
        if (!waitForProperty(client.requestPropertyInterfaces(), &ifaces)) {
            continue;
        }

        if (!ifaces.contains(TP_QT4_IFACE_CLIENT_OBSERVER)) {
            continue;
        }

        ClientObserverInterface *observer = new ClientObserverInterface(name, path, this);

        ChannelClassList filter;
        if (!waitForProperty(observer->requestPropertyObserverChannelFilter(), &filter)) {
            continue;
        }

        Q_FOREACH (ChannelClassSpec spec, filter) {
            if (spec.isSubsetOf(ChannelClassSpec::textChat())) {
                observers.push_back(observer);
                qDebug() << "Found our observer" << name << '\n';
                break;
            }
        }
    }

    return observers;
}

QTEST_MAIN(TestContactMessenger)
#include "_gen/contact-messenger.cpp.moc.hpp"
