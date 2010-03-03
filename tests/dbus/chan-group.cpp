#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingHandles>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReferencedHandles>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/glib/csh/conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class TestChanGroup : public Test
{
    Q_OBJECT

public:
    TestChanGroup(QObject *parent = 0)
        : Test(parent), mConnService(0),
          mRoomNumber(0), mRoomCount(4), mRequested(false)
    { }

protected Q_SLOTS:
    void expectConnReady(Tp::Connection::Status, Tp::ConnectionStatusReason);
    void expectConnInvalidated();
    void expectChanInvalidated(Tp::DBusProxy*,const QString &, const QString &);
    void expectPendingRoomHandlesFinished(Tp::PendingOperation*);
    void expectPendingContactHandlesFinished(Tp::PendingOperation*);
    void expectCreateChannelFinished(Tp::PendingOperation *);
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void onChannelGroupFlagsChanged(uint, uint, uint);
    void onGroupMembersChanged(
            const Tp::Contacts &groupMembersAdded,
            const Tp::Contacts &groupLocalPendingMembersAdded,
            const Tp::Contacts &groupRemotePendingMembersAdded,
            const Tp::Contacts &groupMembersRemoved,
            const Tp::Channel::GroupMemberChangeDetails &details);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testRequestHandle();
    void testCreateChannel();
    void testCreateChannelDetailed();
    void testCreateChannelFallback();
    void testCreateChannelFallbackDetailed();

    void cleanup();
    void cleanupTestCase();

private:
    void checkExpectedIds(const Contacts &contacts,
        const QStringList &expectedIds);
    void debugContacts();
    void doTestCreateChannel();

    QString mConnName, mConnPath;
    ExampleCSHConnection *mConnService;
    ConnectionPtr mConn;
    ChannelPtr mChan;
    QString mChanObjectPath;
    uint mRoomNumber;
    uint mRoomCount;
    ReferencedHandles mRoomHandles;
    ReferencedHandles mContactHandles;
    QList<ContactPtr> mContacts;
    Contacts mChangedCurrent;
    Contacts mChangedLP;
    Contacts mChangedRP;
    Contacts mChangedRemoved;
    Channel::GroupMemberChangeDetails mDetails;
    bool mRequested;
    QString mChanInvalidatedErrorName;
    QString mChanInvalidatedErrorMessage;
};

void TestChanGroup::expectConnReady(Tp::Connection::Status newStatus,
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

void TestChanGroup::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestChanGroup::expectChanInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);
    mChanInvalidatedErrorName = errorName;
    mChanInvalidatedErrorMessage = errorMessage;
    mLoop->exit(0);
}

void TestChanGroup::expectPendingRoomHandlesFinished(PendingOperation *op)
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
    mRoomHandles = pending->handles();
    mLoop->exit(0);
}

void TestChanGroup::expectPendingContactHandlesFinished(PendingOperation *op)
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
    mContactHandles = pending->handles();
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

void TestChanGroup::expectPendingContactsFinished(PendingOperation *op)
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
    PendingContacts *pending = qobject_cast<PendingContacts *>(op);
    mContacts = pending->contacts();

    mLoop->exit(0);
}

void TestChanGroup::onChannelGroupFlagsChanged(uint flags, uint added, uint removed)
{
    qDebug() << "group flags changed";
    mLoop->exit(0);
}

void TestChanGroup::onGroupMembersChanged(
        const Contacts &groupMembersAdded,
        const Contacts &groupLocalPendingMembersAdded,
        const Contacts &groupRemotePendingMembersAdded,
        const Contacts &groupMembersRemoved,
        const Channel::GroupMemberChangeDetails &details)
{
    qDebug() << "group members changed";
    mChangedCurrent = groupMembersAdded;
    mChangedLP = groupLocalPendingMembersAdded;
    mChangedRP = groupRemotePendingMembersAdded;
    mChangedRemoved = groupMembersRemoved;
    mDetails = details;
    debugContacts();
    mLoop->exit(0);
}

void TestChanGroup::checkExpectedIds(const Contacts &contacts,
        const QStringList &expectedIds)
{
    QStringList ids;
    foreach (const ContactPtr &contact, contacts) {
        ids << contact->id();
    }

    ids.sort();
    QCOMPARE(ids, expectedIds);
}

