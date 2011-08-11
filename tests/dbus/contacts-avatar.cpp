#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt4/AvatarData>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>

#include <telepathy-glib/debug.h>

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
        : Test(parent), mConn(0)
    { }

protected Q_SLOTS:
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
    TestConnHelper *mConn;
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
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    const gchar avatarData[] = "fake-avatar-data";
    const gchar avatarToken[] = "fake-avatar-token";
    const gchar avatarMimeType[] = "fake-avatar-mime-type";
    TpHandle handle;
    GArray *array;

    handle = tp_handle_ensure(serviceRepo, id, NULL, NULL);
    array = g_array_new(FALSE, FALSE, sizeof(gchar));
    g_array_append_vals(array, avatarData, strlen(avatarData));

    tp_tests_contacts_connection_change_avatar_data(
            TP_TESTS_CONTACTS_CONNECTION(mConn->service()), handle,
            array, avatarMimeType, avatarToken);
    g_array_unref(array);

    Tp::UIntList handles = Tp::UIntList() << handle;
    Features features = Features()
        << Contact::FeatureAvatarToken
        << Contact::FeatureAvatarData;

    mContacts = mConn->contacts(handles, features);
    QCOMPARE(mContacts.size(), handles.size());

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

void TestContactsAvatar::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("contacts-avatar");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "foo",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestContactsAvatar::init()
{
    initImpl();
}

void TestContactsAvatar::testAvatar()
{
    QVERIFY(mConn->client()->contactManager()->supportedFeatures().contains(
                Contact::FeatureAvatarData));

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
        mConn->client()->optionalInterface<Client::ConnectionInterfaceAvatarsInterface>();

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

void TestContactsAvatar::cleanup()
{
    cleanupImpl();
}

void TestContactsAvatar::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestContactsAvatar)
#include "_gen/contacts-avatar.cpp.moc.hpp"
