#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ConnectionManager>

#include <tests/pinocchio/lib.h>

using namespace Telepathy::Client;

class TestConnBasics : public PinocchioTest
{
    Q_OBJECT

private:
    Telepathy::Client::ConnectionManagerInterface* mCM;
    QString mConnBusName;
    QString mConnObjectPath;
    Connection *mConn;

protected Q_SLOTS:
    void expectNotYetConnected(uint);
    void expectReady(uint);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testInitialIntrospection();
    void testConnect();
    void testSpecifiedBus();
    void testAlreadyConnected();
    void testInterfaceFactory();

    void cleanup();
    void cleanupTestCase();
};


/*
 * Missing test coverage on existing Connection code includes:
 *
 * - pre-Connected introspection (needs Pinocchio support or another CM)
 * - introspecting a Connection that's already Connecting (needs Pinocchio
 *   support or another CM)
 *
 * Out of scope for this test, should be in another test:
 *
 * - SimplePresence introspection (needs Pinocchio support or another CM)
 * - aliasFlags(), presenceStatuses(), simplePresenceStatuses() accessors
 * - requesting a channel
 */


void TestConnBasics::initTestCase()
{
    initTestCaseImpl();

    // Wait for the CM to start up
    QVERIFY(waitForPinocchio(5000));

    // Escape to the low-level API to make a Connection; this uses
    // pseudo-blocking calls for simplicity. Do not do this in production code

    mCM = new Telepathy::Client::ConnectionManagerInterface(
        pinocchioBusName(), pinocchioObjectPath());

    QDBusPendingReply<QString, QDBusObjectPath> reply;
    QVariantMap parameters;
    parameters.insert(QLatin1String("account"),
        QVariant::fromValue(QString::fromAscii("empty")));
    parameters.insert(QLatin1String("password"),
        QVariant::fromValue(QString::fromAscii("s3kr1t")));

    reply = mCM->RequestConnection("dummy", parameters);
    reply.waitForFinished();
    if (!reply.isValid()) {
        qWarning().nospace() << reply.error().name()
            << ": " << reply.error().message();
        QVERIFY(reply.isValid());
    }
    mConnBusName = reply.argumentAt<0>();
    mConnObjectPath = reply.argumentAt<1>().path();
}


void TestConnBasics::init()
{
    initImpl();
}


void TestConnBasics::expectNotYetConnected(uint newReadiness)
{
    switch (newReadiness) {
    case Connection::ReadinessJustCreated:
        qWarning() << "Changing from JustCreated to JustCreated is silly";
        mLoop->exit(1);
        break;
    case Connection::ReadinessNotYetConnected:
        qDebug() << "Correctly changed to NotYetConnected";
        mLoop->exit(0);
        break;
    case Connection::ReadinessConnecting:
    case Connection::ReadinessFull:
    case Connection::ReadinessDead:
        qWarning() << "Got too far!";
        mLoop->exit(2);
        break;
    default:
        qWarning().nospace() << "What sort of readiness is "
            << newReadiness << "?!";
        mLoop->exit(3);
        break;
    }
}


void TestConnBasics::testInitialIntrospection()
{
    mConn = new Connection(mConnBusName, mConnObjectPath);

    QCOMPARE(mConn->readiness(), Connection::ReadinessJustCreated);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusDisconnected));

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));

    delete mConn;
    mConn = NULL;
}


void TestConnBasics::expectReady(uint newReadiness)
{
    switch (newReadiness) {
    case Connection::ReadinessJustCreated:
        qWarning() << "Changing from NYC to JustCreated is silly";
        mLoop->exit(1);
        break;
    case Connection::ReadinessNotYetConnected:
        qWarning() << "Changing from NYC to NYC is silly";
        mLoop->exit(2);
        break;
    case Connection::ReadinessConnecting:
        /* do nothing */
        break;
    case Connection::ReadinessFull:
        qDebug() << "Ready";
        mLoop->exit(0);
        break;
    case Connection::ReadinessDead:
        qWarning() << "Dead!";
        mLoop->exit(3);
        break;
    default:
        qWarning().nospace() << "What sort of readiness is "
            << newReadiness << "?!";
        mLoop->exit(4);
        break;
    }
}


