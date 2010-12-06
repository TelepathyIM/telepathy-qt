#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingReady>

#include <telepathy-glib/telepathy-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/test.h>

using namespace Tp;

class SmartDir : public QDir
{
public:
    SmartDir(const QString &path) : QDir(path) { }
    bool rmdir() { return QDir().rmdir(path()); }
    bool removeDirectory();
};

bool SmartDir::removeDirectory()
{
    bool ret = true;

    QFileInfoList list = entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    Q_FOREACH (QFileInfo info, list) {
        if (info.isDir()) {
            SmartDir subDir(info.filePath());
            if (!subDir.removeDirectory()) {
                ret = false;
            }
        } else {
            qDebug() << "deleting" << info.filePath();
            if (!QFile(info.filePath()).remove()) {
                ret = false;
            }
        }
    }

    if (ret) {
        qDebug() << "deleting" << path();
        ret = rmdir();
    }

    return ret;
}

class TestContactsAvatar : public Test
{
    Q_OBJECT

public:
    TestContactsAvatar(QObject *parent = 0)
        : Test(parent), mConnService(0)
    { }

protected Q_SLOTS:
    void expectConnInvalidated();
    void expectPendingContactsFinished(Tp::PendingOperation *);
    void onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &);
    void onAvatarDataChanged(const Tp::AvatarData &);
    void createContactWithFakeAvatar(const char *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testAvatar();

    void cleanup();
    void cleanupTestCase();

private:
    QString mConnName, mConnPath;
    TpTestsContactsConnection *mConnService;
    ConnectionPtr mConn;
    QList<ContactPtr> mContacts;
    bool mAvatarRetrievedCalled;
};

void TestContactsAvatar::onAvatarRetrieved(uint handle, const QString &token,
    const QByteArray &data, const QString &mimeType)
{
    Q_UNUSED(handle);
    Q_UNUSED(token);
    Q_UNUSED(data);
    Q_UNUSED(mimeType);

    mAvatarRetrievedCalled = true;
}

void TestContactsAvatar::onAvatarDataChanged(const AvatarData &avatar)
{
    Q_UNUSED(avatar);
    mLoop->exit(0);
}

void TestContactsAvatar::createContactWithFakeAvatar(const char *id)
{
    TpHandleRepoIface *serviceRepo = tp_base_connection_get_handles(
        (TpBaseConnection *) mConnService, TP_HANDLE_TYPE_CONTACT);
    const gchar avatarData[] = "fake-avatar-data";
    const gchar avatarToken[] = "fake-avatar-token";
    const gchar avatarMimeType[] = "fake-avatar-mime-type";
    TpHandle handle;
    GArray *array;

    handle = tp_handle_ensure(serviceRepo, id, NULL, NULL);
    array = g_array_new(FALSE, FALSE, sizeof(gchar));
    g_array_append_vals (array, avatarData, strlen(avatarData));

    tp_tests_contacts_connection_change_avatar_data(mConnService, handle,
        array, avatarMimeType, avatarToken);
    g_array_unref(array);

    Tp::UIntList handles = Tp::UIntList() << handle;
    Features features = Features()
        << Contact::FeatureAvatarToken
        << Contact::FeatureAvatarData;

    PendingContacts *pending = mConn->contactManager()->contactsForHandles(
        handles, features);
    QVERIFY(connect(pending,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectPendingContactsFinished(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mContacts.size(), 1);

    if (mContacts[0]->avatarData().fileName.isEmpty()) {
        QVERIFY(connect(mContacts[0].data(),
                        SIGNAL(avatarDataChanged(const Tp::AvatarData &)),
                        SLOT(onAvatarDataChanged(const Tp::AvatarData &))));
        QCOMPARE(mLoop->exec(), 0);
    }

    AvatarData avatar = mContacts[0]->avatarData();

    qDebug() << "Contact created:";
    qDebug() << "Avatar token:" << mContacts[0]->avatarToken();
    qDebug() << "Avatar file:" << avatar.fileName;
    qDebug() << "Avatar MimeType:" << avatar.mimeType;

    QFile file(avatar.fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray data(file.readAll());
    file.close();

    QCOMPARE(mContacts[0]->avatarToken(), QString(QLatin1String(avatarToken)));
    QCOMPARE(data, QByteArray(avatarData));
    QCOMPARE(avatar.mimeType, QString(QLatin1String(avatarMimeType)));
}

#define RAND_STR_LEN 6

void TestContactsAvatar::testAvatar()
{
    /* Make sure our tests does not mess up user's avatar cache */
    qsrand(time(0));
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int DirNameLength = 6;
    QString dirName;
    for (int i = 0; i < DirNameLength; ++i) {
        dirName += QLatin1Char(letters[qrand() % qstrlen(letters)]);
    }
    QString tmpDir = QString(QLatin1String("%1/%2")).arg(QDir::tempPath()).arg(dirName);
    QByteArray a = tmpDir.toLatin1();
    setenv ("XDG_CACHE_HOME", a.constData(), true);

    Client::ConnectionInterfaceAvatarsInterface *connAvatarsInterface =
        mConn->optionalInterface<Client::ConnectionInterfaceAvatarsInterface>();

    /* Check if AvatarRetrieved gets called */
    connect(connAvatarsInterface,
            SIGNAL(AvatarRetrieved(uint, const QString &, const QByteArray &, const QString &)),
            SLOT(onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &)));

    /* First time we create a contact, avatar should not be in cache, so
     * AvatarRetrieved should be called */
    mAvatarRetrievedCalled = false;
    createContactWithFakeAvatar("foo");
    QVERIFY(mAvatarRetrievedCalled);

    /* Second time we create a contact, avatar should be in cache now, so
     * AvatarRetrieved should NOT be called */
    mAvatarRetrievedCalled = false;
    createContactWithFakeAvatar("bar");
    QVERIFY(!mAvatarRetrievedCalled);

    QVERIFY(SmartDir(tmpDir).removeDirectory());
}

void TestContactsAvatar::expectConnInvalidated()
{
    mLoop->exit(0);
}

void TestContactsAvatar::expectPendingContactsFinished(PendingOperation *op)
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

void TestContactsAvatar::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-avatar");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    mConnService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL));
    QVERIFY(mConnService != 0);
    QVERIFY(tp_base_connection_register(TP_BASE_CONNECTION(mConnService),
                "foo", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    mConnName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    mConn = Connection::create(mConnName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(mConn->isReady(), false);

    QVERIFY(connect(mConn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mConn->isReady(), true);

    QCOMPARE(mConn->status(), ConnectionStatusConnected);

    QVERIFY(mConn->contactManager()->supportedFeatures().contains(Contact::FeatureAvatarData));
}

void TestContactsAvatar::init()
{
    initImpl();
}

void TestContactsAvatar::cleanup()
{
    cleanupImpl();
}

void TestContactsAvatar::cleanupTestCase()
{
    if (mConn) {
        // Disconnect and wait for invalidated
        QVERIFY(connect(mConn->lowlevel()->requestDisconnect(),
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

QTEST_MAIN(TestContactsAvatar)
#include "_gen/contacts-avatar.cpp.moc.hpp"
