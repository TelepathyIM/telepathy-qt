#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/echo2/conn.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/PendingAccount>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingStringList>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/Profile>

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
    void onAccountAutomaticPresenceChanged(const Tp::Presence &);
    void onAccountRequestedPresenceChanged(const Tp::Presence &);
    void onAccountCurrentPresenceChanged(const Tp::Presence &);

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
    bool mCreatingAccount;

    QHash<QString, QVariant> mProps;
};

#define TEST_VERIFY_PROPERTY_CHANGE(acc, Type, PropertyName, propertyName, expectedValue) \
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, Type, PropertyName, propertyName, \
            propertyName ## Changed, acc->set ## PropertyName(expectedValue), expectedValue)

#define TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, Type, PropertyName, propertyName, signalName, po, expectedValue) \
    { \
        mProps.clear(); \
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
        if (!mProps.contains(QLatin1String(#PropertyName))) { \
            qDebug().nospace() << "waiting for the " << #propertyName << "Changed signal"; \
            QCOMPARE(mLoop->exec(), 0); \
        } else { \
            qDebug().nospace() << "not waiting for " << #propertyName << "Changed because we already got it"; \
        } \
        \
        QCOMPARE(acc->propertyName(), expectedValue); \
        QCOMPARE(acc->propertyName(), \
                 mProps[QLatin1String(#PropertyName)].value< Type >()); \
        \
        QVERIFY(disconnect(acc.data(), \
                    SIGNAL(signalName(Type)), \
                    this, \
                    SLOT(onAccount ## PropertyName ## Changed(Type)))); \
        processDBusQueue(acc.data()); \
    }

#define TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(Type, PropertyName) \
void TestAccountBasics::onAccount ## PropertyName ## Changed(Type value) \
{ \
    mProps[QLatin1String(#PropertyName)] = qVariantFromValue(value); \
    mLoop->exit(0); \
}

void TestAccountBasics::onNewAccount(const Tp::AccountPtr &acc)
{
    Q_UNUSED(acc);

    mAccountsCount++;
    if (!mCreatingAccount) {
        mLoop->exit(0);
    }
}

TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, ServiceName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, DisplayName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, IconName)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QString &, Nickname)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const Avatar &, Avatar)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const QVariantMap &, Parameters)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const ConnectionCapabilities &, Capabilities)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(bool, ConnectsAutomatically)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const Presence &, AutomaticPresence)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const Presence &, RequestedPresence)
TEST_IMPLEMENT_PROPERTY_CHANGE_SLOT(const Presence &, CurrentPresence)

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
    QVERIFY(!mAM->isReady());

    mConn = new TestConnHelper(this,
            EXAMPLE_TYPE_ECHO_2_CONNECTION,
            "account", "me@example.com",
            "protocol", "echo2",
            NULL);
    QVERIFY(mConn->connect());
}

void TestAccountBasics::init()
{
    mProps.clear();
    mCreatingAccount = false;

    initImpl();
}

void TestAccountBasics::testBasics()
{
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(mAM->isReady());
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
    mCreatingAccount = true;
    QCOMPARE(mLoop->exec(), 0);
    mCreatingAccount = false;
    QVERIFY(pacc->account());

    while (mAccountsCount != 1) {
        QCOMPARE(mLoop->exec(), 0);
    }
    processDBusQueue(mConn->client().data());

    QStringList paths;
    QString accPath(QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    QCOMPARE(pathsForAccounts(mAM->allAccounts()), QStringList() << accPath);
    QList<AccountPtr> accs = mAM->accountsForObjectPaths(
            QStringList() << accPath << QLatin1String("/invalid/path"));
    QCOMPARE(accs.size(), 2);
    QCOMPARE(accs[0]->objectPath(), accPath);
    QVERIFY(!accs[1]);

    QVERIFY(mAM->allAccounts()[0]->isReady(
                Account::FeatureCore | Account::FeatureCapabilities));

    AccountPtr acc = Account::create(mAM->dbusConnection(), mAM->busName(),
            QLatin1String("/org/freedesktop/Telepathy/Account/foo/bar/Account0"),
            mAM->connectionFactory(), mAM->channelFactory(), mAM->contactFactory());
    QVERIFY(connect(acc->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady());

    QCOMPARE(acc->connectionFactory(), mAM->connectionFactory());
    QCOMPARE(acc->channelFactory(), mAM->channelFactory());
    QCOMPARE(acc->contactFactory(), mAM->contactFactory());
    QVERIFY(acc->isValidAccount());
    QVERIFY(acc->isEnabled());
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
    QVERIFY(!acc->avatarRequirements().isValid());
    // FeatureAvatar not ready yet
    QVERIFY(acc->avatar().avatarData.isEmpty());
    QVERIFY(acc->avatar().MIMEType.isEmpty());
    QCOMPARE(acc->parameters().size(), 1);
    QVERIFY(acc->parameters().contains(QLatin1String("account")));
    QCOMPARE(qdbus_cast<QString>(acc->parameters().value(QLatin1String("account"))),
            QLatin1String("foobar"));
    // FeatureProtocolInfo not ready yet
    QVERIFY(!acc->protocolInfo().isValid());
    // FeatureCapabilities not ready yet
    ConnectionCapabilities caps = acc->capabilities();
    QVERIFY(!caps.isSpecificToContact());
    QVERIFY(!caps.textChats());
    QVERIFY(!caps.streamedMediaCalls());
    QVERIFY(!caps.streamedMediaAudioCalls());
    QVERIFY(!caps.streamedMediaVideoCalls());
    QVERIFY(!caps.streamedMediaVideoCallsWithAudio());
    QVERIFY(!caps.upgradingStreamedMediaCalls());
    QVERIFY(!caps.fileTransfers());
    QVERIFY(!caps.textChatrooms());
    QVERIFY(!caps.conferenceStreamedMediaCalls());
    QVERIFY(!caps.conferenceStreamedMediaCallsWithInvitees());
    QVERIFY(!caps.conferenceTextChats());
    QVERIFY(!caps.conferenceTextChatsWithInvitees());
    QVERIFY(!caps.conferenceTextChatrooms());
    QVERIFY(!caps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!caps.contactSearches());
    QVERIFY(!caps.contactSearchesWithSpecificServer());
    QVERIFY(!caps.contactSearchesWithLimit());
    QVERIFY(!caps.streamTubes());
    QVERIFY(caps.allClassSpecs().isEmpty());
    QVERIFY(!acc->connectsAutomatically());
    QVERIFY(!acc->hasBeenOnline());
    QCOMPARE(acc->connectionStatus(), ConnectionStatusDisconnected);
    QCOMPARE(acc->connectionStatusReason(), ConnectionStatusReasonNoneSpecified);
    QVERIFY(acc->connectionError().isEmpty());
    QVERIFY(!acc->connectionErrorDetails().isValid());
    QVERIFY(acc->connectionErrorDetails().allDetails().isEmpty());
    QVERIFY(!acc->connection());
    QVERIFY(!acc->isChangingPresence());
    // Neither FeatureProtocolInfo or FeatureProfile are ready yet and we have no connection
    PresenceSpecList expectedPresences;
    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeAvailable, true, false };
        expectedPresences.append(PresenceSpec(QLatin1String("available"), prSpec));
    }
    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeOffline, true, false };
        expectedPresences.append(PresenceSpec(QLatin1String("offline"), prSpec));
    }
    qSort(expectedPresences);

    PresenceSpecList presences = acc->allowedPresenceStatuses(false);
    qSort(presences);
    QCOMPARE(presences.size(), 2);
    QCOMPARE(presences, expectedPresences);

    presences = acc->allowedPresenceStatuses(true);
    qSort(presences);
    QCOMPARE(presences.size(), 2);
    QCOMPARE(presences, expectedPresences);

    // No connection
    QCOMPARE(acc->maxPresenceStatusMessageLength(), static_cast<uint>(0));
    QCOMPARE(acc->automaticPresence(), Presence::available());
    QCOMPARE(acc->currentPresence(), Presence::offline());
    QCOMPARE(acc->requestedPresence(), Presence::offline());
    QVERIFY(!acc->isOnline());
    QCOMPARE(acc->uniqueIdentifier(), QLatin1String("foo/bar/Account0"));
    QCOMPARE(acc->normalizedName(), QLatin1String("bob"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, DisplayName, displayName, QLatin1String("foo@bar"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, IconName, iconName, QLatin1String("im-foo"));

    // Setting icon to an empty string should fallback to im-$protocol as FeatureProtocolInfo and
    // FeatureProtocolInfo are not ready yet
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QString, IconName, iconName, iconNameChanged,
            acc->setIconName(QString()), QLatin1String("im-bar"));

    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, Nickname, nickname, QLatin1String("Bob rocks!"));

    qDebug() << "making Account::FeatureAvatar ready";
    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureAvatar));

    Avatar expectedAvatar = { QByteArray("asdfg"), QLatin1String("image/jpeg") };
    TEST_VERIFY_PROPERTY_CHANGE(acc, Tp::Avatar, Avatar, avatar, expectedAvatar);

    QVariantMap expectedParameters = acc->parameters();
    expectedParameters[QLatin1String("foo")] = QLatin1String("bar");
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QVariantMap, Parameters, parameters,
            parametersChanged, acc->updateParameters(expectedParameters, QStringList()), expectedParameters);

    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, bool,
            ConnectsAutomatically, connectsAutomatically, connectsAutomaticallyPropertyChanged,
            acc->setConnectsAutomatically(true), true);

    TEST_VERIFY_PROPERTY_CHANGE(acc, Tp::Presence, AutomaticPresence, automaticPresence,
            Presence::busy());

    // Changing requested presence will also change hasBeenOnline/isOnline/currentPresence
    Presence expectedPresence = Presence::busy();
    TEST_VERIFY_PROPERTY_CHANGE(acc, Tp::Presence, RequestedPresence, requestedPresence,
            expectedPresence);
    QVERIFY(acc->hasBeenOnline());
    QVERIFY(acc->isOnline());
    QCOMPARE(acc->currentPresence(), expectedPresence);

    qDebug() << "creating another account";
    pacc = mAM->createAccount(QLatin1String("spurious"),
            QLatin1String("normal"), QLatin1String("foobar"), QVariantMap());
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    mCreatingAccount = true;
    QCOMPARE(mLoop->exec(), 0);
    mCreatingAccount = false;

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
    QVERIFY(acc->isReady());

    QCOMPARE(acc->iconName(), QLatin1String("bob.png"));
    // Setting icon to an empty string should fallback to Profile/ProtocolInfo/im-$protocol
    TEST_VERIFY_PROPERTY_CHANGE_EXTENDED(acc, QString, IconName, iconName, iconNameChanged,
            acc->setIconName(QString()), QLatin1String("im-normal"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureProtocolInfo));

    // This time it's fetched from the protocol object (although it probably internally just
    // infers it from the protocol name too)
    QCOMPARE(acc->iconName(), QLatin1String("im-normal"));

    ProtocolInfo protocolInfo = acc->protocolInfo();
    QVERIFY(protocolInfo.isValid());
    QCOMPARE(protocolInfo.iconName(), QLatin1String("im-normal"));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("account")));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("password")));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("register")));
    QVERIFY(!protocolInfo.hasParameter(QLatin1String("bogusparam")));
    QCOMPARE(protocolInfo.parameters().size(), 3);

    QVERIFY(connect(acc->becomeReady(Account::FeatureProfile),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureProfile));

    ProfilePtr profile = acc->profile();
    QVERIFY(!profile.isNull());
    QVERIFY(profile->isFake());
    QVERIFY(profile->isValid());
    QCOMPARE(profile->serviceName(), QString(QLatin1String("%1-%2"))
                .arg(acc->cmName()).arg(acc->serviceName()));
    QCOMPARE(profile->type(), QLatin1String("IM"));
    QCOMPARE(profile->provider(), QString());
    QCOMPARE(profile->name(), acc->protocolName());
    QCOMPARE(profile->cmName(), acc->cmName());
    QCOMPARE(profile->protocolName(), acc->protocolName());
    QVERIFY(!profile->parameters().isEmpty());
    QVERIFY(profile->allowOtherPresences());
    QVERIFY(profile->presences().isEmpty());
    QVERIFY(profile->unsupportedChannelClassSpecs().isEmpty());

    QCOMPARE(acc->serviceName(), acc->protocolName());
    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, ServiceName, serviceName,
            QLatin1String("spurious-service"));

    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureAvatar));
    QVERIFY(acc->avatar().avatarData.isEmpty());
    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));

    // make redundant becomeReady calls
    QVERIFY(connect(acc->becomeReady(Account::FeatureAvatar | Account::FeatureProtocolInfo),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureAvatar | Account::FeatureProtocolInfo));

    QVERIFY(acc->avatar().avatarData.isEmpty());
    QCOMPARE(acc->avatar().MIMEType, QString(QLatin1String("image/png")));
    protocolInfo = acc->protocolInfo();
    QVERIFY(protocolInfo.isValid());
    QCOMPARE(protocolInfo.iconName(), QLatin1String("im-normal"));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("account")));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("password")));
    QVERIFY(protocolInfo.hasParameter(QLatin1String("register")));

    QVERIFY(connect(acc->becomeReady(Account::FeatureCapabilities),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(acc->isReady(Account::FeatureCapabilities));

    // using protocol info
    caps = acc->capabilities();
    QVERIFY(caps.textChats());

    // set new service name will change caps, icon and serviceName
    QVERIFY(connect(acc.data(),
                    SIGNAL(capabilitiesChanged(const Tp::ConnectionCapabilities &)),
                    SLOT(onAccountCapabilitiesChanged(const Tp::ConnectionCapabilities &))));
    TEST_VERIFY_PROPERTY_CHANGE(acc, QString, ServiceName, serviceName,
            QLatin1String("test-profile"));
    while (!mProps.contains(QLatin1String("IconName")) &&
           !mProps.contains(QLatin1String("Capabilities"))) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // Now that both FeatureProtocolInfo and FeatureProfile are ready, let's check the allowed
    // presences
    expectedPresences.clear();
    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeAvailable, true, true };
        expectedPresences.append(PresenceSpec(QLatin1String("available"), prSpec));
    }
    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeAway, true, true };
        expectedPresences.append(PresenceSpec(QLatin1String("away"), prSpec));
    }
    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeOffline, true, false };
        expectedPresences.append(PresenceSpec(QLatin1String("offline"), prSpec));
    }
    qSort(expectedPresences);

    presences = acc->allowedPresenceStatuses(false);
    qSort(presences);
    QCOMPARE(presences.size(), 3);
    QCOMPARE(presences, expectedPresences);

    {
        SimpleStatusSpec prSpec = { ConnectionPresenceTypeExtendedAway, false, false };
        expectedPresences.append(PresenceSpec(QLatin1String("xa"), prSpec));
    }
    qSort(expectedPresences);

    presences = acc->allowedPresenceStatuses(true);
    qSort(presences);
    QCOMPARE(presences.size(), 4);
    QCOMPARE(presences, expectedPresences);

    QCOMPARE(acc->iconName(), QLatin1String("test-profile-icon"));

    // using merged protocol info caps and profile caps
    caps = acc->capabilities();
    QVERIFY(!caps.textChats());

    Client::DBus::PropertiesInterface *accPropertiesInterface =
        acc->interface<Client::DBus::PropertiesInterface>();

    // simulate that the account has a connection
    QVERIFY(connect(new PendingVoid(
                        accPropertiesInterface->Set(
                            TP_QT_IFACE_ACCOUNT,
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
    QVERIFY(acc->isReady());

    // once the status change the capabilities will be updated
    mProps.clear();
    QVERIFY(connect(acc->setRequestedPresence(Presence::available()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!mProps.contains(QLatin1String("Capabilities"))) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // using connection caps now
    caps = acc->capabilities();
    QVERIFY(caps.textChats());
    QVERIFY(!caps.textChatrooms());
    QVERIFY(!caps.streamedMediaCalls());
    QVERIFY(!caps.streamedMediaAudioCalls());
    QVERIFY(!caps.streamedMediaVideoCalls());
    QVERIFY(!caps.streamedMediaVideoCallsWithAudio());
    QVERIFY(!caps.upgradingStreamedMediaCalls());

    // once the status change the capabilities will be updated
    mProps.clear();
    QVERIFY(connect(new PendingVoid(
                        accPropertiesInterface->Set(
                            TP_QT_IFACE_ACCOUNT,
                            QLatin1String("Connection"),
                            QDBusVariant(QLatin1String("/"))),
                        acc),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QVERIFY(connect(acc->setRequestedPresence(Presence::offline()),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    while (!mProps.contains(QLatin1String("Capabilities"))) {
        QCOMPARE(mLoop->exec(), 0);
    }

    // back to using merged protocol info caps and profile caps
    caps = acc->capabilities();
    QVERIFY(!caps.textChats());

    processDBusQueue(mConn->client().data());
}

void TestAccountBasics::cleanup()
{
    cleanupImpl();
}

void TestAccountBasics::cleanupTestCase()
{
    if (mConn) {
        QVERIFY(mConn->disconnect());
        delete mConn;
    }

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountBasics)
#include "_gen/account-basics.cpp.moc.hpp"
