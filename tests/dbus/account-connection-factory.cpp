#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingReady>

#include <telepathy-glib/debug.h>

using namespace Tp;
using namespace Tp::Client;

// A really minimal Account implementation, totally not spec compliant outside the parts stressed by
// this test
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

class TestAccountConnectionFactory : public Test
{
    Q_OBJECT

public:
    TestAccountConnectionFactory(QObject *parent = 0)
        : Test(parent),
          mConn1(0), mConn2(0),
          mDispatcher(0), mAccountAdaptor(0),
          mReceivedHaveConnection(0), mReceivedConn(0)
    { }

protected Q_SLOTS:
    void onConnectionChanged(const Tp::ConnectionPtr &conn);
    void expectPropertyChange(const QString &property);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testIntrospectSeveralAccounts();
    void testCreateAndIntrospect();
    void testDefaultFactoryInitialConn();
    void testReadifyingFactoryInitialConn();
    void testSwitch();
    void testQueuedSwitch();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn1, *mConn2;
    QObject *mDispatcher;
    QString mAccountBusName, mAccountPath;
    AccountAdaptor *mAccountAdaptor;
    AccountPtr mAccount;
    bool *mReceivedHaveConnection;
    QString *mReceivedConn;
    QStringList mReceivedConns;
};

void TestAccountConnectionFactory::onConnectionChanged(const Tp::ConnectionPtr &conn)
{
    qDebug() << "have connection:" << !conn.isNull();

    if (mReceivedHaveConnection) {
        delete mReceivedHaveConnection;
    }

    mReceivedHaveConnection = new bool(!conn.isNull());
}

void TestAccountConnectionFactory::expectPropertyChange(const QString &property)
{
    if (property != QLatin1String("connection")) {
        // Not interesting
        return;
    }

    ConnectionPtr conn = mAccount->connection();
    qDebug() << "connection changed:" << (conn ? conn->objectPath() : QLatin1String("none"));

    if (conn) {
        QCOMPARE(conn->objectPath(), conn->objectPath());
    }

    if (mReceivedConn) {
        delete mReceivedConn;
    }

    mReceivedConn = new QString(conn ? conn->objectPath() : QLatin1String(""));
    mReceivedConns.push_back(conn ? conn->objectPath() : QLatin1String(""));
}

void TestAccountConnectionFactory::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("account-connection-factory");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn1 = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "simple",
            NULL);
    QCOMPARE(mConn1->isReady(), false);

    mConn2 = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me2@example.com",
            "protocol", "simple",
            NULL);
    QCOMPARE(mConn2->isReady(), false);

    mAccountBusName = TP_QT_IFACE_ACCOUNT_MANAGER;
    mAccountPath = QLatin1String("/org/freedesktop/Telepathy/Account/simple/simple/account");
}

void TestAccountConnectionFactory::init()
{
    initImpl();

    mDispatcher = new QObject(this);

    mAccountAdaptor = new AccountAdaptor(mDispatcher);

    QDBusConnection bus = QDBusConnection::sessionBus();
    QVERIFY(bus.registerService(mAccountBusName));
    QVERIFY(bus.registerObject(mAccountPath, mDispatcher));
}

