#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <cstring>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/Account>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/Client>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/Debug>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/SimpleCallObserver>
#include <TelepathyQt/SimpleObserver>
#include <TelepathyQt/SimpleTextObserver>
#include <TelepathyQt/StreamedMediaChannel>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Types>

#include <telepathy-glib/cm-message.h>
#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo2/chan.h>
#include <tests/lib/glib/callable/media-channel.h>
#include <tests/lib/test.h>

using namespace Tp;
using namespace Tp::Client;

class TestSimpleObserver;

namespace
{

QList<ChannelPtr> channels(const QList<TextChannelPtr> &channels)
{
    QList<ChannelPtr> ret;
    Q_FOREACH (const TextChannelPtr &channel, channels) {
        ret << channel;
    }
    return ret;
}

QList<ChannelPtr> channels(const QList<StreamedMediaChannelPtr> &channels)
{
    QList<ChannelPtr> ret;
    Q_FOREACH (const StreamedMediaChannelPtr &channel, channels) {
        ret << channel;
    }
    return ret;
}

}

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

class TestSimpleObserver : public Test
{
    Q_OBJECT

public:
    TestSimpleObserver(QObject *parent = 0)
        : Test(parent),
          mChannelsCount(0), mSMChannelsCount(0)
    {
        std::memset(mMessagesChanServices, 0, sizeof(mMessagesChanServices) / sizeof(ExampleEcho2Channel*));
        std::memset(mCallableChanServices, 0, sizeof(mCallableChanServices) / sizeof(ExampleCallableMediaChannel*));
    }

protected Q_SLOTS:
    void onObserverNewChannels(const QList<Tp::ChannelPtr> &channels);
    void onObserverChannelInvalidated(const Tp::ChannelPtr &channel,
            const QString &errorName, const QString &errorMessage);
    void onObserverStreamedMediaCallStarted(
            const Tp::StreamedMediaChannelPtr &channel);
    void onObserverStreamedMediaCallEnded(
            const Tp::StreamedMediaChannelPtr &channel,
            const QString &errorMessage, const QString &errorName);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testObserverRegistration();
    void testCrossTalk();

    void cleanup();
    void cleanupTestCase();

private:
    QMap<QString, QString> ourObservers();

    AccountPtr mAccounts[2];

    struct ConnInfo {
        ConnInfo()
            : connService(0), baseConnService(0), contactRepo(0)
        {
        }

        TpTestsContactsConnection *connService;
        TpBaseConnection *baseConnService;
        ConnectionPtr conn;
        TpHandleRepoIface *contactRepo;
    };
    ConnInfo mConns[2];

    QStringList mContacts;

    ExampleEcho2Channel *mMessagesChanServices[2];
    TextChannelPtr mTextChans[2];

    ExampleCallableMediaChannel *mCallableChanServices[2];
    StreamedMediaChannelPtr mSMChans[2];

    int mChannelsCount;
    int mSMChannelsCount;
};

void TestSimpleObserver::onObserverNewChannels(const QList<Tp::ChannelPtr> &channels)
{
    QVERIFY(channels.size() == 1);
    mChannelsCount += channels.size();
}

void TestSimpleObserver::onObserverChannelInvalidated(const Tp::ChannelPtr &channel,
        const QString &errorName, const QString &errorMessage)
{
    QVERIFY(!channel.isNull());
    mChannelsCount -= 1;
}

void TestSimpleObserver::onObserverStreamedMediaCallStarted(
        const Tp::StreamedMediaChannelPtr &channel)
{
    QVERIFY(!channel.isNull());
    mSMChannelsCount++;
}

void TestSimpleObserver::onObserverStreamedMediaCallEnded(const Tp::StreamedMediaChannelPtr &channel,
        const QString &errorMessage, const QString &errorName)
{
    QVERIFY(!channel.isNull());
    mSMChannelsCount--;
}

