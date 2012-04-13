#include <tests/lib/test.h>
#include <tests/lib/test-thread-helper.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/PendingString>

using namespace Tp;

class TestBaseProtocolCM;
typedef SharedPtr<TestBaseProtocolCM> TestBaseProtocolCMPtr;

class TestBaseProtocolCM : public BaseConnectionManager
{
public:
    TestBaseProtocolCM(const QDBusConnection &conn, const QString & name)
        : BaseConnectionManager(conn, name)
    { }

    static void createCM(TestBaseProtocolCMPtr &cm);

private:
    static BaseConnectionPtr createConnectionCb(const QVariantMap &parameters,
            Tp::DBusError *error);
    static QString identifyAccountCb(const QVariantMap &parameters, Tp::DBusError *error);
    static QString normalizeContactCb(const QString &contactId, Tp::DBusError *error);

    static QString normalizeVCardAddressCb(const QString &vcardField,
            const QString &vcardAddress, Tp::DBusError *error);
    static QString normalizeContactUriCb(const QString &uri, Tp::DBusError *error);
};

class TestBaseProtocol : public Test
{
    Q_OBJECT
public:
    TestBaseProtocol(QObject *parent = 0)
        : Test(parent)
    { }

private:
    static void protocolObjectSvcSideCb(TestBaseProtocolCMPtr &cm);
    static void addressingIfaceSvcSideCb(TestBaseProtocolCMPtr &cm);
    static void avatarsIfaceSvcSideCb(TestBaseProtocolCMPtr &cm);
    static void presenceIfaceSvcSideCb(TestBaseProtocolCMPtr &cm);

private Q_SLOTS:
    void initTestCase();
    void init();

    void protocolObjectSvcSide();
    void protocolObjectClientSide();
    void addressingIfaceSvcSide();
    void addressingIfaceClientSide();
    void avatarsIfaceSvcSide();
    void avatarsIfaceClientSide();
    void presenceIfaceSvcSide();
    void presenceIfaceClientSide();

    void cleanup();
    void cleanupTestCase();

private:
    TestThreadHelper<TestBaseProtocolCMPtr> *mThreadHelper;
};

void TestBaseProtocolCM::createCM(TestBaseProtocolCMPtr &cm)
{
    cm = BaseConnectionManager::create<TestBaseProtocolCM>(QLatin1String("testcm"));

    BaseProtocolPtr protocol = BaseProtocol::create(QLatin1String("example"));
    protocol->setConnectionInterfaces(QStringList() <<
            TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST);
    protocol->setParameters(ProtocolParameterList() <<
            ProtocolParameter(QLatin1String("account"), QDBusSignature("s"),
                    Tp::ConnMgrParamFlagRequired | Tp::ConnMgrParamFlagRegister));
    protocol->setRequestableChannelClasses(RequestableChannelClassSpec::textChat());
    protocol->setVCardField(QLatin1String("x-telepathy-example"));
    protocol->setEnglishName(QLatin1String("Test CM"));
    protocol->setIconName(QLatin1String("im-icq"));
    protocol->setAuthenticationTypes(QStringList() <<
            TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    protocol->setCreateConnectionCallback(ptrFun(&TestBaseProtocolCM::createConnectionCb));
    protocol->setIdentifyAccountCallback(ptrFun(&TestBaseProtocolCM::identifyAccountCb));
    protocol->setNormalizeContactCallback(ptrFun(&TestBaseProtocolCM::normalizeContactCb));

    BaseProtocolAddressingInterfacePtr addressingIface =
                BaseProtocolAddressingInterface::create();
    addressingIface->setAddressableUriSchemes(QStringList()
            << QLatin1String("xmpp")
            << QLatin1String("tel"));
    addressingIface->setAddressableVCardFields(QStringList()
            << QLatin1String("x-jabber")
            << QLatin1String("tel"));
    addressingIface->setNormalizeVCardAddressCallback(
            ptrFun(&TestBaseProtocolCM::normalizeVCardAddressCb));
    addressingIface->setNormalizeContactUriCallback(
            ptrFun(&TestBaseProtocolCM::normalizeContactUriCb));
    QVERIFY(protocol->plugInterface(addressingIface));

    BaseProtocolAvatarsInterfacePtr avatarsIface = BaseProtocolAvatarsInterface::create();
    avatarsIface->setAvatarDetails(
            AvatarSpec(QStringList()
                << QLatin1String("image/png")
                << QLatin1String("image/jpeg")
                << QLatin1String("image/gif"),
                32, 96, 64,
                32, 96, 64,
                37748736));
    QVERIFY(protocol->plugInterface(avatarsIface));

    BaseProtocolPresenceInterfacePtr presenceIface = BaseProtocolPresenceInterface::create();
    presenceIface->setStatuses(PresenceSpecList()
            << PresenceSpec::available()
            << PresenceSpec::away()
            << PresenceSpec::busy()
            << PresenceSpec::offline());
    QVERIFY(protocol->plugInterface(presenceIface));

    QVERIFY(cm->addProtocol(protocol));

    Tp::DBusError err;
    QVERIFY(cm->registerObject(&err));
    QVERIFY(!err.isValid());
    QVERIFY(cm->isRegistered());
}

BaseConnectionPtr TestBaseProtocolCM::createConnectionCb(const QVariantMap &parameters,
        Tp::DBusError *error)
{
    if (parameters.contains(QLatin1String("account"))) {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED,
                parameters.value(QLatin1String("account")).toString());
    } else {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("This test doesn't create connections"));
    }
    return BaseConnectionPtr();
}

