#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

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
    void expectSuccessfulCall(QDBusPendingCallWatcher*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testInitialIntrospection();
    void testConnect();

    void cleanup();
    void cleanupTestCase();
};


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

    mConn = new Connection(mConnBusName, mConnObjectPath);
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
    QCOMPARE(mConn->readiness(), Connection::ReadinessJustCreated);

    // Wait for introspection to run (readiness changes to NYC)
    QVERIFY(connect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mConn, SIGNAL(readinessChanged(uint)),
          this, SLOT(expectNotYetConnected(uint))));
}


void TestConnBasics::expectSuccessfulCall(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        qWarning().nospace() << reply.error().name()
            << ": " << reply.error().message();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
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
    testInitialIntrospection();

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
}


void TestConnBasics::cleanup()
{
    delete mConn;
    mConn = NULL;

    cleanupImpl();
}


void TestConnBasics::cleanupTestCase()
{
    delete mCM;
    mCM = NULL;

    cleanupTestCaseImpl();
}


QTEST_MAIN(TestConnBasics)
#include "_gen/conn-basics.cpp.moc.hpp"