void TestSimpleObserver::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("simple-observer");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    QDBusConnection bus = QDBusConnection::sessionBus();
    QString channelDispatcherBusName = TP_QT_IFACE_CHANNEL_DISPATCHER;
    QString channelDispatcherPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher");
    Dispatcher *dispatcher = new Dispatcher(this);
    QVERIFY(bus.registerService(channelDispatcherBusName));
    QVERIFY(bus.registerObject(channelDispatcherPath, dispatcher));

    mContacts << QLatin1String("alice") << QLatin1String("bob");

    // Create 2 accounts to be used by the tests:
    // - each account contains a connection, a text channel and a SM channel setup:
    // - the channels for the first account have alice as target and;
    // - the channels for the second account have bob as target
    for (int i = 0; i < 2; ++i) {
        // setup account
        QString accountBusName = TP_QT_IFACE_ACCOUNT_MANAGER;
        QString accountPath = QLatin1String("/org/freedesktop/Telepathy/Account/simple/account/") +
            QString::number(i);

        QObject *adaptorObject = new QObject(this);
        AccountAdaptor *accountAdaptor = new AccountAdaptor(adaptorObject);
        QVERIFY(bus.registerService(accountBusName));
        QVERIFY(bus.registerObject(accountPath, adaptorObject));

        AccountPtr acc = Account::create(accountBusName, accountPath);
        QVERIFY(connect(acc->becomeReady(),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(acc->isReady(), true);
        QCOMPARE(acc->supportsRequestHints(), false);
        QCOMPARE(acc->requestsSucceedWithChannel(), false);
        mAccounts[i] = acc;

        // setup conn
        TpTestsContactsConnection *connService = TP_TESTS_CONTACTS_CONNECTION(
                g_object_new(TP_TESTS_TYPE_CONTACTS_CONNECTION,
                    "account", "me@example.com",
                    "protocol", "example",
                    NULL));
        QVERIFY(connService != 0);
        TpBaseConnection *baseConnService = TP_BASE_CONNECTION(connService);
        QVERIFY(baseConnService != 0);

        gchar *connName, *connPath;
        GError *error = NULL;

        QString name(QLatin1String("example") + QString::number(i));
        QVERIFY(tp_base_connection_register(baseConnService,
                    name.toLatin1().constData(), &connName, &connPath, &error));
        QVERIFY(error == 0);

        QVERIFY(connName != 0);
        QVERIFY(connPath != 0);

        ConnectionPtr conn = Connection::create(QLatin1String(connName), QLatin1String(connPath),
                ChannelFactory::create(QDBusConnection::sessionBus()), ContactFactory::create());
        QCOMPARE(conn->isReady(), false);

        accountAdaptor->setConnection(QLatin1String(connPath));

        QVERIFY(connect(conn->lowlevel()->requestConnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(conn->isReady(), true);
        QCOMPARE(conn->status(), ConnectionStatusConnected);

        TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(baseConnService,
                TP_HANDLE_TYPE_CONTACT);

        mConns[i].connService = connService;
        mConns[i].baseConnService = baseConnService;
        mConns[i].conn = conn;
        mConns[i].contactRepo = contactRepo;

        // setup channels
        guint handle = tp_handle_ensure(contactRepo, mContacts[i].toLatin1().constData(), 0, 0);

        QString messagesChanPath = QLatin1String(connPath) +
            QLatin1String("/MessagesChannel/") + QString::number(i);
        mMessagesChanServices[i] = EXAMPLE_ECHO_2_CHANNEL(g_object_new(
                    EXAMPLE_TYPE_ECHO_2_CHANNEL,
                    "connection", connService,
                    "object-path", messagesChanPath.toLatin1().constData(),
                    "handle", handle,
                    NULL));

        QVariantMap immutableProperties;
        immutableProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"), mContacts[i]);
        mTextChans[i] = TextChannel::create(conn, messagesChanPath, immutableProperties);
        QVERIFY(connect(mTextChans[i]->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);

        QString callableChanPath = QLatin1String(connPath) +
            QLatin1String("/CallableChannel/") + QString::number(i);
        mCallableChanServices[i] = EXAMPLE_CALLABLE_MEDIA_CHANNEL(g_object_new(
                    EXAMPLE_TYPE_CALLABLE_MEDIA_CHANNEL,
                    "connection", connService,
                    "object-path", callableChanPath.toLatin1().constData(),
                    "handle", handle,
                    NULL));

        immutableProperties.clear();
        immutableProperties.insert(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"), mContacts[i]);
        mSMChans[i] = StreamedMediaChannel::create(conn, callableChanPath, immutableProperties);
        QVERIFY(connect(mSMChans[i]->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
        QCOMPARE(mLoop->exec(), 0);

        g_free(connName);
        g_free(connPath);
    }
}

void TestSimpleObserver::init()
{
    initImpl();
}

void TestSimpleObserver::testObserverRegistration()
{
    QVERIFY(ourObservers().isEmpty());

    QList<SimpleObserverPtr> observers;
    QList<SimpleTextObserverPtr> textObservers;
    QList<SimpleCallObserverPtr> callObservers;
    int numRegisteredObservers = 0;
    // Observers should be shared by bus and channel class, meaning that
    // the following code should register only 4 observers:
    // - one for text chat rooms
    // - one for text chats
    // - one for incoming/outgoing calls
    // - one for incoming calls (incoming)
    //
    // The Simple*Observer instances are added to the lists observers/textObservers/callObservers,
    // so they don't get deleted (refcount == 0) when out of scope
    for (int i = 0; i < 2; ++i) {
        AccountPtr acc = mAccounts[i];

        for (int j = 0; j < 2; ++j) {
            QString contact = mContacts[j];

            if (i == 0 && j == 0) {
                numRegisteredObservers = 1;
            }

            // on first run the following code should register an observer for text chat rooms,
            // on consecutive runs it should use the already registered observer for text chat rooms
            SimpleObserverPtr observer = SimpleObserver::create(acc,
                    ChannelClassSpec::textChatroom(), contact);
            QCOMPARE(observer->account(), acc);
            QVERIFY(observer->channelFilter().size() == 1);
            QVERIFY(observer->channelFilter().contains(ChannelClassSpec::textChatroom()));
            QCOMPARE(observer->contactIdentifier(), contact);
            QVERIFY(observer->extraChannelFeatures().isEmpty());
            QVERIFY(observer->channels().isEmpty());
            observers.append(observer);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);

            // the following code should always reuse the observer for text chat rooms already
            // created.
            QList<ChannelClassFeatures> extraChannelFeatures;
            extraChannelFeatures.append(QPair<ChannelClassSpec, Features>(
                        ChannelClassSpec::textChatroom(), Channel::FeatureCore));
            observer = SimpleObserver::create(acc,
                    ChannelClassSpec::textChatroom(), contact, extraChannelFeatures);
            QCOMPARE(observer->account(), acc);
            QVERIFY(observer->channelFilter().size() == 1);
            QVERIFY(observer->channelFilter().contains(ChannelClassSpec::textChatroom()));
            QCOMPARE(observer->contactIdentifier(), contact);
            QCOMPARE(observer->extraChannelFeatures(), extraChannelFeatures);
            QVERIFY(observer->channels().isEmpty());
            observers.append(observer);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);

            if (i == 0 && j == 0) {
                numRegisteredObservers = 2;
            }

            // on first run the following code should register an observer for text chats,
            // on consecutive runs it should use the already registered observer for text chats
            SimpleTextObserverPtr textObserver = SimpleTextObserver::create(acc, contact);
            QCOMPARE(textObserver->account(), acc);
            QCOMPARE(textObserver->contactIdentifier(), contact);
            QVERIFY(textObserver->textChats().isEmpty());
            textObservers.append(textObserver);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);

            // the following code should always reuse the observer for text chats already
            // created.
            textObserver = SimpleTextObserver::create(acc, contact);
            QCOMPARE(textObserver->account(), acc);
            QCOMPARE(textObserver->contactIdentifier(), contact);
            QVERIFY(textObserver->textChats().isEmpty());
            textObservers.append(textObserver);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);

            if (i == 0 && j == 0) {
                numRegisteredObservers = 3;
            }

            // on first run the following code should register an observer for incoming/outgoing
            // calls, on consecutive runs it should use the already registered observer for
            // incoming/outgoing calls
            SimpleCallObserverPtr callObserver = SimpleCallObserver::create(acc, contact);
            QCOMPARE(callObserver->account(), acc);
            QCOMPARE(callObserver->contactIdentifier(), contact);
            QCOMPARE(callObserver->direction(), SimpleCallObserver::CallDirectionBoth);
            QVERIFY(callObserver->streamedMediaCalls().isEmpty());
            callObservers.append(callObserver);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);

            if (i == 0 && j == 0) {
                numRegisteredObservers = 4;
            }

            // on first run the following code should register an observer for incoming calls,
            // on consecutive runs it should use the already registered observer for incoming calls
            callObserver = SimpleCallObserver::create(acc, contact,
                    SimpleCallObserver::CallDirectionIncoming);
            QCOMPARE(callObserver->account(), acc);
            QCOMPARE(callObserver->contactIdentifier(), contact);
            QCOMPARE(callObserver->direction(), SimpleCallObserver::CallDirectionIncoming);
            QVERIFY(callObserver->streamedMediaCalls().isEmpty());
            callObservers.append(callObserver);
            QCOMPARE(ourObservers().size(), numRegisteredObservers);
        }
    }

    // deleting all SimpleObserver instances (text chat room) should unregister 1 observer
    observers.clear();
    QCOMPARE(ourObservers().size(), 3);
    // deleting all SimpleTextObserver instances should unregister 1 observer
    textObservers.clear();
    QCOMPARE(ourObservers().size(), 2);
    // deleting all SimpleCallObserver instances should unregister 2 observers
    callObservers.clear();
    QVERIFY(ourObservers().isEmpty());
}

void TestSimpleObserver::testCrossTalk()
{
    SimpleObserverPtr observers[2];
    SimpleTextObserverPtr textObservers[2];
    SimpleTextObserverPtr textObserversNoContact[2];
    SimpleCallObserverPtr callObservers[2];
    SimpleCallObserverPtr callObserversNoContact[2];

    for (int i = 0; i < 2; ++i) {
        AccountPtr acc = mAccounts[i];
        observers[i] = SimpleObserver::create(acc,
                ChannelClassSpec::textChat(), mContacts[i]);
        QVERIFY(connect(observers[i].data(), SIGNAL(newChannels(QList<Tp::ChannelPtr>)),
                        SLOT(onObserverNewChannels(QList<Tp::ChannelPtr>))));
        QVERIFY(connect(observers[i].data(),
                        SIGNAL(channelInvalidated(Tp::ChannelPtr,QString,QString)),
                        SLOT(onObserverChannelInvalidated(Tp::ChannelPtr,QString,QString))));

        // SimpleTextObserver::messageSent/Received is already tested in contact-messenger test
        textObservers[i] = SimpleTextObserver::create(acc, mContacts[i]);
        textObserversNoContact[i] = SimpleTextObserver::create(acc);

        callObservers[i] = SimpleCallObserver::create(acc, mContacts[i]);
        QVERIFY(connect(callObservers[i].data(), SIGNAL(streamedMediaCallStarted(Tp::StreamedMediaChannelPtr)),
                        SLOT(onObserverStreamedMediaCallStarted(Tp::StreamedMediaChannelPtr))));
        QVERIFY(connect(callObservers[i].data(), SIGNAL(streamedMediaCallEnded(Tp::StreamedMediaChannelPtr,QString,QString)),
                        SLOT(onObserverStreamedMediaCallEnded(Tp::StreamedMediaChannelPtr,QString,QString))));
        callObserversNoContact[i] = SimpleCallObserver::create(acc);
    }

    QMap<QString, QString> ourObserversMap = ourObservers();
    QMap<QString, QString>::const_iterator it = ourObserversMap.constBegin();
    QMap<QString, QString>::const_iterator end = ourObserversMap.constEnd();
    for (; it != end; ++it) {
        ClientObserverInterface *observerIface = new ClientObserverInterface(
                it.key(), it.value(), this);

        ChannelClassList observerFilter;
        if (!waitForProperty(observerIface->requestPropertyObserverChannelFilter(),
                    &observerFilter)) {
            continue;
        }

        for (int i = 0; i < 2; ++i) {
            Q_FOREACH (const ChannelClassSpec &spec, observerFilter) {
                // only call ObserveChannels for text chat channels on observers that support text
                // chat
                if (spec.isSubsetOf(ChannelClassSpec::textChat())) {
                    ChannelDetails textChan = {
                        QDBusObjectPath(mTextChans[i]->objectPath()),
                        mTextChans[i]->immutableProperties()
                    };
                    observerIface->ObserveChannels(
                            QDBusObjectPath(mAccounts[i]->objectPath()),
                            QDBusObjectPath(mTextChans[i]->connection()->objectPath()),
                            ChannelDetailsList() << textChan,
                            QDBusObjectPath(QLatin1String("/")),
                            Tp::ObjectPathList(),
                            QVariantMap());
                    break;
                }
            }

            Q_FOREACH (const ChannelClassSpec &spec, observerFilter) {
                // only call ObserveChannels for SM channels on observers that support SM channels
                if (spec.isSubsetOf(ChannelClassSpec::streamedMediaCall())) {
                    ChannelDetails smChan = {
                        QDBusObjectPath(mSMChans[i]->objectPath()),
                        mSMChans[i]->immutableProperties()
                    };
                    observerIface->ObserveChannels(
                            QDBusObjectPath(mAccounts[i]->objectPath()),
                            QDBusObjectPath(mSMChans[i]->connection()->objectPath()),
                            ChannelDetailsList() << smChan,
                            QDBusObjectPath(QLatin1String("/")),
                            Tp::ObjectPathList(),
                            QVariantMap());
                    break;
                }
            }
        }
    }

    // due to QtDBus limitation, we cannot wait for ObserveChannels to finish properly before
    // proceeding, so we have to wait till all observers receive the channels
    while (observers[0]->channels().isEmpty() ||
           observers[1]->channels().isEmpty() ||
           textObservers[0]->textChats().isEmpty() ||
           textObservers[1]->textChats().isEmpty() ||
           textObserversNoContact[0]->textChats().isEmpty() ||
           textObserversNoContact[1]->textChats().isEmpty() ||
           callObservers[0]->streamedMediaCalls().isEmpty() ||
           callObservers[1]->streamedMediaCalls().isEmpty() ||
           callObserversNoContact[0]->streamedMediaCalls().isEmpty() ||
           callObserversNoContact[1]->streamedMediaCalls().isEmpty()) {
        mLoop->processEvents();
    }

    QCOMPARE(mChannelsCount, 2);
    QCOMPARE(mSMChannelsCount, 2);

    QCOMPARE(observers[0]->channels().size(), 1);
    QCOMPARE(textObservers[0]->textChats().size(), 1);
    QCOMPARE(textObserversNoContact[0]->textChats().size(), 1);
    QCOMPARE(callObservers[0]->streamedMediaCalls().size(), 1);
    QCOMPARE(callObserversNoContact[0]->streamedMediaCalls().size(), 1);
    QCOMPARE(observers[1]->channels().size(), 1);
    QCOMPARE(textObservers[1]->textChats().size(), 1);
    QCOMPARE(textObserversNoContact[1]->textChats().size(), 1);
    QCOMPARE(callObservers[1]->streamedMediaCalls().size(), 1);
    QCOMPARE(callObserversNoContact[1]->streamedMediaCalls().size(), 1);

    QVERIFY(textObservers[0]->textChats() != textObservers[1]->textChats());
    QVERIFY(textObserversNoContact[0]->textChats() != textObserversNoContact[1]->textChats());
    QCOMPARE(textObservers[0]->textChats(), textObserversNoContact[0]->textChats());
    QCOMPARE(textObservers[1]->textChats(), textObserversNoContact[1]->textChats());

    QVERIFY(callObservers[0]->streamedMediaCalls() != callObservers[1]->streamedMediaCalls());
    QVERIFY(callObservers[0]->streamedMediaCalls() != callObserversNoContact[1]->streamedMediaCalls());
    QCOMPARE(callObservers[0]->streamedMediaCalls(), callObserversNoContact[0]->streamedMediaCalls());
    QCOMPARE(callObservers[1]->streamedMediaCalls(), callObserversNoContact[1]->streamedMediaCalls());

    QCOMPARE(observers[0]->channels(), channels(textObservers[0]->textChats()));
    QVERIFY(observers[0]->channels() != channels(textObservers[1]->textChats()));
    QVERIFY(observers[0]->channels() != channels(callObservers[0]->streamedMediaCalls()));
    QVERIFY(observers[0]->channels() != channels(callObservers[1]->streamedMediaCalls()));
    QVERIFY(observers[1]->channels() != channels(textObservers[0]->textChats()));
    QCOMPARE(observers[1]->channels(), channels(textObservers[1]->textChats()));
    QVERIFY(observers[1]->channels() != channels(callObservers[0]->streamedMediaCalls()));
    QVERIFY(observers[1]->channels() != channels(callObservers[1]->streamedMediaCalls()));

    QVERIFY(channels(callObservers[0]->streamedMediaCalls()) != channels(textObservers[0]->textChats()));
    QVERIFY(channels(callObservers[0]->streamedMediaCalls()) != channels(textObservers[1]->textChats()));
    QVERIFY(channels(callObservers[1]->streamedMediaCalls()) != channels(textObservers[0]->textChats()));
    QVERIFY(channels(callObservers[1]->streamedMediaCalls()) != channels(textObservers[1]->textChats()));

    QCOMPARE(observers[0]->channels().first()->objectPath(), mTextChans[0]->objectPath());
    QCOMPARE(observers[1]->channels().first()->objectPath(), mTextChans[1]->objectPath());
    QCOMPARE(textObservers[0]->textChats().first()->objectPath(), mTextChans[0]->objectPath());
    QCOMPARE(textObservers[1]->textChats().first()->objectPath(), mTextChans[1]->objectPath());
    QCOMPARE(textObserversNoContact[0]->textChats().first()->objectPath(), mTextChans[0]->objectPath());
    QCOMPARE(textObserversNoContact[1]->textChats().first()->objectPath(), mTextChans[1]->objectPath());
    QCOMPARE(callObservers[0]->streamedMediaCalls().first()->objectPath(), mSMChans[0]->objectPath());
    QCOMPARE(callObservers[1]->streamedMediaCalls().first()->objectPath(), mSMChans[1]->objectPath());

    // invalidate channels
    for (int i = 0; i < 2; ++i) {
        if (mMessagesChanServices[i] != 0) {
            g_object_unref(mMessagesChanServices[i]);
            mMessagesChanServices[i] = 0;
        }

        if (mCallableChanServices[i] != 0) {
            g_object_unref(mCallableChanServices[i]);
            mCallableChanServices[i] = 0;
        }
    }

    while (!observers[0]->channels().isEmpty() ||
           !observers[1]->channels().isEmpty() ||
           !textObservers[0]->textChats().isEmpty() ||
           !textObservers[1]->textChats().isEmpty() ||
           !textObserversNoContact[0]->textChats().isEmpty() ||
           !textObserversNoContact[1]->textChats().isEmpty() ||
           !callObservers[0]->streamedMediaCalls().isEmpty() ||
           !callObservers[1]->streamedMediaCalls().isEmpty() ||
           !callObserversNoContact[0]->streamedMediaCalls().isEmpty() ||
           !callObserversNoContact[1]->streamedMediaCalls().isEmpty()) {
        mLoop->processEvents();
    }

    QCOMPARE(mChannelsCount, 0);
    QCOMPARE(mSMChannelsCount, 0);
}

void TestSimpleObserver::cleanup()
{
    cleanupImpl();
}

void TestSimpleObserver::cleanupTestCase()
{
    for (int i = 0; i < 2; ++i) {
        if (!mConns[i].conn) {
            continue;
        }

        if (mConns[i].conn->requestedFeatures().contains(Connection::FeatureCore)) {
            QVERIFY(mConns[i].connService != NULL);

            if (TP_BASE_CONNECTION(mConns[i].connService)->status != TP_CONNECTION_STATUS_DISCONNECTED) {
                tp_base_connection_change_status(TP_BASE_CONNECTION(mConns[i].connService),
                        TP_CONNECTION_STATUS_DISCONNECTED,
                        TP_CONNECTION_STATUS_REASON_REQUESTED);
            }

            while (mConns[i].conn->isValid()) {
                mLoop->processEvents();
            }

        }
        mConns[i].conn.reset();

        mTextChans[i].reset();
        mSMChans[i].reset();

        if (mMessagesChanServices[i] != 0) {
            g_object_unref(mMessagesChanServices[i]);
            mMessagesChanServices[i] = 0;
        }

        if (mCallableChanServices[i] != 0) {
            g_object_unref(mCallableChanServices[i]);
            mCallableChanServices[i] = 0;
        }

        if (mConns[i].connService != 0) {
            mConns[i].baseConnService = 0;
            g_object_unref(mConns[i].connService);
            mConns[i].connService = 0;
        }
    }

    cleanupTestCaseImpl();
}

QMap<QString, QString> TestSimpleObserver::ourObservers()
{
    QStringList registeredNames =
        QDBusConnection::sessionBus().interface()->registeredServiceNames();
    QMap<QString, QString> observers;

    Q_FOREACH (QString name, registeredNames) {
        if (!name.startsWith(QLatin1String("org.freedesktop.Telepathy.Client.TpQtSO"))) {
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

        observers.insert(name, path);
    }

    return observers;
}

QTEST_MAIN(TestSimpleObserver)
#include "_gen/simple-observer.cpp.moc.hpp"