QString TestBaseProtocolCM::identifyAccountCb(const QVariantMap &parameters, Tp::DBusError *error)
{
    QString account = parameters.value(QLatin1String("account")).toString();
    if (account.isEmpty()) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("'account' parameter not given"));
    }
    return account;
}

QString TestBaseProtocolCM::normalizeContactCb(const QString &contactId, Tp::DBusError *error)
{
    if (contactId.isEmpty()) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("ID must not be empty"));
        return QString();
    }

    return contactId.toLower();
}

QString TestBaseProtocolCM::normalizeVCardAddressCb(const QString &vcardField,
        const QString &vcardAddress, Tp::DBusError *error)
{
    if (vcardField == QLatin1String("x-jabber")) {
        return vcardAddress.toLower() + QLatin1String("@wonderland");
    } else {
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Invalid VCard field"));
        return QString();
    }
}

QString TestBaseProtocolCM::normalizeContactUriCb(const QString &uri, Tp::DBusError *error)
{
    if (uri.startsWith(QLatin1String("xmpp:"))) {
        if (uri.contains(QLatin1Char('/'))) {
            return uri.left(uri.indexOf(QLatin1Char('/')));
        } else {
            return uri;
        }
    } else {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Invalid URI"));
        return QString();
    }
}


void TestBaseProtocol::initTestCase()
{
    initTestCaseImpl();
}

void TestBaseProtocol::init()
{
    initImpl();
    mThreadHelper = new TestThreadHelper<TestBaseProtocolCMPtr>();
    TEST_THREAD_HELPER_EXECUTE(mThreadHelper, &TestBaseProtocolCM::createCM);
}

