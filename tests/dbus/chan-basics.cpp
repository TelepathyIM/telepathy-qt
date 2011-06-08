#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestChanBasics : public Test
{
    Q_OBJECT

public:
    TestChanBasics(QObject *parent = 0)
        : Test(parent), mConn(0), mHandle(0)
    { }

protected Q_SLOTS:
    void expectInvalidated();
    void expectPendingHandleFinished(Tp::PendingOperation *);
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
    TestConnHelper *mConn;
    ChannelPtr mChan;
    QString mChanObjectPath;
    uint mHandle;
};

void TestChanBasics::expectInvalidated()
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
    g_set_prgname("chan-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            NULL);
    QCOMPARE(mConn->connect(), true);

    QCOMPARE(mConn->enableFeatures(Connection::FeatureSelfContact), true);
}

void TestChanBasics::init()
{
    initImpl();

    mChan.reset();
}

void TestChanBasics::testRequestHandle()
{
    // Test identifiers
    QStringList ids = QStringList() << QLatin1String("alice");

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->client()->lowlevel()->requestHandles(Tp::HandleTypeContact, ids);
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
    QVERIFY(connect(mConn->client()->lowlevel()->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    Q_ASSERT(!mChan.isNull());

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
    QCOMPARE(mChan->groupSelfContact(), mConn->client()->selfContact());
    QCOMPARE(mChan->targetId(), QString::fromLatin1("alice"));
    QVERIFY(!mChan->targetContact().isNull());
    QCOMPARE(mChan->targetContact()->id(), QString::fromLatin1("alice"));

    QStringList ids;
    Q_FOREACH (const ContactPtr &contact, mChan->groupContacts()) {
        ids << contact->id();

        QVERIFY(contact == mChan->groupSelfContact() || contact == mChan->targetContact());
    }
    ids.sort();
    QStringList toCheck = QStringList() << QLatin1String("me@example.com")
        << QLatin1String("alice");
    toCheck.sort();
    QCOMPARE(ids, toCheck);
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
    QVERIFY(connect(mConn->client()->lowlevel()->ensureChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectEnsureChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(!mChan.isNull());

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
    QCOMPARE(mChan->groupSelfContact(), mConn->client()->selfContact());

    QStringList ids;
    Q_FOREACH (const ContactPtr &contact, mChan->groupContacts()) {
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
}

void TestChanBasics::cleanup()
{
    cleanupImpl();
}

void TestChanBasics::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    if (mChan) {
        QVERIFY(!mChan->isValid());
        QVERIFY(mChan->invalidationReason() == TP_QT4_ERROR_CANCELLED ||
                mChan->invalidationReason() == TP_QT4_ERROR_ORPHANED);
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanBasics)
#include "_gen/chan-basics.cpp.moc.hpp"
