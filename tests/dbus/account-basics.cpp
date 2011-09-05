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
#include <TelepathyQt4/PendingStringList>
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
          mAccountsCount(0)
    { }

protected Q_SLOTS:
    void onNewAccount(const Tp::AccountPtr &);
    void onAccountServiceNameChanged(const QString &);
    void onAccountDisplayNameChanged(const QString &);
    void onAccountIconNameChanged(const QString &);
    void onAccountNicknameChanged(const QString &);
    void onAccountAvatarChanged(const Tp::Avatar &);
    void onAccountParametersChanged(const QVariantMap &);
    void onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &);
    void onAccountConnectsAutomaticallyChanged(bool);

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
    int mAccountsCount;

    bool mServiceNameChanged;
    QString mServiceName;
    bool mDisplayNameChanged;
    QString mDisplayName;
    bool mIconNameChanged;
    QString mIconName;
    bool mNicknameChanged;
    QString mNickname;
    bool mAvatarChanged;
    Avatar mAvatar;
    bool mParametersChanged;
    QVariantMap mParameters;
    bool mCapabilitiesChanged;
    ConnectionCapabilities mCapabilities;
    bool mConnectsAutomaticallyChanged;
    bool mConnectsAutomatically;
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

TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, ServiceName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, DisplayName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, IconName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, Nickname)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const Avatar &, Avatar)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QVariantMap &, Parameters)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const ConnectionCapabilities &, Capabilities)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(bool, ConnectsAutomatically)

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
    mServiceNameChanged = false;
    mServiceName = QString();
    mDisplayNameChanged = false;
    mDisplayName = QString();
    mIconNameChanged = false;
    mIconName = QString();
    mNicknameChanged = false;
    mNickname = QString();
    mAvatarChanged = false;
    mAvatar = Avatar();
    mParametersChanged = false;
    mParameters = QVariantMap();
    mCapabilitiesChanged = false;
    mCapabilities = ConnectionCapabilities();
    mConnectsAutomaticallyChanged = false;
    mConnectsAutomatically = false;

    initImpl();
}

