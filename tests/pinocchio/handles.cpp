#include <list>

#include <QDebug>
#include <QStringList>

#include <QDBusPendingCallWatcher>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusPendingReply>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>

#include <tests/pinocchio/lib.h>

using namespace Telepathy;
using namespace Telepathy::Client;

class TestHandles : public PinocchioTest
{
    Q_OBJECT

private:
    ConnectionManagerInterface *mCM;

    // Bus connection 1, proxy a
    ConnectionPtr mConn1a;
    // Bus connection 1, proxy b
    ConnectionPtr mConn1b;
    // Bus connection 2
    ConnectionPtr mConn2;

    // Temporary storage to move ReferencedHandles away from their self-destructive parents in the
    // finished() handlers
    ReferencedHandles mHandles;

protected Q_SLOTS:
    // these ought to be private, but if they were, QTest would think they
    // were test cases. So, they're protected instead
    void expectConnReady(uint, uint);
    void expectPendingHandlesFinished(Telepathy::PendingOperation*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();
    void testReferences();

    void cleanup();
    void cleanupTestCase();
};

void TestHandles::expectConnReady(uint newStatus, uint newStatusReason)
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
        qWarning().nospace() << "What sort of readiness is "
            << newStatus << "?!";
        mLoop->exit(2);
        break;
    }
}

