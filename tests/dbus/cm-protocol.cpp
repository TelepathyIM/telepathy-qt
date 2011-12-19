#include <tests/lib/test.h>

#include <TelepathyQt/Constants>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/Types>

#include <QtDBus/QtDBus>

using namespace Tp;
using namespace Tp::Client;

namespace
{

PresenceSpec getPresenceSpec(const PresenceSpecList &specs, const QString &status)
{
    Q_FOREACH (const PresenceSpec &spec, specs) {
        if (spec.presence().status() == status) {
            return spec;
        }
    }
    return PresenceSpec();
}

}

class ConnectionManagerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ConnectionManager")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ConnectionManager\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"Protocols\" type=\"a{sa{sv}}\" access=\"read\" >\n"
"      <annotation name=\"com.trolltech.QtDBus.QtTypeName\" value=\"Tp::ProtocolPropertiesMap\" />\n"
"    </property>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(Tp::ProtocolPropertiesMap Protocols READ Protocols)

public:
    ConnectionManagerAdaptor(ProtocolPropertiesMap &protocols, QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mProtocols(protocols)
    {
    }

    virtual ~ConnectionManagerAdaptor()
    {
    }

public: // Properties
    inline QStringList Interfaces() const
    {
        return QStringList();
    }

    inline ProtocolPropertiesMap Protocols() const
    {
        return mProtocols;
    }

private:
    ProtocolPropertiesMap mProtocols;
    ParamSpecList mParameters;
};

class ProtocolAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Protocol")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Protocol\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"Parameters\" type=\"a(susv)\" access=\"read\" >\n"
"      <annotation name=\"com.trolltech.QtDBus.QtTypeName\" value=\"Tp::ParamSpecList\" />\n"
"    </property>\n"
"    <property name=\"ConnectionInterfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"RequestableChannelClasses\" type=\"a(a{sv}as)\" access=\"read\" >\n"
"      <annotation name=\"com.trolltech.QtDBus.QtTypeName\" value=\"Tp::RequestableChannelClassList\" />\n"
"    </property>\n"
"    <property name=\"VCardField\" type=\"s\" access=\"read\" />\n"
"    <property name=\"EnglishName\" type=\"s\" access=\"read\" />\n"
"    <property name=\"Icon\" type=\"s\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(Tp::ParamSpecList Parameters READ Parameters)
    Q_PROPERTY(QStringList ConnectionInterfaces READ ConnectionInterfaces)
    Q_PROPERTY(Tp::RequestableChannelClassList RequestableChannelClasses READ RequestableChannelClasses)
    Q_PROPERTY(QString VCardField READ VCardField)
    Q_PROPERTY(QString EnglishName READ EnglishName)
    Q_PROPERTY(QString Icon READ Icon)

public:
    ProtocolAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent),
          introspectionCalled(0)
    {
        mInterfaces << TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING <<
                       TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS <<
                       TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE;

        mConnInterfaces << TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS;

        mRCCs.append(RequestableChannelClassSpec::textChatroom().bareClass());

        mVCardField = QLatin1String("x-adaptor");
        mEnglishName = QLatin1String("Adaptor");
        mIcon = QLatin1String("icon-adaptor");
    }

    virtual ~ProtocolAdaptor()
    {
    }

    inline QVariantMap immutableProperties() const
    {
        QVariantMap ret;
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Interfaces"),
                   mInterfaces);
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Parameters"),
                   qVariantFromValue(mParameters));
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".ConnectionInterfaces"),
                   mConnInterfaces);
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".RequestableChannelClasses"),
                   qVariantFromValue(mRCCs));
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".VCardField"),
                   mVCardField);
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".EnglishName"),
                   mEnglishName);
        ret.insert(TP_QT_IFACE_PROTOCOL + QLatin1String(".Icon"),
                   mIcon);
        return ret;
    }