void TestAccountBasics::testBasics()
{
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);
    QCOMPARE(mAM->interfaces(), QStringList());
    QCOMPARE(mAM->supportedAccountProperties(), QStringList() <<
            QLatin1String("org.freedesktop.Telepathy.Account.Enabled"));

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
    processDBusQueue(mConn->client().data());

    QStringList paths;
    QString accPath(QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QCOMPARE(pathsForAccounts(mAM->allAccounts()), QStringList() << accPath);
    QList<AccountPtr> accs = mAM->accountsForPaths(
            QStringList() << accPath << QLatin1String("/invalid/path"));
    QCOMPARE(accs.size(), 2);
    QCOMPARE(accs[0]->objectPath(), accPath);
    QVERIFY(!accs[1]);

    QCOMPARE(mAM->allAccounts()[0]->isReady(
                Account::FeatureCore | Account::FeatureCapabilities), true);

    AccountPtr acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(), true);

    QCOMPARE(acc->connectionFactory(), mAM->connectionFactory());
    QCOMPARE(acc->channelFactory(), mAM->channelFactory());
    QCOMPARE(acc->contactFactory(), mAM->contactFactory());
    QCOMPARE(acc->isValidAccount(), true);
    QCOMPARE(acc->isEnabled(), true);
    QCOMPARE(acc->cmName(), QLatin1String("foo"));
    QCOMPARE(acc->protocolName(), QLatin1String("bar"));
    // Service name is empty, fallback to protocol name
    QCOMPARE(acc->serviceName(), QLatin1String("bar"));
    // FeatureProfile not ready yet
    QVERIFY(!acc->profile());
    QCOMPARE(acc->displayName(), QString(QLatin1String("foobar (account 0)")));
    QCOMPARE(acc->iconName(), QLatin1String("bob.png"));
    QCOMPARE(acc->nickname(), QLatin1String("Bob"));
    // FeatureProtocolInfo not ready yet
    QCOMPARE(acc->avatarRequirements().isValid(), false);
    // FeatureAvatar not ready yet
    QCOMPARE(acc->avatar().avatarData.isEmpty(), true);
    QCOMPARE(acc->avatar().MIMEType.isEmpty(), true);
    QCOMPARE(acc->parameters().size(), 1);
    QCOMPARE(acc->parameters().contains(QLatin1String("account")), true);
    QCOMPARE(qdbus_cast<QString>(acc->parameters().value(QLatin1String("account"))),
            QLatin1String("foobar"));
    // FeatureProtocolInfo not ready yet
    QCOMPARE(acc->protocolInfo().isValid(), false);
    // FeatureCapabilities not ready yet
    ConnectionCapabilities caps = acc->capabilities();
    QCOMPARE(caps.isSpecificToContact(), false);
    QCOMPARE(caps.textChats(), false);
    QCOMPARE(caps.streamedMediaCalls(), false);
    QCOMPARE(caps.streamedMediaAudioCalls(), false);
    QCOMPARE(caps.streamedMediaVideoCalls(), false);
    QCOMPARE(caps.streamedMediaVideoCallsWithAudio(), false);
    QCOMPARE(caps.upgradingStreamedMediaCalls(), false);
    QCOMPARE(caps.fileTransfers(), false);
    QCOMPARE(caps.textChatrooms(), false);
    QCOMPARE(caps.conferenceStreamedMediaCalls(), false);
    QCOMPARE(caps.conferenceStreamedMediaCallsWithInvitees(), false);
    QCOMPARE(caps.conferenceTextChats(), false);
    QCOMPARE(caps.conferenceTextChatsWithInvitees(), false);
    QCOMPARE(caps.conferenceTextChatrooms(), false);
    QCOMPARE(caps.conferenceTextChatroomsWithInvitees(), false);
    QCOMPARE(caps.contactSearches(), false);
    QCOMPARE(caps.contactSearchesWithSpecificServer(), false);
    QCOMPARE(caps.contactSearchesWithLimit(), false);
    QCOMPARE(caps.streamTubes(), false);
    QCOMPARE(caps.allClassSpecs().isEmpty(), true);
    QCOMPARE(acc->connectsAutomatically(), false);
    QCOMPARE(acc->hasBeenOnline(), false);
    QCOMPARE(acc->connectionStatus(), ConnectionStatusDisconnected);
    QCOMPARE(acc->connectionStatusReason(), ConnectionStatusReasonNoneSpecified);
    QCOMPARE(acc->connectionError().isEmpty(), true);
    QCOMPARE(acc->connectionErrorDetails().isValid(), false);
    QCOMPARE(acc->connectionErrorDetails().allDetails().isEmpty(), true);
    QVERIFY(!acc->connection());
    QCOMPARE(acc->isChangingPresence(), false);
    // Neither FeatureProtocolInfo or FeatureProfile are ready yet and we have no connection
    for (int i = 0; i < 2; ++i) {
        PresenceSpecList presences = acc->allowedPresenceStatuses(i);
        QCOMPARE(presences.size(), 2);
        QCOMPARE(presences[0].presence(), Presence::available());
        QCOMPARE(presences[0].maySetOnSelf(), true);
        QCOMPARE(presences[0].canHaveStatusMessage(), false);
        QCOMPARE(presences[1].presence(), Presence::offline());
        QCOMPARE(presences[1].maySetOnSelf(), true);
        QCOMPARE(presences[1].canHaveStatusMessage(), false);
    }
    // No connection
    QCOMPARE(acc->maxPresenceStatusMessageLength(), static_cast<uint>(0));
    QCOMPARE(acc->automaticPresence(), Presence::available());
    QCOMPARE(acc->currentPresence(), Presence::offline());
    QCOMPARE(acc->requestedPresence(), Presence::offline());
    QCOMPARE(acc->isOnline(), false);
    QCOMPARE(acc->uniqueIdentifier(), QLatin1String("foo/bar/Account0"));
    QCOMPARE(acc->normalizedName(), QLatin1String("bob"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, DisplayName, displayName, QLatin1String("foo@bar"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, IconName, iconName, QLatin1String("im-foo"));

    // Setting icon to an empty string should fallback to im-$protocol as FeatureProtocolInfo and
    // FeatureProtocolInfo are not ready yet
    mIconNameChanged = false;
    mIconName = QString();
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QString, IconName, iconName, iconNameChanged,
            acc->setIconName(QString()), QLatin1String("im-bar"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, Nickname, nickname, QLatin1String("Bob rocks!"));

    qDebug() << "making Account::FeatureAvatar ready";
    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);

    Avatar expectedAvatar = { QByteArray("asdfg"), QLatin1String("image/jpeg") };
    TEST_VERIFY_PROPERTY_CHANGE(acc, Tp::Avatar, Avatar, avatar, expectedAvatar);

    QVariantMap expectedParameters = acc->parameters();
    expectedParameters[QLatin1String("foo")] = QLatin1String("bar");
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QVariantMap, Parameters, parameters,
            parametersChanged, acc->updateParameters(expectedParameters, QStringList()), expectedParameters);

    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, bool,
            ConnectsAutomatically, connectsAutomatically, connectsAutomaticallyPropertyChanged,
            acc->setConnectsAutomatically(true), true);

    qDebug() << "creating another account";
    pacc = mAM->createAccount(QLatin1String("spurious"),
            QLatin1String("normal"), QLatin1String("foobar"), QVariantMap());
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    while (mAccountsCount != 2) {
        QCOMPARE(mLoop->exec(), 0);
    }
    processDBusQueue(mConn->client().data());

    acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(), true);

    QCOMPARE(acc->iconName(), QLatin1String("bob.png"));
    // Setting icon to an empty string should fallback to Profile/ProtocolInfo/im-$protocol
    mIconNameChanged = false;
    mIconName = QString();
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QString, IconName, iconName, iconNameChanged,
            acc->setIconName(QString()), QLatin1String("im-normal"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureProtocolInfo), true);

    // This time it's fetched from the protocol object (although it probably internally just
    // infers it from the protocol name too)
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));

    ProtocolInfo protocolInfo = acc->protocolInfo();
    QCOMPARE(protocolInfo.isValid(), true);
    QCOMPARE(protocolInfo.iconName(), QLatin1String("im-normal"));
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("account")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("password")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("register")), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureProfile),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureProfile), true);

    ProfilePtr profile = acc->profile();
    QCOMPARE(profile.isNull(), false);
    QCOMPARE(profile->isFake(), true);
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

    QCOMPARE(acc->serviceName(), acc->protocolName());
    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, ServiceName, serviceName,
            QLatin1String("spurious-service"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar), true);
    QCOMPARE(acc->avatar().avatarData.isEmpty(), true);
    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    // make redundant becomeReady calls
    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar | Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureAvatar | Account::FeatureProtocolInfo), true);

    QCOMPARE(acc->avatar().avatarData.isEmpty(), true);
    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));
    protocolInfo = acc->protocolInfo();
    QCOMPARE(protocolInfo.isValid(), true);
    QCOMPARE(protocolInfo.iconName(), QLatin1String("im-normal"));
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("account")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("password")), true);
    QCOMPARE(protocolInfo.hasParameter(QLatin1String("register")), true);

    QVERIFY(connect(acc->becomeReady(Account::FeatureCapabilities),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(Account::FeatureCapabilities), true);

    // using protocol info
    caps = acc->capabilities();
    QCOMPARE(caps.textChats(), true);

    // set new service name will change caps, icon and serviceName
    mCapabilitiesChanged = false;
    mCapabilities = ConnectionCapabilities();
    QVERIFY(connect(acc.data(),
                    SIGNAL(capabilitiesChanged(const Tp::ConnectionCapabilities &)),
                    SLOT(onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &))));
    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, ServiceName, serviceName,
            QLatin1String("test-profile"));
    while (!mCapabilitiesChanged) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // using merged protocol info caps and profile caps
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
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(acc->isReady(), true);

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

    // back to using merged protocol info caps and profile caps
    caps = acc->capabilities();
    QCOMPARE(caps.textChats(), false);

    processDBusQueue(mConn->client().data());
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
