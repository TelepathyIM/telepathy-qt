#include <QtTest/QtTest>

#include <TelepathyQt4/Profile>

using namespace Tp;

class TestProfile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProfile();
};

void TestProfile::testProfile()
{
    ProfilePtr profile = Profile::create(QLatin1String("test-profile"));

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
    QCOMPARE(profile->presences().count(), 4);

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
}

QTEST_MAIN(TestProfile)

#include "_gen/profile.cpp.moc.hpp"
