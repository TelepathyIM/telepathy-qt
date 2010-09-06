#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

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
        emit AccountPropertyChanged(props);
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
          mDispatcher(0), mConnService1(0), mConnService2(0), mAccountAdaptor(0),
          mReceivedHaveConnection(0), mReceivedConn(0)
    { }

protected Q_SLOTS:
    void onHaveConnectionChanged(bool have);
    void expectPropertyChange(const QString &property);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreateAndIntrospect();
    void testDefaultFactoryInitialConn();
    void testReadifyingFactoryInitialConn();
    void testSwitch();
    void testQueuedSwitch();

    void cleanup();
    void cleanupTestCase();

private:
    QObject *mDispatcher;
    QString mAccountBusName, mAccountPath;
    TpTestsContactsConnection *mConnService1, *mConnService2;
    QString mConnPath1, mConnPath2;
    QString mConnName1, mConnName2;
    AccountAdaptor *mAccountAdaptor;
    AccountPtr mAccount;
    bool *mReceivedHaveConnection;
    QString *mReceivedConn;
    QStringList mReceivedConns;
};

void TestAccountConnectionFactory::onHaveConnectionChanged(bool have)
{
    qDebug() << "have connection:" << have;

    Q_ASSERT(!mReceivedHaveConnection);
    mReceivedHaveConnection = new bool(have);
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
        QCOMPARE(mAccount->connectionObjectPath(), conn->objectPath());
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

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService1 = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me1@example.com",
            "protocol", "simple",
            NULL));
    QVERIFY(mConnService1 != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService1),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName1 = QLatin1String(name);
    mConnPath1 = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConnService2 = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me2@example.com",
            "protocol", "simple",
            NULL));
    QVERIFY(mConnService2 != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService2),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName2 = QLatin1String(name);
    mConnPath2 = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mAccountBusName = QLatin1String(TELEPATHY_INTERFACE_ACCOUNT_MANAGER);
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
    mAccountAdaptor->setConnection(mConnPath1);

    mAccount = Account::create(mAccountBusName, mAccountPath);

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mAccount->connectionObjectPath(), mConnPath1);
    QVERIFY(!mAccount->connection().isNull());

    QCOMPARE(mAccount->connectionFactory()->features(), Features());
}

void TestAccountConnectionFactory::testReadifyingFactoryInitialConn()
{
    mAccountAdaptor->setConnection(mConnPath1);

    mAccount = Account::create(mAccountBusName, mAccountPath,
            ConnectionFactory::create(QDBusConnection::sessionBus(),
                Connection::FeatureCore),
            ChannelFactory::stockFreshFactory(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mAccount->connectionObjectPath(), mConnPath1);

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
            ChannelFactory::stockFreshFactory(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mAccount->connectionObjectPath().isEmpty());
    QVERIFY(!mAccount->haveConnection());

    QVERIFY(connect(mAccount.data(),
                SIGNAL(haveConnectionChanged(bool)),
                SLOT(onHaveConnectionChanged(bool))));

    QVERIFY(connect(mAccount.data(),
                SIGNAL(propertyChanged(QString)),
                SLOT(expectPropertyChange(QString))));

    // Switch from none to conn 1
    mAccountAdaptor->setConnection(mConnPath1);
    while (!mReceivedHaveConnection || !mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedHaveConnection, true);
    QCOMPARE(*mReceivedConn, mConnPath1);

    delete mReceivedHaveConnection;
    mReceivedHaveConnection = 0;

    delete mReceivedConn;
    mReceivedConn = 0;

    QCOMPARE(mAccount->connectionObjectPath(), mConnPath1);
    QVERIFY(mAccount->haveConnection());

    ConnectionPtr conn = mAccount->connection();
    QVERIFY(!conn.isNull());
    QCOMPARE(conn->objectPath(), mConnPath1);

    QVERIFY(conn->isReady(Connection::FeatureCore));

    // Switch from conn 1 to conn 2
    mAccountAdaptor->setConnection(mConnPath2);
    while (!mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedConn, mConnPath2);

    delete mReceivedConn;
    mReceivedConn = 0;

    // haveConnectionChanged() shouldn't have been emitted as it was true -> true
    QVERIFY(!mReceivedHaveConnection);
    QVERIFY(mAccount->haveConnection());

    QCOMPARE(mAccount->connectionObjectPath(), mConnPath2);

    conn = mAccount->connection();
    QVERIFY(!conn.isNull());
    QCOMPARE(conn->objectPath(), mConnPath2);

    QVERIFY(conn->isReady(Connection::FeatureCore));

    // Switch from conn 2 to none
    mAccountAdaptor->setConnection(QString());
    while (!mReceivedHaveConnection || !mReceivedConn) {
        mLoop->processEvents();
    }
    QCOMPARE(*mReceivedConn, QString());
    QCOMPARE(*mReceivedHaveConnection, false);

    QVERIFY(mAccount->connectionObjectPath().isEmpty());
    QVERIFY(mAccount->connection().isNull());

    QVERIFY(!mAccount->haveConnection());
}

void TestAccountConnectionFactory::testQueuedSwitch()
{
    mAccount = Account::create(mAccountBusName, mAccountPath,
            ConnectionFactory::create(QDBusConnection::sessionBus(),
                Connection::FeatureCore),
            ChannelFactory::stockFreshFactory(QDBusConnection::sessionBus()));

    QVERIFY(connect(mAccount->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mAccount->connectionObjectPath().isEmpty());
    QVERIFY(!mAccount->haveConnection());

    QVERIFY(connect(mAccount.data(),
                SIGNAL(propertyChanged(QString)),
                SLOT(expectPropertyChange(QString))));

    // Switch a few times but don't give the proxy update machinery time to run
    mAccountAdaptor->setConnection(mConnPath1);
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConnPath2);
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConnPath2);
    mAccountAdaptor->setConnection(QString());
    mAccountAdaptor->setConnection(mConnPath2);
    mAccountAdaptor->setConnection(mConnPath2);
    mAccountAdaptor->setConnection(mConnPath1);

    // We should get a total of 8 changes because some of them aren't actually any different
    while (mReceivedConns.size() < 8) {
        mLoop->processEvents();
    }
    // To ensure it didn't go over, which might be possible if it handled two events in one iter
    QCOMPARE(mReceivedConns.size(), 8);

    // Ensure we got them in the correct order
    QCOMPARE(mReceivedConns, QStringList() << mConnPath1
                                           << QString()
                                           << mConnPath2
                                           << QString()
                                           << mConnPath2
                                           << QString()
                                           << mConnPath2
                                           << mConnPath1);

    // Check that the final state is correct
    QCOMPARE(mAccount->connectionObjectPath(), mConnPath1);
    QVERIFY(mAccount->haveConnection());
    QVERIFY(!mAccount->connection().isNull());
    QCOMPARE(mAccount->connection()->objectPath(), mConnPath1);
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
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountConnectionFactory)
#include "_gen/account-connection-factory.cpp.moc.hpp"