void TestBaseProtocol::protocolObjectSvcSideCb(TestBaseProtocolCMPtr &cm)
{
    QCOMPARE(cm->name(), QLatin1String("testcm"));
    QVERIFY(cm->hasProtocol(QLatin1String("example")));
    QCOMPARE(cm->protocols().size(), 1);

    BaseProtocolPtr protocol = cm->protocols().at(0);
    QVERIFY(protocol);

    //basic properties
    QCOMPARE(protocol->name(), QLatin1String("example"));
    QCOMPARE(protocol->vcardField(), QLatin1String("x-telepathy-example"));
    QCOMPARE(protocol->englishName(), QLatin1String("Test CM"));
    QCOMPARE(protocol->iconName(), QLatin1String("im-icq"));
    QCOMPARE(protocol->connectionInterfaces(), QStringList() <<
            TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST);
    QCOMPARE(protocol->authenticationTypes(), QStringList() <<
            TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    QCOMPARE(protocol->requestableChannelClasses().size(), 1);
    QCOMPARE(protocol->requestableChannelClasses().at(0),
            RequestableChannelClassSpec::textChat());

    //parameters
    QCOMPARE(protocol->parameters().size(), 1);
    QCOMPARE(protocol->parameters().at(0).name(), QLatin1String("account"));
    QCOMPARE(protocol->parameters().at(0).dbusSignature(), QDBusSignature("s"));
    QVERIFY(protocol->parameters().at(0).isRequired());
    QVERIFY(protocol->parameters().at(0).isRequiredForRegistration());
    QVERIFY(!protocol->parameters().at(0).isSecret());

    //interfaces
    QCOMPARE(protocol->interfaces().size(), 3);

    //immutable props
    QVariantMap props = protocol->immutableProperties();
    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".Interfaces")));

    QStringList sl = props.value(
            TP_QT_IFACE_PROTOCOL + QLatin1String(".Interfaces")).toStringList();
    QVERIFY(sl.contains(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING));
    QVERIFY(sl.contains(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS));
    QVERIFY(sl.contains(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE));

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".Parameters")));
    ParamSpecList params = qvariant_cast<ParamSpecList>(props.value(
            TP_QT_IFACE_PROTOCOL + QLatin1String(".Parameters")));
    QCOMPARE(params.size(), 1);
    QCOMPARE(params.at(0).name, QLatin1String("account"));
    QCOMPARE(params.at(0).signature, QLatin1String("s"));
    QCOMPARE(params.at(0).flags, uint(ConnMgrParamFlagRequired | ConnMgrParamFlagRegister));

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".VCardField")));
    QCOMPARE(props.value(TP_QT_IFACE_PROTOCOL + QLatin1String(".VCardField")).toString(),
             QLatin1String("x-telepathy-example"));

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".EnglishName")));
    QCOMPARE(props.value(TP_QT_IFACE_PROTOCOL + QLatin1String(".EnglishName")).toString(),
             QLatin1String("Test CM"));

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".Icon")));
    QCOMPARE(props.value(TP_QT_IFACE_PROTOCOL + QLatin1String(".Icon")).toString(),
             QLatin1String("im-icq"));

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".RequestableChannelClasses")));
    RequestableChannelClassList rcc = qvariant_cast<RequestableChannelClassList>(props.value(
            TP_QT_IFACE_PROTOCOL + QLatin1String(".RequestableChannelClasses")));
    QCOMPARE(rcc.size(), 1);
    QCOMPARE(RequestableChannelClassSpec(rcc.at(0)), RequestableChannelClassSpec::textChat());

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".ConnectionInterfaces")));
    sl = props.value(TP_QT_IFACE_PROTOCOL + QLatin1String(".ConnectionInterfaces")).toStringList();
    QCOMPARE(sl, QStringList() << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST);

    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL + QLatin1String(".AuthenticationTypes")));
    sl = props.value(TP_QT_IFACE_PROTOCOL + QLatin1String(".AuthenticationTypes")).toStringList();
    QCOMPARE(sl, QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);

    //interface immutable properties should also be here
    //test only one - the rest later
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight")).toInt(),
        32);

    //methods
    {
        Tp::DBusError err;
        QString normalizedContact = protocol->normalizeContact(QLatin1String("BoB"), &err);
        QVERIFY(!err.isValid());
        QCOMPARE(normalizedContact, QLatin1String("bob"));
    }

    {
        Tp::DBusError err;
        QString account = protocol->identifyAccount(QVariantMap(), &err);
        QVERIFY(err.isValid());
        QVERIFY(account.isEmpty());
        QCOMPARE(err.name(), TP_QT_ERROR_INVALID_ARGUMENT);
        QCOMPARE(err.message(), QLatin1String("'account' parameter not given"));
    }

    {
        Tp::DBusError err;
        BaseConnectionPtr conn = protocol->createConnection(QVariantMap(), &err);
        QVERIFY(err.isValid());
        QVERIFY(conn.isNull());
        QCOMPARE(err.name(), TP_QT_ERROR_NOT_IMPLEMENTED);
        QCOMPARE(err.message(), QLatin1String("This test doesn't create connections"));
    }
}

void TestBaseProtocol::protocolObjectSvcSide()
{
    TEST_THREAD_HELPER_EXECUTE(mThreadHelper, &TestBaseProtocol::protocolObjectSvcSideCb);
}

