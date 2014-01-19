#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/Client>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Message>
#include <TelepathyQt/MessageContentPart>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>

#include <telepathy-glib/cm-message.h>
#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo2/chan.h>
#include <tests/lib/test.h>

using namespace Tp;
using namespace Tp::Client;

class TestContactMessenger;

class CDMessagesAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelDispatcher.Interface.Messages1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelDispatcher.Interface.Messages1\" >\n"
"    <method name=\"SendMessage\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"TargetID\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Message\" type=\"aa{sv}\" direction=\"in\" />\n"
"      <arg name=\"Flags\" type=\"u\" direction=\"in\" />\n"
"      <arg name=\"Token\" type=\"s\" direction=\"out\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    CDMessagesAdaptor(const QDBusConnection &bus, TestContactMessenger *test, QObject *parent)
        : QDBusAbstractAdaptor(parent),
        test(test),
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
    QString SendMessage(const QDBusObjectPath &account,
            const QString &targetID, const Tp::MessagePartList &message,
            uint flags);

private:

    TestContactMessenger *test;
    QDBusConnection mBus;
    QString mSimulatedSendError;
};

class AccountAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Account")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Account\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"Connection\" type=\"o\" access=\"read\" />\n"
"    <signal name=\"AccountPropertyChanged\" >\n"
"      <arg name=\"Properties\" type=\"a{sv}\" />\n"
"    </signal>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Connection READ Connection)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    AccountAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent), mConnection(QLatin1String("/"))
    {
    }

    virtual ~AccountAdaptor()
    {
    }

    void setConnection(QString conn)
    {
        if (conn.isEmpty()) {
            conn = QLatin1String("/");
        }

        mConnection = QDBusObjectPath(conn);
        QVariantMap props;
        props.insert(QLatin1String("Connection"), QVariant::fromValue(mConnection));
        Q_EMIT AccountPropertyChanged(props);
    }

public: // Properties
    inline QDBusObjectPath Connection() const
    {
        return mConnection;
    }

    inline QStringList Interfaces() const
    {
        return QStringList();
    }

Q_SIGNALS: // Signals
    void AccountPropertyChanged(const QVariantMap &properties);

private:
    QDBusObjectPath mConnection;
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
          mCDMessagesAdaptor(0), mAccountAdaptor(0),
          // service side (telepathy-glib)
          mConnService(0), mBaseConnService(0), mContactRepo(0),
          mSendFinished(false), mGotMessageSent(false)
    { }

protected Q_SLOTS:
    void expectPendingContactsFinished(Tp::PendingOperation *op);
    void onSendFinished(Tp::PendingOperation *);
    void onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
            const QString &sentMessageToken, const Tp::TextChannelPtr &channel);
    void onMessageReceived(const Tp::ReceivedMessage &message,
            const Tp::TextChannelPtr &channel);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testNoSupport();
    void testObserverRegistration();
    void testSimpleSend();
    void testReceived();
    void testReceivedFromContact();

    void cleanup();
    void cleanupTestCase();

private:

    friend class CDMessagesAdaptor;

    QList<ClientObserverInterface *> ourObservers();

    CDMessagesAdaptor *mCDMessagesAdaptor;
    AccountAdaptor *mAccountAdaptor;
    QString mAccountBusName, mAccountPath;

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    ConnectionPtr mConn;
    TextChannelPtr mChan;

    TpTestsContactsConnection *mConnService;
    TpBaseConnection *mBaseConnService;
    TpHandleRepoIface *mContactRepo;
    ExampleEcho2Channel *mMessagesChanService;

    QString mConnName;
    QString mConnPath;
    QString mMessagesChanPath;

    bool mSendFinished, mGotMessageSent, mGotMessageReceived;
    QString mSendError, mSendToken, mMessageSentText, mMessageSentToken, mMessageSentChannel;
    QString mMessageReceivedText;
    ChannelPtr mMessageReceivedChan;

    QList<ContactPtr> mContacts;
};

QString CDMessagesAdaptor::SendMessage(const QDBusObjectPath &account,
        const QString &targetID, const MessagePartList &message,
        uint flags)
{
    if (!mSimulatedSendError.isEmpty()) {
        dynamic_cast<QDBusContext *>(QObject::parent())->sendErrorReply(mSimulatedSendError,
                QLatin1String("Let's pretend this interface and method don't exist, shall we?"));
        return QString();
    }

    /*
     * Sadly, the QDBus local-loop "optimization" prevents us from correctly waiting for the
     * ObserveChannels call to return, and consequently prevents us from knowing when we can call
     * Send, knowing that the observer has connected to the message sent signal.
     *
     * The real MC doesn't have this limitation because it actually really calls and waits our
     * ObserveChannels method to finish, unlike dear QDBus here.
     */
    QList<ClientObserverInterface *> observers = test->ourObservers();
    Q_FOREACH(ClientObserverInterface *iface, observers) {
        ChannelDetails chan = { QDBusObjectPath(test->mChan->objectPath()), test->mChan->immutableProperties() };
        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(iface->ObserveChannels(
                QDBusObjectPath(test->mAccount->objectPath()),
                QDBusObjectPath(test->mChan->connection()->objectPath()),
                ChannelDetailsList() << chan,
                QDBusObjectPath(QLatin1String("/")),
                Tp::ObjectPathList(),
                QVariantMap()));

        connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    test->mLoop,
                    SLOT(quit()));
        test->mLoop->exec();
        QDBusPendingReply<void> reply = *watcher;
        qDebug() << reply.error(); // Always gives out "local-loop messages can't have delayed replies"

        delete watcher;
    }

    qDebug() << "Calling send"; // And this is always called before the observer manages to connect to messageSent. Bummer.

    PendingSendMessage *msg = test->mChan->send(message, static_cast<MessageSendingFlags>(flags));
    connect(msg,
            SIGNAL(finished(Tp::PendingOperation*)),
            test,
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    test->mLoop->exec();
    return msg->sentMessageToken();
}

