#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/echo2/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestChanBasics : public Test
{
    Q_OBJECT

public:
    TestChanBasics(QObject *parent = 0)
        : Test(parent), mConnService(0), mHandle(0)
    { }

protected Q_SLOTS:
    void expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectPendingHandleFinished(Tp::PendingOperation*);
    void expectCreateChannelFinished(Tp::PendingOperation *);
    void expectEnsureChannelFinished(Tp::PendingOperation *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestHandle();
    void testCreateChannel();
    void testEnsureChannel();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    ExampleEcho2Connection *mConnService;
    ConnectionPtr mConn;
    ChannelPtr mChan;
    QString mChanObjectPath;
    uint mHandle;
};

void TestChanBasics::expectConnReady(Tp::Connection::Status newStatus,
        Tp::ConnectionStatusReason newStatusReason)
{
    qDebug() << "connection changed to status" << newStatus;
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

void TestChanBasics::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestChanBasics::expectPendingHandleFinished(PendingOperation *op)
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
    mHandle = pending->handles().at(0);
    mLoop->exit(0);
}

void TestChanBasics::expectCreateChannelFinished(PendingOperation* op)
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

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = pc->channel();
    mChanObjectPath = mChan->objectPath();
    mLoop->exit(0);
}

void TestChanBasics::expectEnsureChannelFinished(PendingOperation* op)
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

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = pc->channel();
    QCOMPARE(pc->yours(), false);
    QCOMPARE(mChan->objectPath(), mChanObjectPath);
    mLoop->exit(0);
}

void TestChanBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("conn-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_ECHO_2_CONNECTION(g_object_new(
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "contacts", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    Features features = Features() << Connection::FeatureSelfContact;
    QVERIFY(connect(mConn->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(features), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn.data(),
                        SIGNAL(statusChanged(Tp::Connection::Status, Tp::ConnectionStatusReason)),
                        SLOT(expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn.data(),
                           SIGNAL(statusChanged(Tp::Connection::Status, Tp::ConnectionStatusReason)),
                           this,
                           SLOT(expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason))));
        QCOMPARE(mConn->status(), Connection::StatusConnected);
    }

    QVERIFY(mConn->requestsInterface() != 0);
}

void TestChanBasics::init()
{
    initImpl();
}

void TestChanBasics::testRequestHandle()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->requestHandles(Tp::HandleTypeContact, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandleFinished(Tp::PendingOperation*))));
    QVERIFY(mHandle != 0);
}

void TestChanBasics::testCreateChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    if (mChan) {
        QVERIFY(connect(mChan->becomeReady(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(), true);
        QCOMPARE(mChan->isRequested(), true);
        QCOMPARE(mChan->groupCanAddContacts(), false);
        QCOMPARE(mChan->groupCanRemoveContacts(), false);
        QCOMPARE(mChan->initiatorContact()->id(), QString(QLatin1String("me@example.com")));
        QCOMPARE(mChan->groupSelfContact()->id(), QString(QLatin1String("me@example.com")));
        QCOMPARE(mChan->groupSelfContact(), mConn->selfContact());

        QStringList ids;
        foreach (const ContactPtr &contact, mChan->groupContacts()) {
            ids << contact->id();
        }
        ids.sort();
        QStringList toCheck = QStringList() << QLatin1String("me@example.com")
            << QLatin1String("alice");
        toCheck.sort();
        QCOMPARE(ids, toCheck);

        mChan.reset();
    }
}

void TestChanBasics::testEnsureChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mHandle);
    QVERIFY(connect(mConn->ensureChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectEnsureChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    if (mChan) {
        QVERIFY(connect(mChan->becomeReady(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(), true);
        QCOMPARE(mChan->isRequested(), true);
        QCOMPARE(mChan->groupCanAddContacts(), false);
        QCOMPARE(mChan->groupCanRemoveContacts(), false);
        QCOMPARE(mChan->initiatorContact()->id(), QString(QLatin1String("me@example.com")));
        QCOMPARE(mChan->groupSelfContact()->id(), QString(QLatin1String("me@example.com")));
        QCOMPARE(mChan->groupSelfContact(), mConn->selfContact());

        QStringList ids;
        foreach (const ContactPtr &contact, mChan->groupContacts()) {
            ids << contact->id();
        }
        ids.sort();
        QStringList toCheck = QStringList() << QLatin1String("me@example.com")
            << QLatin1String("alice");
        toCheck.sort();
        QCOMPARE(ids, toCheck);

        QVERIFY(connect(mChan->requestClose(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isValid(), false);

        mChan.reset();
    }
}

void TestChanBasics::cleanup()
{
    cleanupImpl();
}

void TestChanBasics::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Tp::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn.data(),
                            SIGNAL(invalidated(Tp::DBusProxy *,
                                               const QString &, const QString &)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanBasics)
#include "_gen/chan-basics.cpp.moc.hpp"
