#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingHandles>
#include <TelepathyQt4/Client/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/csh/conn.h>
#include <tests/lib/test.h>

using namespace Telepathy::Client;

class TestChanGroup : public Test
{
    Q_OBJECT

public:
    TestChanGroup(QObject *parent = 0)
        : Test(parent), mConnService(0), mConn(0), mChan(0)
    { }

protected Q_SLOTS:
    void expectConnReady(uint, uint);
    void expectConnInvalidated();
    void expectPendingHandlesFinished(Telepathy::Client::PendingOperation*);
    void expectCreateChannelFinished(Telepathy::Client::PendingOperation *);
    void onGroupMembersChanged(
            const QList<QSharedPointer<Contact> > &groupMembersAdded,
            const QList<QSharedPointer<Contact> > &groupLocalPendingMembersAdded,
            const QList<QSharedPointer<Contact> > &groupRemotePendingMembersAdded,
            const QList<QSharedPointer<Contact> > &groupMembersRemoved,
            uint actor, uint reason, const QString &message);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestHandle();
    void testCreateChannel();

    void cleanup();
    void cleanupTestCase();

private:
    void debugContacts();

    QString mConnName, mConnPath;
    ExampleCSHConnection *mConnService;
    Connection *mConn;
    Channel *mChan;
    QString mChanObjectPath;
    ReferencedHandles mHandles;
};

void TestChanGroup::expectConnReady(uint newStatus, uint newStatusReason)
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

void TestChanGroup::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestChanGroup::expectPendingHandlesFinished(PendingOperation *op)
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

void TestChanGroup::expectCreateChannelFinished(PendingOperation* op)
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

void TestChanGroup::onGroupMembersChanged(
        const QList<QSharedPointer<Contact> > &groupMembersAdded,
        const QList<QSharedPointer<Contact> > &groupLocalPendingMembersAdded,
        const QList<QSharedPointer<Contact> > &groupRemotePendingMembersAdded,
        const QList<QSharedPointer<Contact> > &groupMembersRemoved,
        uint actor, uint reason, const QString &message)
{
    debugContacts();

    QVERIFY(mChan->groupContacts().size() > 1);
    QVERIFY(mChan->groupLocalPendingContacts().isEmpty());
    QVERIFY(mChan->groupRemotePendingContacts().isEmpty());

    if (mChan->groupContacts().count() == 5) {
        QStringList ids;
        foreach (const QSharedPointer<Contact> &contact, mChan->groupContacts()) {
            ids << contact->id();
        }

        QCOMPARE(ids, QStringList() <<
                        "me@#room" <<
                        "alice@#room" <<
                        "bob@#room" <<
                        "chris@#room" <<
                        "anonymous coward@#room");

        mLoop->exit(0);
    }
}

void TestChanGroup::debugContacts()
{
    qDebug() << "contacts on group:";
    foreach (const QSharedPointer<Contact> &contact, mChan->groupContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "local pending contacts on group:";
    foreach (const QSharedPointer<Contact> &contact, mChan->groupLocalPendingContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "remote pending contacts on group:";
    foreach (const QSharedPointer<Contact> &contact, mChan->groupRemotePendingContacts()) {
        qDebug() << " " << contact->id();
    }
}

void TestChanGroup::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("chan-group");
    tp_debug_set_flags("all");

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = EXAMPLE_CSH_CONNECTION(g_object_new(
            EXAMPLE_TYPE_CSH_CONNECTION,
            "account", "me@example.com",
            "protocol", "contacts",
            0));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "csh", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = name;
    mConnPath = connPath;

    g_free(name);
    g_free(connPath);

    mConn = new Connection(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    if (mConn->status() != Connection::StatusConnected) {
        QVERIFY(connect(mConn,
                        SIGNAL(statusChanged(uint, uint)),
                        SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mLoop->exec(), 0);
        QVERIFY(disconnect(mConn,
                           SIGNAL(statusChanged(uint, uint)),
                           this,
                           SLOT(expectConnReady(uint, uint))));
        QCOMPARE(mConn->status(), (uint) Connection::StatusConnected);
    }

    QVERIFY(mConn->requestsInterface() != 0);
}

void TestChanGroup::init()
{
    initImpl();
}

void TestChanGroup::testRequestHandle()
{
    // Test identifiers
    QStringList ids = QStringList() << "#room";

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->requestHandles(Telepathy::HandleTypeRoom, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectPendingHandlesFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                       this,
                       SLOT(expectPendingHandlesFinished(Telepathy::Client::PendingOperation*))));
}

void TestChanGroup::testCreateChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Telepathy::HandleTypeRoom);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mHandles[0]);
    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Telepathy::Client::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    if (mChan) {
        QVERIFY(connect(mChan->becomeReady(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);
        QCOMPARE(mChan->isReady(), true);

        debugContacts();

        QVERIFY(connect(mChan,
                        SIGNAL(groupMembersChanged(
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                uint, uint, const QString &)),
                        SLOT(onGroupMembersChanged(
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                const QList<QSharedPointer<Contact> > &,
                                uint, uint, const QString &))));
        QCOMPARE(mLoop->exec(), 0);

        delete mChan;
        mChan = 0;
    }
}

void TestChanGroup::cleanup()
{
    cleanupImpl();
}

void TestChanGroup::cleanupTestCase()
{
    if (mConn != 0) {
        // Disconnect and wait for the readiness change
        QVERIFY(connect(mConn->requestDisconnect(),
                        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                        SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation*))));
        QCOMPARE(mLoop->exec(), 0);

        if (mConn->isValid()) {
            QVERIFY(connect(mConn,
                            SIGNAL(invalidated(Telepathy::Client::DBusProxy *,
                                               const QString &, const QString &)),
                            SLOT(expectConnInvalidated())));
            QCOMPARE(mLoop->exec(), 0);
        }

        delete mConn;
        mConn = 0;
    }

    if (mConnService != 0) {
        g_object_unref(mConnService);
        mConnService = 0;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestChanGroup)
#include "_gen/chan-group.cpp.moc.hpp"
