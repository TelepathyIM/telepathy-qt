#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/AccountSet>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingVoid>
#include <TelepathyQt4/Profile>

#include <telepathy-glib/debug.h>

using namespace Tp;

class TestAccountBasics : public Test
{
    Q_OBJECT

public:
    TestAccountBasics(QObject *parent = 0)
        : Test(parent),
          mConn(0),
          mServiceNameChanged(false),
          mIconNameChanged(false),
          mCapabilitiesChanged(false),
          mAccountsCount(0),
          mGotAvatarChanged(false)
    { }

protected Q_SLOTS:
    void onNewAccount(const Tp::AccountPtr &);
    void onAccountServiceNameChanged(const QString &);
    void onAccountIconNameChanged(const QString &);
    void onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &);
    void onAvatarChanged(const Tp::Avatar &);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();

private:
    QStringList pathsForAccounts(const QList<AccountPtr> &list);
    QStringList pathsForAccounts(const AccountSetPtr &set);

    Tp::AccountManagerPtr mAM;
    TestConnHelper *mConn;
    bool mServiceNameChanged;
    QString mServiceName;
    bool mIconNameChanged;
    QString mIconName;
    bool mCapabilitiesChanged;
    int mAccountsCount;
    bool mGotAvatarChanged;
};

#define TEST_VERIFY_PROPERTY_CHANGE(acc, Type, PropertyName, propertyName, expectedValue) \
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, Type, PropertyName, propertyName, \
            propertyName ## Changed, acc->set ## PropertyName(expectedValue), expectedValue)

#define TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, Type, PropertyName, propertyName, signalName, po, expectedValue) \
    { \
        m ## PropertyName = Type(); \
        m ## PropertyName ## Changed = false; \
        qDebug().nospace() << "connecting to " << #propertyName << "Changed()"; \
        QVERIFY(connect(acc.data(), \
                    SIGNAL(signalName(Type)), \
                    SLOT(onAccount ## PropertyName ## Changed(Type)))); \
        qDebug().nospace() << "setting " << #propertyName; \
        QVERIFY(connect(po, \
                    SIGNAL(finished(Tp::PendingOperation*)), \
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*)))); \
        QCOMPARE(mLoop->exec(), 0); \
        \
        if (!m ## PropertyName ## Changed) { \
            qDebug().nospace() << "waiting for the " << #propertyName << "Changed signal"; \
            QCOMPARE(mLoop->exec(), 0); \
        } else { \
            qDebug().nospace() << "not waiting for " << #propertyName << "Changed because we already got it"; \
        } \
        \
        QCOMPARE(acc->propertyName(), expectedValue); \
        QCOMPARE(acc->propertyName(), m ## PropertyName); \
        \
        processDBusQueue(acc.data()); \
    }

#define TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(Type, PropertyName) \
void TestAccountBasics::onAccount ## PropertyName ## Changed(Type value) \
{ \
    m ## PropertyName ## Changed = true; \
    m ## PropertyName = value; \
    mLoop->exit(0); \
}

void TestAccountBasics::onNewAccount(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    mLoop->exit(0);
}

void TestAccountBasics::onAccountServiceNameChanged(const QString &serviceName)
{
    mServiceNameChanged = true;
    mServiceName = serviceName;
    mLoop->exit(0);
}

void TestAccountBasics::onAccountIconNameChanged(const QString &iconName)
{
    mIconNameChanged = true;
    mIconName = iconName;
    mLoop->exit(0);
}

void TestAccountBasics::onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &caps)
{
    mCapabilitiesChanged = true;
    mLoop->exit(0);
}

void TestAccountBasics::onAvatarChanged(const Tp::Avatar &avatar)
{
    qDebug() << "on avatar changed";
    mGotAvatarChanged = true;
    QCOMPARE(avatar.avatarData, QByteArray("asdfg"));
    QCOMPARE(avatar.MIMEType, QString(QLatin1String("image/jpeg")));
    mLoop->exit(0);
}

QStringList TestAccountBasics::pathsForAccounts(const QList<AccountPtr> &list)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, list) {
        ret << account->objectPath();
    }
    return ret;
}

QStringList TestAccountBasics::pathsForAccounts(const AccountSetPtr &set)
{
    QStringList ret;
    Q_FOREACH (const AccountPtr &account, set->accounts()) {
        ret << account->objectPath();
    }
    return ret;
}