void TestContactMessenger::expectPendingContactsFinished(PendingOperation *op)
{
    TEST_VERIFY_OP(op);

    PendingContacts *pending = qobject_cast<PendingContacts *>(op);
    mContacts = pending->contacts();
    mLoop->exit(0);
}

void TestContactMessenger::onSendFinished(Tp::PendingOperation *op)
{
    PendingSendMessage *msg = qobject_cast<PendingSendMessage *>(op);
    QVERIFY(msg != NULL);

    if (msg->isValid()) {
        qDebug() << "Send succeeded, got token" << msg->sentMessageToken();
        mSendToken = msg->sentMessageToken();
    } else {
        qDebug() << "Send failed, got error" << msg->errorName();
        mSendError = msg->errorName();
    }

    mSendFinished = true;
}

void TestContactMessenger::onMessageSent(const Tp::Message &message, Tp::MessageSendingFlags flags,
        const QString &sentMessageToken, const Tp::TextChannelPtr &channel)
{
    qDebug() << "Got ContactMessenger::messageSent()";

    mGotMessageSent = true;
    mMessageSentToken = sentMessageToken;
    mMessageSentText = message.text();
}

void TestContactMessenger::onMessageReceived(const Tp::ReceivedMessage &message,
        const Tp::TextChannelPtr &channel)
{
    qDebug() << "Got ContactMessenger::messageReceived()";

    mGotMessageReceived = true;
    mMessageReceivedText = message.text();
    mMessageReceivedChan = channel;
}