void TestHandles::initTestCase()
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
    QString busName = reply.argumentAt<0>();
    QString objectPath = reply.argumentAt<1>().path();

    // Get a few connections connected
    mConn1a = Connection::create(busName, objectPath);
    mConn1b = Connection::create(busName, objectPath);
    QDBusConnection privateBus =
        QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                "tpqt4_handles_test_private_bus");
    mConn2 = Connection::create(privateBus, busName, objectPath);

    // Connecting one connects them all
    QVERIFY(connect(mConn1a->requestConnect(),
            SIGNAL(finished(Telepathy::PendingOperation*)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    ConnectionPtr connections[3] = {mConn1a, mConn1b, mConn2};
    for (int i = 0; i < 3; ++i) {
        ConnectionPtr conn = connections[i];

        if (conn->status() == Connection::StatusConnected) {
            qDebug() << conn << "Already ready";
            continue;
        }

        QVERIFY(connect(conn.data(), SIGNAL(statusChanged(uint, uint)),
                    this, SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(conn.data(), SIGNAL(statusChanged(uint, uint)),
                    this, SLOT(expectConnReady(uint, uint))));
    }
}

void TestHandles::init()
{
    initImpl();
}

void TestHandles::expectPendingHandlesFinished(PendingOperation *op)
{
    if (!op->isFinished()) {
        qWarning() << "unfinished";
        mLoop->exit(1);
        return;
    }

    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(2);
        return;
    }

    if (!op->isValid()) {
        qWarning() << "inconsistent results";
        mLoop->exit(3);
        return;
    }

    qDebug() << "finished";
    PendingHandles *pending = qobject_cast<PendingHandles*>(op);
    mHandles = pending->handles();
    mLoop->exit(0);
}

void TestHandles::testBasics()
{
    // Get a reference to compare against (synchronously, don't do this in real applications)
    QStringList ids = QStringList() << "friend" << "buddy" << "associate" << "dude" << "guy";
    ConnectionInterface iface(mConn1a->busName(), mConn1a->objectPath());
    Telepathy::UIntList shouldBe = iface.RequestHandles(Telepathy::HandleTypeContact, ids);

    // Try and get the same handles asynchronously using the convenience API
    PendingHandles *pending = mConn1a->requestHandles(Telepathy::HandleTypeContact, ids);

    // Check that the closure is consistent with what we asked for
    QVERIFY(pending->isRequest());
    QCOMPARE(pending->namesRequested(), ids);
    QCOMPARE(pending->connection(), mConn1a);
    QCOMPARE(pending->handleType(), static_cast<uint>(Telepathy::HandleTypeContact));

    // Finish the request and extract the resulting ReferencedHandles
    QVERIFY(connect(pending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    ReferencedHandles handles = mHandles;
    mHandles = ReferencedHandles();

    // Check that the ReferencedHandles are what we asked for
    QCOMPARE(handles.connection(), mConn1a);
    QCOMPARE(handles.handleType(), static_cast<uint>(Telepathy::HandleTypeContact));
    QVERIFY(handles == shouldBe);

    // Check that a copy of the received ReferencedHandles is also what we asked for (it's supposed
    // to be equivalent with one that we already verified as being that)
    ReferencedHandles copy = handles;
    QCOMPARE(copy.connection(), mConn1a);
    QCOMPARE(copy.handleType(), static_cast<uint>(Telepathy::HandleTypeContact));

    QVERIFY(copy == handles);
    QVERIFY(copy == shouldBe);
}

void TestHandles::testReferences()
{
    // Used for verifying the handles we get actually work and continue to do so after various
    // operations which are supposed to preserve them
    ConnectionInterface iface(mConn1a->busName(), mConn1a->objectPath());

    // Declare some IDs to use as a test case
    QStringList ids = QStringList() << "mate" << "contact" << "partner" << "bloke" << "fellow";

    // Get referenced handles for all 5 of the IDs
    PendingHandles *allPending = mConn1a->requestHandles(Telepathy::HandleTypeContact, ids);
    QVERIFY(connect(allPending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(allPending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    ReferencedHandles allHandles = mHandles;
    mHandles = ReferencedHandles();

    // Check that we actually have 5 handles
    QCOMPARE(allHandles.size(), 5);

    // ... and that they're valid at this point by inspecting them
    QDBusReply<QStringList> inspectReply = iface.InspectHandles(Telepathy::HandleTypeContact,
            Telepathy::UIntList::fromStdList(std::list<uint>(allHandles.begin(),
                    allHandles.end())));
    QVERIFY(inspectReply.isValid());
    QCOMPARE(inspectReply.value().size(), 5);

    // Get another fresh reference to the middle three using the Connection
    PendingHandles *middlePending = mConn1a->referenceHandles(Telepathy::HandleTypeContact,
            Telepathy::UIntList() << allHandles[1] << allHandles[2] << allHandles[3]);
    QVERIFY(connect(middlePending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(middlePending, SIGNAL(finished(Telepathy::PendingOperation*)),
          this, SLOT(expectPendingHandlesFinished(Telepathy::PendingOperation*))));
    ReferencedHandles middleHandles = mHandles;
    mHandles = ReferencedHandles();

    // ... and another reference to the last three using ReferencedHandles RAII magic
    ReferencedHandles lastHandles = allHandles.mid(2);

    // Check that they actually contain the right handles
    QCOMPARE(middleHandles.size(), 3);
    QCOMPARE(lastHandles.size(), 3);

    QCOMPARE(middleHandles[0], allHandles[1]);
    QCOMPARE(middleHandles[1], allHandles[2]);
    QCOMPARE(middleHandles[2], allHandles[3]);

    QCOMPARE(lastHandles[0], allHandles[2]);
    QCOMPARE(lastHandles[1], allHandles[3]);
    QCOMPARE(lastHandles[2], allHandles[4]);

    // Ok, so at this point they're valid handles, because they're the same we already checked as
    // being valid - but what if we nuke the original ReferencedHandles containing all of the
    // handles? Let's save its first one though...
    uint firstHandle = allHandles.first();
    allHandles = ReferencedHandles();

    // Let's process the now-queued events first so what's going to be released is released
    mLoop->processEvents();

    // Now check that our middle and last handles can still be inspected
    inspectReply = iface.InspectHandles(Telepathy::HandleTypeContact,
            Telepathy::UIntList::fromStdList(std::list<uint>(middleHandles.begin(),
                    middleHandles.end())));
    QVERIFY(inspectReply.isValid());
    QCOMPARE(inspectReply.value().size(), 3);

    inspectReply = iface.InspectHandles(Telepathy::HandleTypeContact,
            Telepathy::UIntList::fromStdList(std::list<uint>(lastHandles.begin(),
                    lastHandles.end())));
    QVERIFY(inspectReply.isValid());
    QCOMPARE(inspectReply.value().size(), 3);

    // Because we know that in this self-contained test, nobody else can possibly be holding the
    // first handle, and we have dropped the last ReferencedHandles having it, it should be invalid
    //
    // However, the telepathy-python 0.15.3 ReleaseHandles implementation is made of cheese. I know
    // how to fix it, but until we've released tp-python with the fix, and added a dependency on
    // that new version of tp-python for the tests, we can't enable this.
    // inspectReply = iface.InspectHandles(Telepathy::HandleTypeContact,
    //        Telepathy::UIntList() << firstHandle);
    // QVERIFY(!inspectReply.isValid());
    Q_UNUSED(firstHandle);
}

void TestHandles::cleanup()
{
    cleanupImpl();
}

void TestHandles::cleanupTestCase()
{
    // Disconnecting one disconnects them all, because they all poke the same
    // remote Connection
    QVERIFY(connect(mConn1a->requestDisconnect(),
          SIGNAL(finished(Telepathy::PendingOperation*)),
          this,
          SLOT(expectSuccessfulCall(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    delete mCM;
    mCM = NULL;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestHandles)
#include "_gen/handles.cpp.moc.hpp"