void TestAccountBasics::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("account-basics");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mAM = AccountManager::create(AccountFactory::create(QDBusConnection::sessionBus(),
                Account::FeatureCore | Account::FeatureCapabilities));
    QCOMPARE(mAM->isReady(), false);

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "echo2",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestAccountBasics::init()
{
    mGotAvatarChanged = false;

    initImpl();
}

void TestAccountBasics::testBasics()
{
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVERIFY(connect(mAM.data(),
                    SIGNAL(newAccount(const Tp::AccountPtr &)),
                    SLOT(onNewAccount(const Tp::AccountPtr &))));

    QVariantMap parameters;
    parameters[QLatin1String("account")] = QLatin1String("foobar");
    PendingAccount *pacc = mAM->createAccount(QLatin1String("foo"),
            QLatin1String("bar"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pacc->account());

    while (mAccountsCount != 1) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(mAM->interfaces(), QStringList());

    QStringList paths;
    QString accPath(QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QCOMPARE(pathsForAccounts(mAM->allAccounts()), QStringList() << accPath);

    AccountPtr acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady()) {
        mLoop->processEvents();
    }

    QCOMPARE(acc->displayName(), QString(QLatin1String("foobar (account 0)")));

    qDebug() << "making Account::FeatureAvatar ready";

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureAvatar)) {
        mLoop->processEvents();
    }

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    processDBusQueue(acc.data());

    qDebug() << "connecting to avatarChanged()";

    QVERIFY(connect(acc.data(),
                    SIGNAL(avatarChanged(const Tp::Avatar &)),
                    SLOT(onAvatarChanged(const Tp::Avatar &))));

    qDebug() << "setting the avatar";

    Tp::Avatar avatar = { QByteArray("asdfg"), QLatin1String("image/jpeg") };
    QVERIFY(connect(acc->setAvatar(avatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    qDebug() << "making Account::FeatureAvatar ready again (redundantly)";

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureAvatar)) {
        mLoop->processEvents();
    }

    // We might have got it already in the earlier mainloop runs
    if (!mGotAvatarChanged) {
        qDebug() << "waiting for the avatarChanged signal";
        QCOMPARE(mLoop->exec(), 0);
    } else {
        qDebug() << "not waiting for avatarChanged because we already got it";
    }

    qDebug() << "creating another account";

    pacc = mAM->createAccount(QLatin1String("spurious"),
            QLatin1String("normal"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    while (mAccountsCount != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }

    acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady()) {
        mLoop->processEvents();
    }

    acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());

    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady()) {
        mLoop->processEvents();
    }

    // At this point, there's a set icon
    QCOMPARE(acc->iconName(), QLatin1String("bob.png")); // ?!??

    // Unset that
    mIconNameChanged = false;
    mIconName = QString();
    QVERIFY(connect(acc.data(),
                    SIGNAL(iconNameChanged(const QString &)),
                    SLOT(onAccountIconNameChanged(const QString &))));
    QVERIFY(connect(acc->setIconName(QString()),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!mIconNameChanged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // Now that it's unset, an icon name is formed from the protocol name
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));
    QCOMPARE(acc->iconName(), mIconName);

    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureProtocolInfo)) {
        mLoop->processEvents();
    }

    // This time it's fetched from the protocol object (although it probably internally just
    // infers it from the protocol name too)
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureProfile),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureProfile)) {
        mLoop->processEvents();
    }

    ProfilePtr profile = acc->profile();
    QCOMPARE(profile.isNull(), false);
    QCOMPARE(profile->isValid(), true);
    QCOMPARE(profile->serviceName(), QString(QLatin1String("%1-%2"))
                .arg(acc->cmName()).arg(acc->serviceName()));

    QVERIFY(acc->serviceName() == acc->protocolName());

    connect(acc.data(),
            SIGNAL(serviceNameChanged(const QString &)),
            SLOT(onAccountServiceNameChanged(const QString &)));
    connect(acc->setServiceName(acc->protocolName()),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation *)));
    // wait for setServiceName finish
    QCOMPARE(mLoop->exec(), 0);
    // wait for serviceNameChanged
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->serviceName(), acc->protocolName());
    QCOMPARE(mServiceName, acc->serviceName());

    ProtocolInfo protocolInfo = acc->protocolInfo();
    QCOMPARE(protocolInfo.isValid(), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("account")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("password")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("register")), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureAvatar)) {
        mLoop->processEvents();
    }

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar | Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureAvatar | Account::FeatureProtocolInfo)) {
        mLoop->processEvents();
    }

    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));
    protocolInfo = acc->protocolInfo();
    QCOMPARE(protocolInfo.isValid(), true);

    profile = acc->profile();
    QCOMPARE(profile.isNull(), false);
    QCOMPARE(profile->isValid(), true);
    QCOMPARE(profile->serviceName(), QString(QLatin1String("%1-%2"))
                .arg(acc->cmName()).arg(acc->serviceName()));
    QCOMPARE(profile->type(), QLatin1String("IM"));
    QCOMPARE(profile->provider(), QString());
    QCOMPARE(profile->name(), acc->protocolName());
    QCOMPARE(profile->cmName(), acc->cmName());
    QCOMPARE(profile->protocolName(), acc->protocolName());
    QCOMPARE(profile->parameters().isEmpty(), false);
    QCOMPARE(profile->allowOtherPresences(), true);
    QCOMPARE(profile->presences().isEmpty(), true);
    QCOMPARE(profile->unsupportedChannelClassSpecs().isEmpty(), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureCapabilities),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!acc->isReady(Account::FeatureCapabilities)) {
        mLoop->processEvents();
    }

    /* using protocol info */
    ConnectionCapabilities caps = acc->capabilities();
    QCOMPARE(caps.textChats(), true);

    mServiceNameChanged = false;
    mServiceName = QString();
    mIconNameChanged = false;
    mIconName = QString();
    mCapabilitiesChanged = false;
    QVERIFY(connect(acc.data(),
                    SIGNAL(serviceNameChanged(const QString &)),
                    SLOT(onAccountServiceNameChanged(const QString &))));
    QVERIFY(connect(acc.data(),
                    SIGNAL(capabilitiesChanged(const Tp::ConnectionCapabilities &)),
                    SLOT(onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &))));
    QVERIFY(connect(acc->setServiceName(QLatin1String("test-profile")),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    while (!mServiceNameChanged && !mIconNameChanged && !mCapabilitiesChanged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QCOMPARE(acc->serviceName(), QLatin1String("test-profile"));
    QCOMPARE(mServiceName, acc->serviceName());

    QCOMPARE(acc->iconName(), QLatin1String("test-profile-icon"));
    QCOMPARE(mIconName, acc->iconName());

    /* using merged protocol info caps and profile caps */
    caps = acc->capabilities();
    QCOMPARE(caps.textChats(), false);

    Client::DBus::PropertiesInterface *accPropertiesInterface =
        acc->interface<Client::DBus::PropertiesInterface>();

    // simulate that the account has a connection
    QVERIFY(connect(new PendingVoid(
                        accPropertiesInterface->Set(
                            QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                            QLatin1String("Connection"),
                            QDBusVariant(mConn->objectPath())),
                        acc),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    // wait for the connection to be built in Account
    while (acc->connection().isNull()) {
        QCOMPARE(mLoop->exec(), 0);
    }

    QVERIFY(connect(acc->connection()->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!acc->connection()->isReady()) {
        mLoop->processEvents();
    }

    // once the status change the capabilities will be updated
    mCapabilitiesChanged = false;
    QVERIFY(connect(acc->setRequestedPresence(Presence::available()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!mCapabilitiesChanged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // using connection caps now
    caps = acc->capabilities();
    QCOMPARE(caps.textChats(), true);
    QCOMPARE(caps.textChatrooms(), false);
    QCOMPARE(caps.streamedMediaCalls(), false);
    QCOMPARE(caps.streamedMediaAudioCalls(), false);
    QCOMPARE(caps.streamedMediaVideoCalls(), false);
    QCOMPARE(caps.streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(caps.upgradingStreamedMediaCalls(), false);

    // once the status change the capabilities will be updated
    mCapabilitiesChanged = false;
    QVERIFY(connect(new PendingVoid(
                        accPropertiesInterface->Set(
                            QLatin1String(TELEPATHY_INTERFACE_ACCOUNT),
                            QLatin1String("Connection"),
                            QDBusVariant(QLatin1String("/"))),
                        acc),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QVERIFY(connect(acc->setRequestedPresence(Presence::offline()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!mCapabilitiesChanged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    /* back to using merged protocol info caps and profile caps */
    caps = acc->capabilities();
    QCOMPARE(caps.textChats(), false);
}

void TestAccountBasics::cleanup()
{
    cleanupImpl();
}

void TestAccountBasics::cleanupTestCase()
{
    if (mConn) {
        QCOMPARE(mConn->disconnect(), true);
        delete mConn;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountBasics)
#include "_gen/account-basics.cpp.moc.hpp"