void TestContactMessenger::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contact-messenger");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    QDBusConnection bus = QDBusConnection::sessionBus();
    QString channelDispatcherBusName = TP_QT_IFACE_CHANNEL_DISPATCHER;
    QString channelDispatcherPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher");
    Dispatcher *dispatcher = new Dispatcher(this);
    mCDMessagesAdaptor = new CDMessagesAdaptor(bus, this, dispatcher);
    QVERIFY(bus.registerService(channelDispatcherBusName));
    QVERIFY(bus.registerObject(channelDispatcherPath, dispatcher));

    mAccountBusName = TP_QT_IFACE_ACCOUNT_MANAGER;
    mAccountPath = QLatin1String("/org/freedesktop/Telepathy/Account/simple/simple/account");
    QObject *acc = new QObject(this);

    mAccountAdaptor = new AccountAdaptor(acc);

    QVERIFY(bus.registerService(mAccountBusName));
    QVERIFY(bus.registerObject(mAccountPath, acc));

    mAccount = Account::create(mAccountBusName, mAccountPath);
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

    mAccountAdaptor->setConnection(mConnPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);
    QCOMPARE(static_cast<uint>(mConn->status()),
            static_cast<uint>(ConnectionStatusConnected));

    mContactRepo = tp_base_connection_get_handles(mBaseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(mContactRepo, "Ann", 0, 0);

    mMessagesChanPath = mConnPath + QLatin1String("/MessagesChannel");
    QByteArray chanPath = mMessagesChanPath.toLatin1();
    mMessagesChanService = EXAMPLE_ECHO_2_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_2_CHANNEL,
                "connection", mConnService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    QVariantMap immutableProperties;
    immutableProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"),
            QLatin1String("ann"));
    mChan = TextChannel::create(mConn, mMessagesChanPath, immutableProperties);
    QVERIFY(connect(mChan->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestContactMessenger::init()
{
    initImpl();

    mSendFinished = false;
    mGotMessageSent = false;
    mGotMessageReceived = false;
    mCDMessagesAdaptor->setSimulatedSendError(QString());
}

void TestContactMessenger::testNoSupport()
{
    // We should give a descriptive error message if the CD doesn't actually support sending
    // messages using the new API. NotImplemented should probably be documented for the
    // sendMessage() methods as an indication that the CD implementation needs to be upgraded.

    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, QLatin1String("Ann"));
    QVERIFY(!messenger.isNull());

    mCDMessagesAdaptor->setSimulatedSendError(TP_QT_DBUS_ERROR_UNKNOWN_METHOD);

    PendingSendMessage *pendingSend = messenger->sendMessage(QLatin1String("Hi!"));
    QVERIFY(pendingSend != NULL);

    QVERIFY(connect(pendingSend,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pendingSend->isFinished());
    QVERIFY(!pendingSend->isValid());

    QCOMPARE(pendingSend->errorName(), TP_QT_ERROR_NOT_IMPLEMENTED);

    // Let's try using the other sendMessage overload similarly as well

    Message m(ChannelTextMessageTypeAction, QLatin1String("is testing!"));

    pendingSend = messenger->sendMessage(m.parts());
    QVERIFY(pendingSend != NULL);

    QVERIFY(connect(pendingSend,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectFailure(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pendingSend->isFinished());
    QVERIFY(!pendingSend->isValid());

    QCOMPARE(pendingSend->errorName(), TP_QT_ERROR_NOT_IMPLEMENTED);
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
        QCOMPARE(recover, true);
    }

    // If we destroy our messenger (which is the last/only one for that ID), the observers should go
    // away, at least in a few mainloop iterations
    messenger.reset();

    QVERIFY(ourObservers().empty());
}

void TestContactMessenger::testSimpleSend()
{
    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, QLatin1String("Ann"));

    QVERIFY(connect(messenger->sendMessage(QLatin1String("Hi!")),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onSendFinished(Tp::PendingOperation*))));

    while (!mSendFinished) {
        mLoop->processEvents();
    }

    QVERIFY(mSendError.isEmpty());
}

void TestContactMessenger::testReceived()
{
    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, QLatin1String("Ann"));

    QVERIFY(connect(messenger.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
            SLOT(onMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr))));

    QList<ClientObserverInterface *> observers = ourObservers();
    Q_FOREACH(ClientObserverInterface *iface, observers) {
        ChannelDetails chan = { QDBusObjectPath(mChan->objectPath()), mChan->immutableProperties() };
        iface->ObserveChannels(
                QDBusObjectPath(mAccount->objectPath()),
                QDBusObjectPath(mChan->connection()->objectPath()),
                ChannelDetailsList() << chan,
                QDBusObjectPath(QLatin1String("/")),
                Tp::ObjectPathList(),
                QVariantMap());
    }

    guint handle = tp_handle_ensure(mContactRepo, "Ann", 0, 0);
    TpMessage *msg = tp_cm_message_new_text(mBaseConnService, handle, TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL, "Hi!");

    tp_message_mixin_take_received(G_OBJECT(mMessagesChanService), msg);

    while (!mGotMessageReceived) {
        mLoop->processEvents();
    }

    QCOMPARE(mMessageReceivedText, QString::fromLatin1("Hi!"));
    QCOMPARE(mMessageReceivedChan->objectPath(), mChan->objectPath());
}

void TestContactMessenger::testReceivedFromContact()
{
    QVERIFY(connect(mAccount->connection()->contactManager()->contactsForIdentifiers(
                        QStringList() << QLatin1String("Ann")),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    ContactPtr ann = mContacts.first();
    ContactMessengerPtr messenger = ContactMessenger::create(mAccount, ann);

    QVERIFY(connect(messenger.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
            SLOT(onMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr))));

    QList<ClientObserverInterface *> observers = ourObservers();
    Q_FOREACH(ClientObserverInterface *iface, observers) {
        ChannelDetails chan = { QDBusObjectPath(mChan->objectPath()), mChan->immutableProperties() };
        iface->ObserveChannels(
                QDBusObjectPath(mAccount->objectPath()),
                QDBusObjectPath(mChan->connection()->objectPath()),
                ChannelDetailsList() << chan,
                QDBusObjectPath(QLatin1String("/")),
                Tp::ObjectPathList(),
                QVariantMap());
    }

    guint handle = tp_handle_ensure(mContactRepo, "Ann", 0, 0);
    TpMessage *msg = tp_cm_message_new_text(mBaseConnService, handle, TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL, "Hi!");

    tp_message_mixin_take_received(G_OBJECT(mMessagesChanService), msg);

    while (!mGotMessageReceived) {
        mLoop->processEvents();
    }

    QCOMPARE(mMessageReceivedText, QString::fromLatin1("Hi!"));
    QCOMPARE(mMessageReceivedChan->objectPath(), mChan->objectPath());
}


void TestContactMessenger::cleanup()
{
    mMessageReceivedChan.reset();

    cleanupImpl();
}

void TestContactMessenger::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *,
                                               const QString &, const QString &)),
                            mLoop,
                            SLOT(quit())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    mChan.reset();

    if (mMessagesChanService != 0) {
        g_object_unref(mMessagesChanService);
        mMessagesChanService = 0;
    }

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

        QString path = QLatin1Char('/') + name;
        path.replace(QLatin1Char('.'), QLatin1Char('/'));

        ClientInterface client(name, path);
        QStringList ifaces;
        if (!waitForProperty(client.requestPropertyInterfaces(), &ifaces)) {
            continue;
        }

        if (!ifaces.contains(TP_QT_IFACE_CLIENT_OBSERVER)) {
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