public: // Properties
    inline QStringList Interfaces() const
    {
        // if we request all properties we are going to get here, so marking as
        // introspectionCalled;
        introspectionCalled++;
        return mInterfaces;
    }

    inline Tp::ParamSpecList Parameters() const
    {
        return mParameters;
    }

    inline QStringList ConnectionInterfaces() const
    {
        return mConnInterfaces;
    }

    inline Tp::RequestableChannelClassList RequestableChannelClasses() const
    {
        return mRCCs;
    }

    inline QString VCardField() const
    {
        return mVCardField;
    }

    inline QString EnglishName() const
    {
        return mEnglishName;
    }

    inline QString Icon() const
    {
        return mIcon;
    }

    mutable int introspectionCalled;

private:
    QStringList mInterfaces;
    ParamSpecList mParameters;
    QStringList mConnInterfaces;
    RequestableChannelClassList mRCCs;
    QString mVCardField;
    QString mEnglishName;
    QString mIcon;
};

class ProtocolAddressingAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Protocol.Interface.Addressing")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Protocol.Interface.Addressing\" >\n"
"    <property name=\"AddressableVCardFields\" type=\"as\" access=\"read\" />\n"
"    <property name=\"AddressableURISchemes\" type=\"as\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList AddressableVCardFields READ AddressableVCardFields)
    Q_PROPERTY(QStringList AddressableURISchemes READ AddressableURISchemes)

public:
    ProtocolAddressingAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent),
          introspectionCalled(0)
    {
        mVCardFields << QLatin1String("x-adaptor");
        mUris << QLatin1String("adaptor");
    }

    virtual ~ProtocolAddressingAdaptor()
    {
    }

    inline QVariantMap immutableProperties() const
    {
        QVariantMap ret;
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING + QLatin1String(".AddressableVCardFields"),
                   mVCardFields);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_ADDRESSING + QLatin1String(".AddressableURISchemes"),
                   mUris);
        return ret;
    }


public: // Properties
    inline QStringList AddressableVCardFields() const
    {
        // if we request all properties we are going to get here, so marking as
        // introspectionCalled;
        introspectionCalled++;
        return mVCardFields;
    }

    inline QStringList AddressableURISchemes() const
    {
        return mUris;
    }

    mutable int introspectionCalled;

private:
    QStringList mVCardFields;
    QStringList mUris;
};

class ProtocolAvatarsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Protocol.Interface.Avatars")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Protocol.Interface.Avatars\" >\n"
"    <property name=\"SupportedAvatarMIMETypes\" type=\"as\" access=\"read\" />\n"
"    <property name=\"MinimumAvatarHeight type=\"u\" access=\"read\" />\n"
"    <property name=\"MinimumAvatarWidth type=\"u\" access=\"read\" />\n"
"    <property name=\"RecommendedAvatarHeight type=\"u\" access=\"read\" />\n"
"    <property name=\"RecommendedAvatarWidth type=\"u\" access=\"read\" />\n"
"    <property name=\"MaximumAvatarHeight type=\"u\" access=\"read\" />\n"
"    <property name=\"MaximumAvatarWidth type=\"u\" access=\"read\" />\n"
"    <property name=\"MaximumAvatarBytes type=\"u\" access=\"read\" />\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList SupportedAvatarMIMETypes READ SupportedAvatarMIMETypes)
    Q_PROPERTY(uint MinimumAvatarHeight READ MinimumAvatarHeight)
    Q_PROPERTY(uint MinimumAvatarWidth READ MinimumAvatarWidth)
    Q_PROPERTY(uint RecommendedAvatarHeight READ RecommendedAvatarHeight)
    Q_PROPERTY(uint RecommendedAvatarWidth READ RecommendedAvatarWidth)
    Q_PROPERTY(uint MaximumAvatarHeight READ MaximumAvatarHeight)
    Q_PROPERTY(uint MaximumAvatarWidth READ MaximumAvatarWidth)
    Q_PROPERTY(uint MaximumAvatarBytes READ MaximumAvatarBytes)