void TestChanGroup::debugContacts()
{
    qDebug() << "contacts on group:";
    foreach (const ContactPtr &contact, mChan->groupContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "local pending contacts on group:";
    foreach (const ContactPtr &contact, mChan->groupLocalPendingContacts()) {
        qDebug() << " " << contact->id();
    }

    qDebug() << "remote pending contacts on group:";
    foreach (const ContactPtr &contact, mChan->groupRemotePendingContacts()) {
        qDebug() << " " << contact->id();
    }
}

void TestChanGroup::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("chan-group");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

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

    mConn = Connection::create(mConnName, mConnPath);
    QCOMPARE(mConn->isReady(), false);

    mConn->requestConnect();

    QVERIFY(connect(mConn->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

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

void TestChanGroup::init()
{
    initImpl();
}

void TestChanGroup::testRequestHandle()
{
    // Test identifiers
    QStringList ids;
    for (uint i = 0; i < mRoomCount; ++i) {
        ids << QString("#room%1").arg(i);
    }

    // Request handles for the identifiers and wait for the request to process
    PendingHandles *pending = mConn->requestHandles(Tp::HandleTypeRoom, ids);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingRoomHandlesFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(pending,
                       SIGNAL(finished(Tp::PendingOperation*)),
                       this,
                       SLOT(expectPendingRoomHandlesFinished(Tp::PendingOperation*))));
}

void TestChanGroup::testCreateChannel()
{
    mRequested = true;
    example_csh_connection_set_enable_change_members_detailed(mConnService, false);
    example_csh_connection_set_use_properties_room (mConnService, true);
    doTestCreateChannel();
    mRoomNumber++;
}

void TestChanGroup::testCreateChannelDetailed()
{
    mRequested = true;
    example_csh_connection_set_enable_change_members_detailed(mConnService, true);
    example_csh_connection_set_use_properties_room (mConnService, true);
    doTestCreateChannel();
    mRoomNumber++;
}

void TestChanGroup::testCreateChannelFallback()
{
    mRequested = false;
    example_csh_connection_set_enable_change_members_detailed(mConnService, false);
    example_csh_connection_set_use_properties_room (mConnService, false);
    doTestCreateChannel();
    mRoomNumber++;
}

void TestChanGroup::testCreateChannelFallbackDetailed()
{
    mRequested = false;
    example_csh_connection_set_enable_change_members_detailed(mConnService, true);
    example_csh_connection_set_use_properties_room (mConnService, false);
    doTestCreateChannel();
    mRoomNumber++;
}


void TestChanGroup::doTestCreateChannel()
{
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeRoom);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mRoomHandles[mRoomNumber]);

    QVERIFY(connect(mConn->createChannel(request),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectCreateChannelFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mChan);

    QVERIFY(connect(mChan->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->isReady(), true);

    QCOMPARE(mChan->isRequested(), mRequested);
    QCOMPARE(mChan->initiatorContact().isNull(), true);
    QCOMPARE(mChan->groupSelfContact()->id(), QString("me@#room%1").arg(mRoomNumber));

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupFlagsChanged(uint, uint, uint)),
                    SLOT(onChannelGroupFlagsChanged(uint, uint, uint))));

    if (!mChan->groupCanAddContacts() || !mChan->groupCanRemoveContacts()) {
        // wait the group flags change
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mChan->groupCanAddContacts(), true);
    QCOMPARE(mChan->groupCanRemoveContacts(), true);

    debugContacts();

    QVERIFY(connect(mChan.data(),
                    SIGNAL(groupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &)),
                    SLOT(onGroupMembersChanged(
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Contacts &,
                            const Tp::Channel::GroupMemberChangeDetails &))));

    if (mChan->groupContacts().count() != 5) {
        // wait the initial contacts to be added to the group
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mChan->groupContacts().count(), 5);

    QString roomName = QString("#room%1").arg(mRoomNumber);

    QStringList expectedIds;
    expectedIds << QString("me@") + roomName <<
        QString("alice@") + roomName <<
        QString("bob@") + roomName <<
        QString("chris@") + roomName <<
        QString("anonymous coward@") + roomName;
    expectedIds.sort();
    checkExpectedIds(mChan->groupContacts(), expectedIds);

    QStringList ids = QStringList() <<
        QString("john@#room%1").arg(mRoomNumber) <<
        QString("mary@#room%1").arg(mRoomNumber) <<
        QString("another anonymous coward@#room%1").arg(mRoomNumber);
    QVERIFY(connect(mConn->requestHandles(Tp::HandleTypeContact, ids),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactHandlesFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    // Wait for the contacts to be built
    QVERIFY(connect(mConn->contactManager()->contactsForHandles(mContactHandles),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mContacts.size(), 3);
    ids.sort();
    checkExpectedIds(Contacts::fromList(mContacts), ids);

    QString message("I want to add them");
    mChan->groupAddContacts(mContacts, message);

    // expect contacts to be added to remote pending, the csh test emits one at
    // a time
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mDetails.actor(), mChan->groupSelfContact());
    QCOMPARE(mDetails.message(), message);
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mDetails.actor(), mChan->groupSelfContact());
    QCOMPARE(mDetails.message(), message);
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mDetails.actor(), mChan->groupSelfContact());
    QCOMPARE(mDetails.message(), message);

    expectedIds.clear();
    expectedIds << QString("john@") + roomName <<
            QString("mary@") + roomName <<
            QString("another anonymous coward@") + roomName;
    expectedIds.sort();
    checkExpectedIds(mChan->groupRemotePendingContacts(), expectedIds);

    QList<ContactPtr> toRemove;
    toRemove.append(mContacts[1]);
    toRemove.append(mContacts[2]);
    mChan->groupRemoveContacts(toRemove, "I want to remove some of them");

    // expect mary and another anonymous coward to be removed
    // CSH emits these as two signals though, so waiting for one membersChanged isn't enough
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLoop->exec(), 0);

    expectedIds.clear();
    expectedIds << QString("me@") + roomName <<
        QString("alice@") + roomName <<
        QString("bob@") + roomName <<
        QString("chris@") + roomName <<
        QString("anonymous coward@") + roomName;
    expectedIds.sort();
    checkExpectedIds(mChan->groupContacts(), expectedIds);
    expectedIds.clear();
    expectedIds << QString("john@") + roomName;
    checkExpectedIds(mChan->groupRemotePendingContacts(), expectedIds);

    example_csh_connection_accept_invitations(mConnService);

    // expect john to accept invite
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mDetails.message(), QString("Invitation accepted"));

    expectedIds.clear();
    expectedIds << QString("me@") + roomName <<
            QString("alice@") + roomName <<
            QString("bob@") + roomName <<
            QString("chris@") + roomName <<
            QString("anonymous coward@") + roomName <<
            QString("john@") + roomName;
    expectedIds.sort();
    checkExpectedIds(mChan->groupContacts(), expectedIds);

    toRemove.clear();
    ids.clear();
    foreach (const ContactPtr &contact, mChan->groupContacts()) {
        ids << contact->id();
        if (contact != mChan->groupSelfContact() && toRemove.isEmpty()) {
            toRemove.append(contact);
            expectedIds.removeOne(contact->id());
        }
    }

    mChan->groupRemoveContacts(toRemove, "Checking removal of a contact in current list");
    QCOMPARE(mLoop->exec(), 0);
    expectedIds.sort();
    checkExpectedIds(mChan->groupContacts(), expectedIds);

    QVERIFY(connect(mChan.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(expectChanInvalidated(Tp::DBusProxy *, const QString &, const QString &))));

    mChan->groupRemoveContacts(QList<ContactPtr>() << mChan->groupSelfContact(), "I want to remove myself");
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChan->groupSelfContactRemoveInfo().hasActor(), true);
    QCOMPARE(mChan->groupSelfContactRemoveInfo().actor(), mChan->groupSelfContact());
    QCOMPARE(mChan->groupSelfContactRemoveInfo().hasMessage(), true);
    QCOMPARE(mChan->groupSelfContactRemoveInfo().message(), QString("I want to remove myself"));
    QCOMPARE(mChan->groupSelfContactRemoveInfo().hasError(), false);

    // wait until chan gets invalidated
    while (mChan->isValid()) {
        QCOMPARE(mLoop->exec(), 0);
    }
    QCOMPARE(mChanInvalidatedErrorName, QString(TELEPATHY_ERROR_CANCELLED));
    QCOMPARE(mChanInvalidatedErrorMessage, QString("I want to remove myself"));

    mChan.reset();
}

void TestChanGroup::cleanup()
{
    cleanupImpl();
}

void TestChanGroup::cleanupTestCase()
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

QTEST_MAIN(TestChanGroup)
#include "_gen/chan-group.cpp.moc.hpp"