void TestBaseProtocol::protocolObjectClientSide()
{
    ConnectionManagerPtr cliCM = ConnectionManager::create(QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("example")));

    ProtocolInfo protocol = cliCM->protocol(QLatin1String("example"));
    QVERIFY(protocol.isValid());

    Tp::Client::ProtocolInterface protocolIface(cliCM->busName(),
            cliCM->objectPath() + QLatin1String("/example"));

    //basic properties
    QCOMPARE(protocol.vcardField(), QLatin1String("x-telepathy-example"));
    QCOMPARE(protocol.englishName(), QLatin1String("Test CM"));
    QCOMPARE(protocol.iconName(), QLatin1String("im-icq"));
    QCOMPARE(protocol.capabilities().allClassSpecs().size(), 1);
    QVERIFY(protocol.capabilities().textChats());

    PendingVariant *pv = protocolIface.requestPropertyConnectionInterfaces();
    connect(pv, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pv->result().toStringList(), QStringList() <<
            TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST);

    pv = protocolIface.requestPropertyAuthenticationTypes();
    connect(pv, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pv->result().toStringList(), QStringList() <<
            TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);

    //parameters
    QVERIFY(protocol.hasParameter(QLatin1String("account")));
    ProtocolParameterList params = protocol.parameters();
    QCOMPARE(params.size(), 1);
    QCOMPARE(params.at(0).name(), QLatin1String("account"));
    QCOMPARE(params.at(0).dbusSignature(), QDBusSignature("s"));
    QVERIFY(params.at(0).isRequired());
    QVERIFY(params.at(0).isRequiredForRegistration());
    QVERIFY(!params.at(0).isSecret());

    //methods
    {
        QDBusPendingReply<QString> reply = protocolIface.NormalizeContact(QLatin1String("ALiCe"));
        reply.waitForFinished();
        QVERIFY(!reply.isError());
        QCOMPARE(reply.value(), QLatin1String("alice"));
    }

    QVariantMap map;
    map.insert(QLatin1String("account"), QLatin1String("example@nowhere.com"));

    {
        QDBusPendingReply<QString> reply = protocolIface.IdentifyAccount(map);
        reply.waitForFinished();
        QVERIFY(!reply.isError());
        QCOMPARE(reply.value(), QLatin1String("example@nowhere.com"));
    }

    PendingConnection *pc = cliCM->lowlevel()->requestConnection(QLatin1String("example"), map);
    connect(pc, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_NOT_IMPLEMENTED);
    QCOMPARE(mLastErrorMessage, QLatin1String("example@nowhere.com"));
}

void TestBaseProtocol::addressingIfaceSvcSideCb(TestBaseProtocolCMPtr &cm)
{
    QCOMPARE(cm->name(), QLatin1String("testcm"));
    QVERIFY(cm->hasProtocol(QLatin1String("example")));
    QCOMPARE(cm->protocols().size(), 1);

    Tp::BaseProtocolPtr protocol = cm->protocols().at(0);
    QVERIFY(protocol);

    Tp::BaseProtocolAddressingInterfacePtr iface =
            Tp::BaseProtocolAddressingInterfacePtr::qObjectCast(
                    protocol->interface(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING));
    QVERIFY(iface);

    //properties
    QStringList uriSchemes = iface->addressableUriSchemes();
    QCOMPARE(uriSchemes.size(), 2);
    QVERIFY(uriSchemes.contains(QLatin1String("xmpp")));
    QVERIFY(uriSchemes.contains(QLatin1String("tel")));

    QStringList vcardFields = iface->addressableVCardFields();
    QCOMPARE(vcardFields.size(), 2);
    QVERIFY(vcardFields.contains(QLatin1String("x-jabber")));
    QVERIFY(vcardFields.contains(QLatin1String("tel")));

    //no immutable properties
    QVERIFY(iface->immutableProperties().isEmpty());

    //methods
    {
        Tp::DBusError err;
        QString result = iface->normalizeVCardAddress(QLatin1String("x-msn"),
                QLatin1String("Alice"), &err);
        QVERIFY(err.isValid());
        QVERIFY(result.isEmpty());
        QCOMPARE(err.name(), TP_QT_ERROR_NOT_IMPLEMENTED);
        QCOMPARE(err.message(), QLatin1String("Invalid VCard field"));
    }

    {
        Tp::DBusError err;
        QString result = iface->normalizeContactUri(
                QLatin1String("xmpp:alice@wonderland/Mobile"), &err);
        QVERIFY(!err.isValid());
        QCOMPARE(result, QLatin1String("xmpp:alice@wonderland"));
    }
}