public:
    ProtocolAvatarsAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent),
          introspectionCalled(0)
    {
        mMimeTypes << QLatin1String("image/png");
        mMinimumAvatarHeight = 16;
        mMinimumAvatarWidth = 16;
        mRecommendedAvatarHeight = 32;
        mRecommendedAvatarWidth = 32;
        mMaximumAvatarHeight = 64;
        mMaximumAvatarWidth = 64;
        mMaximumAvatarBytes = 4096;
    }

    virtual ~ProtocolAvatarsAdaptor()
    {
    }

    inline QVariantMap immutableProperties() const
    {
        QVariantMap ret;
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".SupportedAvatarMIMETypes"),
                   mMimeTypes);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarHeight"),
                   mMinimumAvatarHeight);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MinimumAvatarWidth"),
                   mMinimumAvatarWidth);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarHeight"),
                   mMaximumAvatarHeight);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarWidth"),
                   mMaximumAvatarWidth);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarHeight"),
                   mRecommendedAvatarHeight);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".RecommendedAvatarWidth"),
                   mRecommendedAvatarWidth);
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_AVATARS + QLatin1String(".MaximumAvatarBytes"),
                   mMaximumAvatarBytes);
        return ret;
    }

public: // Properties
    inline QStringList SupportedAvatarMIMETypes() const
    {
        // if we request all properties we are going to get here, so marking as
        // introspectionCalled;
        introspectionCalled++;
        return mMimeTypes;
    }

    inline uint MinimumAvatarHeight() const
    {
        return mMinimumAvatarHeight;
    }

    inline uint MinimumAvatarWidth() const
    {
        return mMinimumAvatarWidth;
    }

    inline uint RecommendedAvatarHeight() const
    {
        return mRecommendedAvatarHeight;
    }

    inline uint RecommendedAvatarWidth() const
    {
        return mRecommendedAvatarWidth;
    }

    inline uint MaximumAvatarHeight() const
    {
        return mMaximumAvatarHeight;
    }

    inline uint MaximumAvatarWidth() const
    {
        return mMaximumAvatarWidth;
    }

    inline uint MaximumAvatarBytes() const
    {
        return mMaximumAvatarBytes;
    }

    mutable int introspectionCalled;

private:
    QStringList mMimeTypes;
    uint mMinimumAvatarHeight;
    uint mMinimumAvatarWidth;
    uint mRecommendedAvatarHeight;
    uint mRecommendedAvatarWidth;
    uint mMaximumAvatarHeight;
    uint mMaximumAvatarWidth;
    uint mMaximumAvatarBytes;
};

class ProtocolPresenceAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.Protocol.Interface.Presence")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.Protocol.Interface.Presence\" >\n"
"    <property name=\"Statuses\" type=\"a{s(ubb)}\" access=\"read\" >\n"
"      <annotation name=\"com.trolltech.QtDBus.QtTypeName\" value=\"Tp::SimpleStatusSpecMap\" />\n"
"    </property>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(Tp::SimpleStatusSpecMap Statuses READ Statuses)

public:
    ProtocolPresenceAdaptor(QObject *parent)
        : QDBusAbstractAdaptor(parent),
          introspectionCalled(0)
    {
        SimpleStatusSpec spec;
        spec.type = ConnectionPresenceTypeAvailable;
        spec.maySetOnSelf = true;
        spec.canHaveMessage = false;
        mStatuses.insert(QLatin1String("available"), spec);
    }

    virtual ~ProtocolPresenceAdaptor()
    {
    }

    inline QVariantMap immutableProperties() const
    {
        QVariantMap ret;
        ret.insert(TP_QT_IFACE_PROTOCOL_INTERFACE_PRESENCE + QLatin1String(".Statuses"),
                   qVariantFromValue(mStatuses));
        return ret;
    }

public: // Properties
    inline SimpleStatusSpecMap Statuses() const
    {
        // if we request all properties we are going to get here, so marking as
        // introspectionCalled;
        introspectionCalled++;
        return mStatuses;
    }

    mutable int introspectionCalled;

private:
    SimpleStatusSpecMap mStatuses;
};