// If this test fails, probably the code which tries to introspect the CD just once and then
// continue with Account introspection has a bug
void TestAccountConnectionFactory::testIntrospectSeveralAccounts()
{
    QList<PendingOperation *> ops;
    for (int i = 0; i < 10; i++) {
        AccountPtr acc = Account::create(mAccountBusName, mAccountPath);

        // This'll get the CD introspected in the middle (but won't finish any of the pending ops,
        // as they'll only finish in a singleShot in the next iter)
        //
        // One iteration to get readinessHelper to start introspecting,
        // the second    to download the CD property
        // the third     to get PendingVariant to actually emit the finished signal for it
        if (i == 5) {
            mLoop->processEvents();
            mLoop->processEvents();
            mLoop->processEvents();
        }

        ops.push_back(acc->becomeReady());
    }

    QVERIFY(connect(new PendingComposite(ops, SharedPtr<RefCounted>()),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

// If this test fails, probably the mini-Account implements too little for the Account proxy to work
// OR the Account proxy is completely broken :)
void TestAccountConnectionFactory::testCreateAndIntrospect()
{
    mAccount = Account::create(mAccountBusName, mAccountPath);

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestAccountConnectionFactory::testDefaultFactoryInitialConn()
{
    mAccountAdaptor->setConnection(mConn1->objectPath());

    mAccount = Account::create(mAccountBusName, mAccountPath);

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mAccount->connection()->objectPath(), mConn1->objectPath());
    QVERIFY(!mAccount->connection().isNull());

    QCOMPARE(mAccount->connectionFactory()->features(), Features());
}

void TestAccountConnectionFactory::testReadifyingFactoryInitialConn()
{
    mAccountAdaptor->setConnection(mConn1->objectPath());

    mAccount = Account::create(mAccountBusName, mAccountPath,
            ConnectionFactory::create(QDBusConnection::sessionBus(),
                Connection::FeatureCore),
            ChannelFactory::create(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mAccount->connection()->objectPath(), mConn1->objectPath());

    ConnectionPtr conn = mAccount->connection();
    QVERIFY(!conn.isNull());

    QVERIFY(conn->isReady(Connection::FeatureCore));

    QCOMPARE(mAccount->connectionFactory()->features(), Features(Connection::FeatureCore));
}

void TestAccountConnectionFactory::testSwitch()
{
    mAccount = Account::create(mAccountBusName, mAccountPath,
            ConnectionFactory::create(QDBusConnection::sessionBus(),
                Connection::FeatureCore),
            ChannelFactory::create(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mAccount->connection().isNull());

    QVERIFY(connect(mAccount.data(),
                SIGNAL(connectionChanged(const Tp::ConnectionPtr &)),
                SLOT(onConnectionChanged(const Tp::ConnectionPtr &))));

    QVERIFY(connect(mAccount.data(),
                SIGNAL(propertyChanged(QString)),
                SLOT(expectPropertyChange(QString))));

    // Switch from none to conn 1
    mAccountAdaptor->setConnection(mConn1->objectPath());
    while (!mReceivedHaveConnection || !mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedHaveConnection, true);
    QCOMPARE(*mReceivedConn, mConn1->objectPath());

    delete mReceivedHaveConnection;
    mReceivedHaveConnection = 0;

    delete mReceivedConn;
    mReceivedConn = 0;

    QCOMPARE(mAccount->connection()->objectPath(), mConn1->objectPath());
    QVERIFY(!mAccount->connection().isNull());

    ConnectionPtr conn = mAccount->connection();
    QVERIFY(!conn.isNull());
    QCOMPARE(conn->objectPath(), mConn1->objectPath());

    QVERIFY(conn->isReady(Connection::FeatureCore));

    // Switch from conn 1 to conn 2
    mAccountAdaptor->setConnection(mConn2->objectPath());
    while (!mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedConn, mConn2->objectPath());

    delete mReceivedConn;
    mReceivedConn = 0;

    // connectionChanged() should have been emitted as it is a new connection
    QVERIFY(mReceivedHaveConnection);
    QVERIFY(!mAccount->connection().isNull());

    QCOMPARE(mAccount->connection()->objectPath(), mConn2->objectPath());

    conn = mAccount->connection();
    QVERIFY(!conn.isNull());
    QCOMPARE(conn->objectPath(), mConn2->objectPath());

    QVERIFY(conn->isReady(Connection::FeatureCore));

    // Switch from conn 2 to none
    mAccountAdaptor->setConnection(QString());
    while (!mReceivedHaveConnection || !mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedConn, QString());
    QCOMPARE(*mReceivedHaveConnection, false);

    QVERIFY(mAccount->connection().isNull());
}

void TestAccountConnectionFactory::testQueuedSwitch()
{
    mAccount = Account::create(mAccountBusName, mAccountPath,
            ConnectionFactory::create(QDBusConnection::sessionBus(),
                Connection::FeatureCore),
            ChannelFactory::create(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mAccount->connection().isNull());

    QVERIFY(connect(mAccount.data(),
                SIGNAL(propertyChanged(QString)),
                SLOT(expectPropertyChange(QString))));

    // Switch a few times but don't give the proxy update machinery time to run
    mAccountAdaptor->setConnection(mConn1->objectPath());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConn2->objectPath());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConn2->objectPath());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConn2->objectPath());
    mAccountAdaptor->setConnection(mConn2->objectPath());
    mAccountAdaptor->setConnection(mConn1->objectPath());

    // We should get a total of 8 changes because some of them aren't actually any different
    while (mReceivedConns.size() < 8) {
        mLoop->processEvents();
    }
    // To ensure it didn't go over, which might be possible if it handled two events in one iter
    QCOMPARE(mReceivedConns.size(), 8);

    // Ensure we got them in the correct order
    QCOMPARE(mReceivedConns, QStringList() << mConn1->objectPath()
                                           << QString()
                                           << mConn2->objectPath()
                                           << QString()
                                           << mConn2->objectPath()
                                           << QString()
                                           << mConn2->objectPath()
                                           << mConn1->objectPath());

    // Check that the final state is correct
    QCOMPARE(mAccount->connection()->objectPath(), mConn1->objectPath());
    QVERIFY(!mAccount->connection().isNull());
}

void TestAccountConnectionFactory::cleanup()
{
    mAccount.reset();

    if (mReceivedHaveConnection) {
        delete mReceivedHaveConnection;
        mReceivedHaveConnection = 0;
    }

    if (mReceivedConn) {
        delete mReceivedConn;
        mReceivedConn = 0;
    }

    if (mAccountAdaptor) {
        delete mAccountAdaptor;
        mAccountAdaptor = 0;
    }

    if (mDispatcher) {
        delete mDispatcher;
        mDispatcher = 0;
    }

    mReceivedConns.clear();

    cleanupImpl();
}

void TestAccountConnectionFactory::cleanupTestCase()
{
    if (mConn1) {
        QCOMPARE(mConn1->disconnect(), true);
        delete mConn1;
    }

    if (mConn2) {
        QCOMPARE(mConn2->disconnect(), true);
        delete mConn2;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountConnectionFactory)
#include "_gen/account-connection-factory.cpp.moc.hpp"