void TestBaseProtocol::addressingIfaceSvcSide()
{
    TEST_THREAD_HELPER_EXECUTE(mThreadHelper, &TestBaseProtocol::addressingIfaceSvcSideCb);
}

void TestBaseProtocol::addressingIfaceClientSide()
{
    ConnectionManagerPtr cliCM = ConnectionManager::create(QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("example")));

    ProtocolInfo protocol = cliCM->protocol(QLatin1String("example"));
    QVERIFY(protocol.isValid());

    //properties
    QStringList uriSchemes = protocol.addressableUriSchemes();
    QCOMPARE(uriSchemes.size(), 2);
    QVERIFY(uriSchemes.contains(QLatin1String("xmpp")));
    QVERIFY(uriSchemes.contains(QLatin1String("tel")));

    QStringList vcardFields = protocol.addressableVCardFields();
    QCOMPARE(vcardFields.size(), 2);
    QVERIFY(vcardFields.contains(QLatin1String("x-jabber")));
    QVERIFY(vcardFields.contains(QLatin1String("tel")));

    //methods
    PendingString *str = protocol.normalizeVCardAddress(QLatin1String("x-jabber"),
                QLatin1String("Alice"));
    connect(str, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(str->result(), QLatin1String("alice@wonderland"));

    str = protocol.normalizeContactUri(QLatin1String("invalid"));
    connect(str, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectFailure(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mLastError, TP_QT_ERROR_INVALID_ARGUMENT);
    QCOMPARE(mLastErrorMessage, QLatin1String("Invalid URI"));
}

void TestBaseProtocol::avatarsIfaceSvcSideCb(TestBaseProtocolCMPtr &cm)
{
    QCOMPARE(cm->name(), QLatin1String("testcm"));
    QVERIFY(cm->hasProtocol(QLatin1String("example")));
    QCOMPARE(cm->protocols().size(), 1);

    Tp::BaseProtocolPtr protocol = cm->protocols().at(0);
    QVERIFY(protocol);

    Tp::BaseProtocolAvatarsInterfacePtr iface =
            Tp::BaseProtocolAvatarsInterfacePtr::qObjectCast(
                    protocol->interface(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS));
    QVERIFY(iface);

    //avatar details property
    AvatarSpec avatarSpec = iface->avatarDetails();
    QVERIFY(avatarSpec.isValid());

    QStringList mimeTypes = avatarSpec.supportedMimeTypes();
    QCOMPARE(mimeTypes.size(), 3);
    QVERIFY(mimeTypes.contains(QLatin1String("image/png")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/jpeg")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/gif")));

    QCOMPARE(avatarSpec.minimumWidth(), 32U);
    QCOMPARE(avatarSpec.maximumWidth(), 96U);
    QCOMPARE(avatarSpec.recommendedWidth(), 64U);
    QCOMPARE(avatarSpec.minimumHeight(), 32U);
    QCOMPARE(avatarSpec.maximumHeight(), 96U);
    QCOMPARE(avatarSpec.recommendedHeight(), 64U);
    QCOMPARE(avatarSpec.maximumBytes(), 37748736U);

    //immutable properties
    QVariantMap props = protocol->immutableProperties();

    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".SupportedAvatarMIMETypes")));
    mimeTypes = props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".SupportedAvatarMIMETypes"))
            .toStringList();
    QCOMPARE(mimeTypes.size(), 3);
    QVERIFY(mimeTypes.contains(QLatin1String("image/png")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/jpeg")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/gif")));

    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight")).toInt(),
        32);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarWidth")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarWidth")).toInt(),
        32);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarHeight")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarHeight")).toInt(),
        64);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarWidth")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarWidth")).toInt(),
        64);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarHeight")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarHeight")).toInt(),
        96);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarWidth")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarWidth")).toInt(),
        96);
    QVERIFY(props.contains(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarBytes")));
    QCOMPARE(props.value(
        TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarBytes")).toInt(),
        37748736);
}

void TestBaseProtocol::avatarsIfaceSvcSide()
{
    TEST_THREAD_HELPER_EXECUTE(mThreadHelper, &TestBaseProtocol::avatarsIfaceSvcSideCb);
}

void TestBaseProtocol::avatarsIfaceClientSide()
{
    ConnectionManagerPtr cliCM = ConnectionManager::create(QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("example")));

    ProtocolInfo protocol = cliCM->protocol(QLatin1String("example"));
    QVERIFY(protocol.isValid());

    //avatar iface
    AvatarSpec avatarSpec = protocol.avatarRequirements();
    QVERIFY(avatarSpec.isValid());

    QStringList mimeTypes = avatarSpec.supportedMimeTypes();
    QCOMPARE(mimeTypes.size(), 3);
    QVERIFY(mimeTypes.contains(QLatin1String("image/png")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/jpeg")));
    QVERIFY(mimeTypes.contains(QLatin1String("image/gif")));

    QCOMPARE(avatarSpec.minimumWidth(), 32U);
    QCOMPARE(avatarSpec.maximumWidth(), 96U);
    QCOMPARE(avatarSpec.recommendedWidth(), 64U);
    QCOMPARE(avatarSpec.minimumHeight(), 32U);
    QCOMPARE(avatarSpec.maximumHeight(), 96U);
    QCOMPARE(avatarSpec.recommendedHeight(), 64U);
    QCOMPARE(avatarSpec.maximumBytes(), 37748736U);
}

void TestBaseProtocol::presenceIfaceSvcSideCb(TestBaseProtocolCMPtr &cm)
{
    QCOMPARE(cm->name(), QLatin1String("testcm"));
    QVERIFY(cm->hasProtocol(QLatin1String("example")));
    QCOMPARE(cm->protocols().size(), 1);

    Tp::BaseProtocolPtr protocol = cm->protocols().at(0);
    QVERIFY(protocol);

    Tp::BaseProtocolPresenceInterfacePtr iface =
            Tp::BaseProtocolPresenceInterfacePtr::qObjectCast(
                    protocol->interface(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE));
    QVERIFY(iface);

    //presence interface
    PresenceSpecList statuses = iface->statuses();
    QCOMPARE(statuses.size(), 4);
    QVERIFY(statuses.contains(PresenceSpec::available()));
    QVERIFY(statuses.contains(PresenceSpec::away()));
    QVERIFY(statuses.contains(PresenceSpec::busy()));
    QVERIFY(statuses.contains(PresenceSpec::offline()));
    QVERIFY(!statuses.contains(PresenceSpec::xa()));

    //immutable properties
    QVariantMap props = protocol->immutableProperties();
    QVERIFY(props.contains(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE + QLatin1String(".Statuses")));
    statuses = PresenceSpecList(qvariant_cast<SimpleStatusSpecMap>(props.value(
            TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE + QLatin1String(".Statuses"))));
    QCOMPARE(statuses.size(), 4);
    QVERIFY(statuses.contains(PresenceSpec::available()));
    QVERIFY(statuses.contains(PresenceSpec::away()));
    QVERIFY(statuses.contains(PresenceSpec::busy()));
    QVERIFY(statuses.contains(PresenceSpec::offline()));
    QVERIFY(!statuses.contains(PresenceSpec::xa()));
}

void TestBaseProtocol::presenceIfaceSvcSide()
{
    TEST_THREAD_HELPER_EXECUTE(mThreadHelper, &TestBaseProtocol::presenceIfaceSvcSideCb);
}

void TestBaseProtocol::presenceIfaceClientSide()
{
    ConnectionManagerPtr cliCM = ConnectionManager::create(QLatin1String("testcm"));
    PendingReady *pr = cliCM->becomeReady(ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliCM->supportedProtocols().size(), 1);
    QVERIFY(cliCM->hasProtocol(QLatin1String("example")));

    ProtocolInfo protocol = cliCM->protocol(QLatin1String("example"));
    QVERIFY(protocol.isValid());

    //presence interface
    PresenceSpecList statuses = protocol.allowedPresenceStatuses();
    QCOMPARE(statuses.size(), 4);
    QVERIFY(statuses.contains(PresenceSpec::available()));
    QVERIFY(statuses.contains(PresenceSpec::away()));
    QVERIFY(statuses.contains(PresenceSpec::busy()));
    QVERIFY(statuses.contains(PresenceSpec::offline()));
    QVERIFY(!statuses.contains(PresenceSpec::xa()));
}

void TestBaseProtocol::cleanup()
{
    delete mThreadHelper;
    cleanupImpl();
}

void TestBaseProtocol::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestBaseProtocol)
#include "_gen/base-protocol.cpp.moc.hpp"