struct CMHelper
{
    CMHelper(const QString &cmName,
             bool withProtocolProps = false,
             bool withProtocolAddressingProps = false,
             bool withProtocolAvatarsProps = false,
             bool withProtocolPresenceProps = false)
    {
        QDBusConnection bus = QDBusConnection::sessionBus();

        QString cmBusNameBase = TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE;
        QString cmPathBase = TP_QT_CONNECTION_MANAGER_OBJECT_PATH_BASE;

        protocolObject = new QObject();
        protocolAdaptor = new ProtocolAdaptor(protocolObject);
        protocolAddressingAdaptor = new ProtocolAddressingAdaptor(protocolObject);
        protocolAvatarsAdaptor = new ProtocolAvatarsAdaptor(protocolObject);
        protocolPresenceAdaptor = new ProtocolPresenceAdaptor(protocolObject);
        QVERIFY(bus.registerService(cmBusNameBase + cmName));
        QVERIFY(bus.registerObject(cmPathBase + cmName + QLatin1String("/") + cmName,
                    protocolObject));

        ProtocolPropertiesMap protocols;
        QVariantMap immutableProperties;
        if (withProtocolProps) {
            immutableProperties.unite(protocolAdaptor->immutableProperties());
        }
        if (withProtocolAddressingProps) {
            immutableProperties.unite(protocolAddressingAdaptor->immutableProperties());
        }
        if (withProtocolAvatarsProps) {
            immutableProperties.unite(protocolAvatarsAdaptor->immutableProperties());
        }
        if (withProtocolPresenceProps) {
            immutableProperties.unite(protocolPresenceAdaptor->immutableProperties());
        }
        protocols.insert(cmName, immutableProperties);

        cmObject = new QObject();
        cmAdaptor = new ConnectionManagerAdaptor(protocols, cmObject);
        QVERIFY(bus.registerService(cmBusNameBase + cmName));
        QVERIFY(bus.registerObject(cmPathBase + cmName, cmObject));

        cm = ConnectionManager::create(bus, cmName);
    }

    ~CMHelper()
    {
        delete cmObject;
        delete protocolObject;
    }

    ConnectionManagerPtr cm;
    QObject *cmObject;
    QObject *protocolObject;
    ConnectionManagerAdaptor *cmAdaptor;
    ProtocolAdaptor *protocolAdaptor;
    ProtocolAddressingAdaptor *protocolAddressingAdaptor;
    ProtocolAvatarsAdaptor *protocolAvatarsAdaptor;
    ProtocolPresenceAdaptor *protocolPresenceAdaptor;
};

class TestCmProtocol : public Test
{
    Q_OBJECT

public:
    TestCmProtocol(QObject *parent = 0)
        : Test(parent),
          mCM(0)
    {
    }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testIntrospection();
    void testIntrospectionWithManager();
    void testIntrospectionWithProperties();
    void testIntrospectionWithSomeProperties();

    void cleanup();
    void cleanupTestCase();

private:
    void testIntrospectionWithAdaptorCommon(const ConnectionManagerPtr &cm);

    CMHelper *mCM;
};

void TestCmProtocol::initTestCase()
{
    initTestCaseImpl();
}

void TestCmProtocol::init()
{
    initImpl();
}

