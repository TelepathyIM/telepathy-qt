#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/contacts-conn.h>

#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>

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
        : Test(parent), mConn(0),
          mGotAvatarRetrieved(false), mAvatarDatasChanged(0)
    { }

protected Q_SLOTS:
    void onAvatarRetrieved(uint, const QString &, const QByteArray &, const QString &);
    void onAvatarDataChanged(const Tp::AvatarData &);
    void createContactWithFakeAvatar(const char *);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testAvatar();
    void testRequestAvatars();

    void cleanup();
    void cleanupTestCase();

private:
    TestConnHelper *mConn;
    QList<ContactPtr> mContacts;
    bool mGotAvatarRetrieved;
    int mAvatarDatasChanged;
};

void TestContactsAvatar::onAvatarRetrieved(uint handle, const QString &token,
    const QByteArray &data, const QString &mimeType)
{
    Q_UNUSED(handle);
    Q_UNUSED(token);
    Q_UNUSED(data);
    Q_UNUSED(mimeType);

    mGotAvatarRetrieved = true;
}

void TestContactsAvatar::onAvatarDataChanged(const AvatarData &avatar)
{
    Q_UNUSED(avatar);
    mAvatarDatasChanged++;
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
            array, avatarMimeType, avatarToken, true);
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

    mGotAvatarRetrieved = false;
    mAvatarDatasChanged = 0;
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
    mGotAvatarRetrieved = false;
    createContactWithFakeAvatar("foo");
    QVERIFY(mGotAvatarRetrieved);

    /* Second time we create a contact, avatar should be in cache now, so
     * AvatarRetrieved should NOT be called */
    mGotAvatarRetrieved = false;
    createContactWithFakeAvatar("bar");
    QVERIFY(!mGotAvatarRetrieved);

    QVERIFY(SmartDir(tmpDir).removeDirectory());
}

void TestContactsAvatar::testRequestAvatars()
{
    TpHandleRepoIface *serviceRepo = tp_base_connection_get_handles(
            TP_BASE_CONNECTION(mConn->service()), TP_HANDLE_TYPE_CONTACT);
    const gchar avatarData[] = "fake-avatar-data";
    const gchar avatarToken[] = "fake-avatar-token";
    const gchar avatarMimeType[] = "fake-avatar-mime-type";
    TpHandle handle;
    GArray *array;

    array = g_array_new(FALSE, FALSE, sizeof(gchar));
    g_array_append_vals(array, avatarData, strlen(avatarData));

    // First let's create the contacts
    Tp::UIntList handles;
    for (int i = 0; i < 100; ++i) {
        QString contactId = QLatin1String("contact") + QString::number(i);
        handle = tp_handle_ensure(serviceRepo, contactId.toLatin1().constData(), NULL, NULL);
        handles << handle;
    }
    Features features = Features() << Contact::FeatureAvatarToken << Contact::FeatureAvatarData;
    QList<ContactPtr> contacts = mConn->contacts(handles, features);
    QCOMPARE(contacts.size(), handles.size());

    // now let's update the avatar for half of them so we can later check that requestContactAvatars
    // actually worked for all contacts.
    mAvatarDatasChanged = 0;
    for (int i = 0; i < contacts.size(); ++i) {
        ContactPtr contact = contacts[i];
        QVERIFY(contact->avatarData().fileName.isEmpty());

        QString contactAvatarToken = QLatin1String(avatarToken) + QString::number(i);

        QVERIFY(connect(contact.data(),
                        SIGNAL(avatarDataChanged(const Tp::AvatarData &)),
                        SLOT(onAvatarDataChanged(const Tp::AvatarData &))));

        tp_tests_contacts_connection_change_avatar_data(
                TP_TESTS_CONTACTS_CONNECTION(mConn->service()), contact->handle()[0],
                array, avatarMimeType, contactAvatarToken.toLatin1().constData(),
                (i % 2));
    }

    processDBusQueue(mConn->client().data());

    while (mAvatarDatasChanged < contacts.size() / 2) {
        mLoop->processEvents();
    }

    // check the only half got the updates
    QCOMPARE(mAvatarDatasChanged, contacts.size() / 2);

    for (int i = 0; i < contacts.size(); ++i) {
        ContactPtr contact = contacts[i];
        if (i % 2) {
            QVERIFY(!contact->avatarData().fileName.isEmpty());
            QCOMPARE(contact->avatarData().mimeType, QLatin1String(avatarMimeType));
            QString contactAvatarToken = QLatin1String(avatarToken) + QString::number(i);
            QCOMPARE(contact->avatarToken(), contactAvatarToken);
        } else {
            QVERIFY(contact->avatarData().fileName.isEmpty());
        }
    }

    // let's call ContactManager::requestContactAvatars now, it should update all contacts
    mAvatarDatasChanged = 0;
    mConn->client()->contactManager()->requestContactAvatars(contacts);
    processDBusQueue(mConn->client().data());

    // the other half will now receive the avatar
    while (mAvatarDatasChanged < contacts.size() / 2) {
        mLoop->processEvents();
    }

    // check the only half got the updates
    QCOMPARE(mAvatarDatasChanged, contacts.size() / 2);

    for (int i = 0; i < contacts.size(); ++i) {
        ContactPtr contact = contacts[i];
        QVERIFY(!contact->avatarData().fileName.isEmpty());
        QCOMPARE(contact->avatarData().mimeType, QLatin1String(avatarMimeType));
        QString contactAvatarToken = QLatin1String(avatarToken) + QString::number(i);
        QCOMPARE(contact->avatarToken(), contactAvatarToken);
    }

    mAvatarDatasChanged = 0;

    // empty D-DBus queue
    processDBusQueue(mConn->client().data());

    // it should silently work, no crash
    mConn->client()->contactManager()->requestContactAvatars(QList<ContactPtr>());

    // let the mainloop run
    processDBusQueue(mConn->client().data());

    QCOMPARE(mAvatarDatasChanged, 0);
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
