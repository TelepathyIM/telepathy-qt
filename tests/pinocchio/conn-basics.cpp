#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingReady>

#include <tests/pinocchio/lib.h>

using Telepathy::Client::Connection;
using Telepathy::Client::ConnectionPtr;
using Telepathy::Client::ConnectionManagerInterface;
using Telepathy::Client::DBus::PeerInterface;
using Telepathy::Client::DBus::PropertiesInterface;
using Telepathy::Client::Features;

class TestConnBasics : public PinocchioTest
{
    Q_OBJECT

private:
    Telepathy::Client::ConnectionManagerInterface* mCM;
    QString mConnBusName;
    QString mConnObjectPath;
    ConnectionPtr mConn;

protected Q_SLOTS:
    void expectReady(uint, uint);

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

    mCM = new ConnectionManagerInterface(
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


void TestConnBasics::testInitialIntrospection()
{
    mConn = Connection::create(mConnBusName, mConnObjectPath);

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusUnknown));

    mConn.reset();
}


void TestConnBasics::expectReady(uint newStatus, uint newStatusReason)
{
    switch (newStatus) {
    case Connection::StatusDisconnected:
        qWarning() << "Disconnected";
        mLoop->exit(1);
        break;
    case Connection::StatusConnecting:
        /* do nothing */
        break;
    case Connection::StatusConnected:
        qDebug() << "Ready";
        mLoop->exit(0);
        break;
    default:
        qWarning().nospace() << "What sort of status is "
            << newStatus << "?!";
        mLoop->exit(2);
        break;
    }
}


void TestConnBasics::testConnect()
{
    mConn = Connection::create(mConnBusName, mConnObjectPath);
    QCOMPARE(mConn->isReady(), false);

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusUnknown));

    qDebug() << "calling Connect()";
    QVERIFY(connect(mConn->requestConnect(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mConn->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(expectReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(uint, uint)),
                           this,
                           SLOT(expectReady(uint, uint))));
        QCOMPARE(mConn->status(), (uint) Connection::StatusConnected);
    }

    Features features = Features() << Connection::FeatureSimplePresence;
    QVERIFY(connect(mConn->becomeReady(features),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);
    QVERIFY(mConn->missingFeatures() == features);

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusConnected));
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

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusDisconnected));
    QCOMPARE(static_cast<uint>(mConn->statusReason()),
        static_cast<uint>(Telepathy::ConnectionStatusReasonRequested));

    mConn.reset();
}


void TestConnBasics::testAlreadyConnected()
{
    mConn = Connection::create(mConnBusName, mConnObjectPath);

    qDebug() << "calling Connect()";
    QVERIFY(connect(mConn->requestConnect(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mConn->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(expectReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(uint, uint)),
                           this,
                           SLOT(expectReady(uint, uint))));
        QCOMPARE(mConn->status(), (uint) Connection::StatusConnected);
    }

    // delete proxy, make a new one
    mConn.reset();
    mConn = Connection::create(mConnBusName, mConnObjectPath);

    // Wait for introspection to run (readiness changes to Full immediately)
    QVERIFY(connect(mConn.data(), SIGNAL(statusChanged(uint, uint)),
          this, SLOT(expectReady(uint, uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn.data(), SIGNAL(statusChanged(uint, uint)),
          this, SLOT(expectReady(uint, uint))));

    QVERIFY(connect(mConn->requestDisconnect(),
          SIGNAL(finished(Telepathy::Client::PendingOperation*)),
          this,
          SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    mConn.reset();
}


void TestConnBasics::testInterfaceFactory()
{
    mConn = Connection::create(QDBusConnection::sessionBus(),
        mConnBusName, mConnObjectPath);

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusUnknown));

    PropertiesInterface* props = mConn->propertiesInterface();
    QVERIFY(props != NULL);

    PropertiesInterface* props2 =
        mConn->optionalInterface<PropertiesInterface>(Connection::BypassInterfaceCheck);
    QVERIFY(props2 == props);

    PeerInterface* notListed = mConn->optionalInterface<PeerInterface>();
    QVERIFY(notListed == NULL);
    notListed = mConn->optionalInterface<PeerInterface>(Connection::BypassInterfaceCheck);
    QVERIFY(notListed != NULL);

    mConn.reset();
}


void TestConnBasics::cleanup()
{
    cleanupImpl();
}


void TestConnBasics::testSpecifiedBus()
{
    mConn = Connection::create(QDBusConnection::sessionBus(),
        mConnBusName, mConnObjectPath);

    QCOMPARE(static_cast<uint>(mConn->status()),
        static_cast<uint>(Connection::StatusUnknown));

    mConn.reset();
}


void TestConnBasics::cleanupTestCase()
{
    delete mCM;
    mCM = NULL;

    cleanupTestCaseImpl();
}


QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
