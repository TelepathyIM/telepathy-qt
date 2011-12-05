#include <QtTest/QtTest>

#include <TelepathyQt/Debug>
#include <TelepathyQt/Profile>

using namespace Tp;

class TestProfile : public QObject
{
    Q_OBJECT

public:
    TestProfile(QObject *parent = 0);

private Q_SLOTS:
    void testProfile();
};

TestProfile::TestProfile(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

void TestProfile::testProfile()
{
    QString top_srcdir = QString::fromLocal8Bit(::getenv("abs_top_srcdir"));
    if (!top_srcdir.isEmpty()) {
        QDir::setCurrent(top_srcdir + QLatin1String("/tests"));
    }

    ProfilePtr profile = Profile::createForServiceName(QLatin1String("test-profile-file-not-found"));
    QCOMPARE(profile->isValid(), false);

    profile = Profile::createForServiceName(QLatin1String("test-profile-malformed"));
    QCOMPARE(profile->isValid(), false);

    profile = Profile::createForServiceName(QLatin1String("test-profile-invalid-service-id"));
    QCOMPARE(profile->isValid(), false);

    profile = Profile::createForServiceName(QLatin1String("test-profile-non-im-type"));
    QCOMPARE(profile->isValid(), false);

    profile = Profile::createForFileName(QLatin1String("telepathy/profiles/test-profile-non-im-type.profile"));
    QCOMPARE(profile->isValid(), true);

    profile = Profile::createForServiceName(QLatin1String("test-profile"));
    QCOMPARE(profile->isValid(), true);

    QCOMPARE(profile->serviceName(), QLatin1String("test-profile"));
    QCOMPARE(profile->type(), QLatin1String("IM"));
    QCOMPARE(profile->provider(), QLatin1String("TestProfileProvider"));
    QCOMPARE(profile->name(), QLatin1String("TestProfile"));
    QCOMPARE(profile->cmName(), QLatin1String("testprofilecm"));
    QCOMPARE(profile->protocolName(), QLatin1String("testprofileproto"));

    QCOMPARE(profile->parameters().isEmpty(), false);
    QCOMPARE(profile->parameters().count(), 2);

    QCOMPARE(profile->hasParameter(QLatin1String("foo")), false);

    QCOMPARE(profile->hasParameter(QLatin1String("server")), true);
    Profile::Parameter param = profile->parameter(QLatin1String("server"));
    QCOMPARE(param.name(), QLatin1String("server"));
    QCOMPARE(param.dbusSignature(), QDBusSignature(QLatin1String("s")));
    QCOMPARE(param.type(), QVariant::String);
    QCOMPARE(param.value(), QVariant(QLatin1String("profile.com")));
    QCOMPARE(param.label(), QString());
    QCOMPARE(param.isMandatory(), true);

    QCOMPARE(profile->hasParameter(QLatin1String("port")), true);
    param = profile->parameter(QLatin1String("port"));
    QCOMPARE(param.name(), QLatin1String("port"));
    QCOMPARE(param.dbusSignature(), QDBusSignature(QLatin1String("u")));
    QCOMPARE(param.type(), QVariant::UInt);
    QCOMPARE(param.value(), QVariant(QLatin1String("1111")));
    QCOMPARE(param.label(), QString());
    QCOMPARE(param.isMandatory(), true);

    QCOMPARE(profile->presences().isEmpty(), false);
    QCOMPARE(profile->presences().count(), 5);

    QCOMPARE(profile->hasPresence(QLatin1String("foo")), false);

    QCOMPARE(profile->hasPresence(QLatin1String("available")), true);
    Profile::Presence presence = profile->presence(QLatin1String("available"));
    QCOMPARE(presence.id(), QLatin1String("available"));
    QCOMPARE(presence.label(), QLatin1String("Online"));
    QCOMPARE(presence.iconName(), QLatin1String("online"));
    QCOMPARE(presence.isDisabled(), false);

    QCOMPARE(profile->hasPresence(QLatin1String("offline")), true);
    presence = profile->presence(QLatin1String("offline"));
    QCOMPARE(presence.id(), QLatin1String("offline"));
    QCOMPARE(presence.label(), QLatin1String("Offline"));
    QCOMPARE(presence.iconName(), QString());
    QCOMPARE(presence.isDisabled(), false);

    QCOMPARE(profile->hasPresence(QLatin1String("away")), true);
    presence = profile->presence(QLatin1String("away"));
    QCOMPARE(presence.id(), QLatin1String("away"));
    QCOMPARE(presence.label(), QLatin1String("Gone"));
    QCOMPARE(presence.iconName(), QString());
    QCOMPARE(presence.isDisabled(), false);

    QCOMPARE(profile->hasPresence(QLatin1String("hidden")), true);
    presence = profile->presence(QLatin1String("hidden"));
    QCOMPARE(presence.id(), QLatin1String("hidden"));
    QCOMPARE(presence.label(), QString());
    QCOMPARE(presence.iconName(), QString());
    QCOMPARE(presence.isDisabled(), true);

    QCOMPARE(profile->unsupportedChannelClassSpecs().isEmpty(), false);
    QCOMPARE(profile->unsupportedChannelClassSpecs().count(), 2);

    RequestableChannelClassSpec rccSpec = profile->unsupportedChannelClassSpecs().first();
    QCOMPARE(rccSpec.hasTargetHandleType(), true);
    QCOMPARE(rccSpec.targetHandleType(), HandleTypeContact);
    QCOMPARE(rccSpec.channelType(), QLatin1String("org.freedesktop.Telepathy.Channel.Type.Text"));

    profile = Profile::createForServiceName(QLatin1String("test-profile-no-icon-and-provider"));
    QCOMPARE(profile->isValid(), true);

    QCOMPARE(profile->serviceName(), QLatin1String("test-profile-no-icon-and-provider"));
    QCOMPARE(profile->type(), QLatin1String("IM"));
    QCOMPARE(profile->provider().isEmpty(), true);
    QCOMPARE(profile->cmName(), QLatin1String("testprofilecm"));
    QCOMPARE(profile->protocolName(), QLatin1String("testprofileproto"));
    QCOMPARE(profile->iconName().isEmpty(), true);
}

QTEST_MAIN(TestProfile)

#include "_gen/profile.cpp.moc.hpp"