void TestCmProtocol::testIntrospection()
{
    mCM = new CMHelper(QLatin1String("protocolnomanager"), false);

    ConnectionManagerPtr cm = mCM->cm;

    QVERIFY(connect(cm->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(cm->isReady(), true);

    testIntrospectionWithAdaptorCommon(cm);

    QVERIFY(mCM->protocolAdaptor->introspectionCalled > 0);
    QVERIFY(mCM->protocolAddressingAdaptor->introspectionCalled > 0);
    QVERIFY(mCM->protocolAvatarsAdaptor->introspectionCalled > 0);
    QVERIFY(mCM->protocolPresenceAdaptor->introspectionCalled > 0);
}

void TestCmProtocol::testIntrospectionWithManager()
{
    mCM = new CMHelper(QLatin1String("protocol"), false);

    ConnectionManagerPtr cm = mCM->cm;

    QVERIFY(connect(cm->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(cm->isReady(), true);

    QCOMPARE(cm->interfaces(), QStringList());
    QCOMPARE(cm->supportedProtocols(), QStringList() << QLatin1String("protocol"));

    QVERIFY(cm->hasProtocol(QLatin1String("protocol")));
    QVERIFY(!cm->hasProtocol(QLatin1String("not-there")));

    ProtocolInfo info = cm->protocol(QLatin1String("protocol"));
    QVERIFY(info.isValid());

    QCOMPARE(info.cmName(), QLatin1String("protocol"));
    QCOMPARE(info.name(), QLatin1String("protocol"));
    QCOMPARE(info.parameters().size(), 1);
    ProtocolParameter param = info.parameters().at(0);
    QCOMPARE(param.name(), QLatin1String("account"));
    QCOMPARE(static_cast<uint>(param.type()), static_cast<uint>(QVariant::String));
    QCOMPARE(param.defaultValue().isNull(), true);
    QCOMPARE(param.dbusSignature().signature(), QLatin1String("s"));
    QCOMPARE(param.isRequired(), true);
    QCOMPARE(param.isRequiredForRegistration(), true); // though it can't register!
    QCOMPARE(param.isSecret(), false);
    QVERIFY(!info.canRegister());
    QVERIFY(!info.capabilities().isSpecificToContact());
    QVERIFY(!info.capabilities().textChatrooms());
    QVERIFY(info.capabilities().textChats());
    QCOMPARE(info.vcardField(), QLatin1String("x-telepathy-protocol"));
    QCOMPARE(info.englishName(), QLatin1String("Telepathy Protocol"));
    QCOMPARE(info.iconName(), QLatin1String("im-protocol"));

    QStringList addressableVCardFields = info.addressableVCardFields();
    QCOMPARE(addressableVCardFields, QStringList() << QLatin1String("x-protocol"));
    QStringList addressableUriSchemes = info.addressableUriSchemes();
    QCOMPARE(addressableUriSchemes, QStringList() << QLatin1String("protocol"));

    AvatarSpec avatarReqs = info.avatarRequirements();
    QStringList supportedMimeTypes = avatarReqs.supportedMimeTypes();
    QCOMPARE(supportedMimeTypes, QStringList() << QLatin1String("image/jpeg"));
    QCOMPARE(avatarReqs.minimumHeight(), (uint) 32);
    QCOMPARE(avatarReqs.maximumHeight(), (uint) 96);
    QCOMPARE(avatarReqs.recommendedHeight(), (uint) 64);
    QCOMPARE(avatarReqs.minimumWidth(), (uint) 32);
    QCOMPARE(avatarReqs.maximumWidth(), (uint) 96);
    QCOMPARE(avatarReqs.recommendedWidth(), (uint) 64);
    QCOMPARE(avatarReqs.maximumBytes(), (uint) 37748736);

    PresenceSpecList statuses = info.allowedPresenceStatuses();
    QCOMPARE(statuses.size(), 2);
    PresenceSpec spec = getPresenceSpec(statuses, QLatin1String("available"));
    QVERIFY(spec.isValid());
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeAvailable);
    QVERIFY(spec.maySetOnSelf());
    QVERIFY(spec.canHaveStatusMessage());
    spec = getPresenceSpec(statuses, QLatin1String("offline"));
    QVERIFY(spec.isValid());
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeOffline);
    QVERIFY(!spec.maySetOnSelf());
    QVERIFY(!spec.canHaveStatusMessage());

    QCOMPARE(mCM->protocolAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolAddressingAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolAvatarsAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolPresenceAdaptor->introspectionCalled, 0);
}

void TestCmProtocol::testIntrospectionWithProperties()
{
    mCM = new CMHelper(QLatin1String("protocolwithprops"),
            true, true, true, true);

    ConnectionManagerPtr cm = mCM->cm;

    QVERIFY(connect(cm->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(cm->isReady(), true);

    testIntrospectionWithAdaptorCommon(cm);

    QCOMPARE(mCM->protocolAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolAddressingAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolAvatarsAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolPresenceAdaptor->introspectionCalled, 0);
}

void TestCmProtocol::testIntrospectionWithSomeProperties()
{
    mCM = new CMHelper(QLatin1String("protocolwithsomeprops"),
            false, false, true, true);

    ConnectionManagerPtr cm = mCM->cm;

    QVERIFY(connect(cm->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(cm->isReady(), true);

    testIntrospectionWithAdaptorCommon(cm);

    QVERIFY(mCM->protocolAdaptor->introspectionCalled > 0);
    QVERIFY(mCM->protocolAddressingAdaptor->introspectionCalled > 0);
    QCOMPARE(mCM->protocolAvatarsAdaptor->introspectionCalled, 0);
    QCOMPARE(mCM->protocolPresenceAdaptor->introspectionCalled, 0);
}

void TestCmProtocol::testIntrospectionWithAdaptorCommon(const ConnectionManagerPtr &cm)
{
    QCOMPARE(cm->interfaces(), QStringList());
    QCOMPARE(cm->supportedProtocols(), QStringList() << cm->name());

    QVERIFY(cm->hasProtocol(cm->name()));
    QVERIFY(!cm->hasProtocol(QLatin1String("not-there")));

    ProtocolInfo info = cm->protocol(cm->name());
    QVERIFY(info.isValid());

    QCOMPARE(info.cmName(), cm->name());
    QCOMPARE(info.name(), cm->name());

    QCOMPARE(info.parameters().size(), 0);
    QVERIFY(!info.canRegister());
    QVERIFY(!info.capabilities().isSpecificToContact());
    QVERIFY(info.capabilities().textChatrooms());
    QVERIFY(!info.capabilities().textChats());
    QCOMPARE(info.vcardField(), QLatin1String("x-adaptor"));
    QCOMPARE(info.englishName(), QLatin1String("Adaptor"));
    QCOMPARE(info.iconName(), QLatin1String("icon-adaptor"));

    QStringList addressableVCardFields = info.addressableVCardFields();
    QCOMPARE(addressableVCardFields, QStringList() << QLatin1String("x-adaptor"));
    QStringList addressableUriSchemes = info.addressableUriSchemes();
    QCOMPARE(addressableUriSchemes, QStringList() << QLatin1String("adaptor"));

    AvatarSpec avatarReqs = info.avatarRequirements();
    QStringList supportedMimeTypes = avatarReqs.supportedMimeTypes();
    QCOMPARE(supportedMimeTypes, QStringList() << QLatin1String("image/png"));
    QCOMPARE(avatarReqs.minimumHeight(), (uint) 16);
    QCOMPARE(avatarReqs.maximumHeight(), (uint) 64);
    QCOMPARE(avatarReqs.recommendedHeight(), (uint) 32);
    QCOMPARE(avatarReqs.minimumWidth(), (uint) 16);
    QCOMPARE(avatarReqs.maximumWidth(), (uint) 64);
    QCOMPARE(avatarReqs.recommendedWidth(), (uint) 32);
    QCOMPARE(avatarReqs.maximumBytes(), (uint) 4096);

    PresenceSpecList statuses = info.allowedPresenceStatuses();
    QCOMPARE(statuses.size(), 1);
    PresenceSpec spec = getPresenceSpec(statuses, QLatin1String("available"));
    QVERIFY(spec.isValid());
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeAvailable);
    QVERIFY(spec.maySetOnSelf());
    QVERIFY(!spec.canHaveStatusMessage());
    spec = getPresenceSpec(statuses, QLatin1String("offline"));
    QVERIFY(!spec.isValid());
}

void TestCmProtocol::cleanup()
{
    cleanupImpl();
}

void TestCmProtocol::cleanupTestCase()
{
    delete mCM;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestCmProtocol)
#include "_gen/cm-protocol.cpp.moc.hpp"