void TestConnBasics::testConnect()
{
    mConn = new Connection(mConnBusName, mConnObjectPath);

    QCOMPARE(mConn->readiness(), Connection::ReadinessJustCreated);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusDisconnected));

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));

    // FIXME: should have convenience API
    qDebug() << "calling Connect()";

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
            mConn->Connect());
    QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher*))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    // Wait for readiness to reach Full

    qDebug() << "waiting for Full readiness";
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));

    QCOMPARE(mConn->readiness(), Connection::ReadinessFull);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusConnected));
    QCOMPARE(static_cast<uint>(mConn->statusReason()),
        static_cast<uint>(Telepathy::ConnectionStatusReasonRequested));

    QStringList interfaces = mConn->interfaces();
    QVERIFY(interfaces.contains(QLatin1String(
            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_ALIASING)));
    QVERIFY(interfaces.contains(QLatin1String(
            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_AVATARS)));
    QVERIFY(interfaces.contains(QLatin1String(
            TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CAPABILITIES)));

    QVERIFY(connect(mConn->requestDisconnect(),
          SIGNAL(finished(Telepathy::Client::PendingOperation*)),
          this,
          SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mConn->readiness(), Connection::ReadinessDead);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusDisconnected));
    QCOMPARE(static_cast<uint>(mConn->statusReason()),
        static_cast<uint>(Telepathy::ConnectionStatusReasonRequested));

    delete mConn;
    mConn = NULL;
}


void TestConnBasics::testAlreadyConnected()
{
    mConn = new Connection(mConnBusName, mConnObjectPath);

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));

    // FIXME: should have convenience API
    qDebug() << "calling Connect()";

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
            mConn->Connect());
    QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher*))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    // Wait for readiness to reach Full

    qDebug() << "waiting for Full readiness";
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));

    QCOMPARE(mConn->readiness(), Connection::ReadinessFull);

    // delete proxy, make a new one
    delete mConn;
    mConn = new Connection(mConnBusName, mConnObjectPath);

    // Wait for introspection to run (readiness changes to Full immediately)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectReady(uint))));

    watcher = new QDBusPendingCallWatcher(
            mConn->Disconnect());
    QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher*))));
    QCOMPARE(mLoop->exec(), 0);
    delete watcher;

    delete mConn;
    mConn = NULL;
}


void TestConnBasics::testInterfaceFactory()
{
    mConn = new Connection(QDBusConnection::sessionBus(),
        mConnBusName, mConnObjectPath);

    QCOMPARE(mConn->readiness(), Connection::ReadinessJustCreated);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusDisconnected));

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));

    DBus::PropertiesInterface* props = mConn->propertiesInterface();
    QVERIFY(props != NULL);

    DBus::PropertiesInterface* props2 =
        mConn->optionalInterface<DBus::PropertiesInterface>(Connection::BypassInterfaceCheck);
    QVERIFY(props2 == props);

    DBus::PeerInterface* notListed = mConn->optionalInterface<DBus::PeerInterface>();
    QVERIFY(notListed == NULL);
    notListed = mConn->optionalInterface<DBus::PeerInterface>(Connection::BypassInterfaceCheck);
    QVERIFY(notListed != NULL);

    delete mConn;
    mConn = NULL;
}


void TestConnBasics::cleanup()
{
    cleanupImpl();
}


void TestConnBasics::testSpecifiedBus()
{
    mConn = new Connection(QDBusConnection::sessionBus(),
        mConnBusName, mConnObjectPath);

    QCOMPARE(mConn->readiness(), Connection::ReadinessJustCreated);
    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Telepathy::ConnectionStatusDisconnected));

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));

    delete mConn;
    mConn = NULL;
}


void TestConnBasics::cleanupTestCase()
{
    delete mCM;
    mCM = NULL;

    cleanupTestCaseImpl();
}


QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
