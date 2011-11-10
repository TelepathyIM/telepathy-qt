#include <QtTest/QtTest>

#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ProfileManager>

#include <tests/lib/test.h>

using namespace Tp;

class TestProfileManager : public Test
{
    Q_OBJECT

private Q_SLOTS:
    void testProfileManager();
};

void TestProfileManager::testProfileManager()
{
    ProfileManagerPtr pm = ProfileManager::create(QDBusConnection::sessionBus());
    QVERIFY(connect(pm->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pm->isReady(), true);

    QCOMPARE(pm->profiles().isEmpty(), false);
    QCOMPARE(pm->profiles().count(), 2);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile")).isNull(), false);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile-file-not-found")).isNull(), true);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile-non-im-type")).isNull(), true);
    QCOMPARE(pm->profilesForCM(QLatin1String("testprofilecm")).isEmpty(), false);
    QCOMPARE(pm->profilesForCM(QLatin1String("testprofilecm")).count(), 2);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("testprofileproto")).isEmpty(), false);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("testprofileproto")).count(), 2);

    QVERIFY(connect(pm->becomeReady(ProfileManager::FeatureFakeProfiles),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(pm->isReady(ProfileManager::FeatureFakeProfiles), true);

    QCOMPARE(pm->profiles().isEmpty(), false);
    QCOMPARE(pm->profiles().count(), 4);
    QCOMPARE(pm->profileForService(QLatin1String("spurious-normal")).isNull(), false);
    QCOMPARE(pm->profileForService(QLatin1String("spurious-weird")).isNull(), false);
    QCOMPARE(pm->profilesForCM(QLatin1String("spurious")).isEmpty(), false);
    QCOMPARE(pm->profilesForCM(QLatin1String("spurious")).count(), 2);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("normal")).isEmpty(), false);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("weird")).isEmpty(), false);

    ProfilePtr profile = pm->profileForService(QLatin1String("spurious-normal"));
    QCOMPARE(profile->type(), QLatin1String("IM"));

    QCOMPARE(profile->provider(), QString());
    QCOMPARE(profile->name(), QLatin1String("normal"));
    QCOMPARE(profile->cmName(), QLatin1String("spurious"));
    QCOMPARE(profile->protocolName(), QLatin1String("normal"));
    QCOMPARE(profile->presences().isEmpty(), true);

    QCOMPARE(profile->parameters().isEmpty(), false);
    QCOMPARE(profile->parameters().count(), 1);

    QCOMPARE(profile->hasParameter(QLatin1String("not-found")), false);

    /* profile will only return CM default params, account is ignored */
    QCOMPARE(profile->hasParameter(QLatin1String("account")), false);

    /* profile will only return CM default params, password is ignored */
    QCOMPARE(profile->hasParameter(QLatin1String("password")), false);

    Profile::Parameter param = profile->parameter(QLatin1String("register"));
    QCOMPARE(param.name(), QLatin1String("register"));
    QCOMPARE(param.dbusSignature(), QDBusSignature(QLatin1String("b")));
    QCOMPARE(param.type(), QVariant::Bool);
    QCOMPARE(param.value(), QVariant(true));
    QCOMPARE(param.label(), QString());
    QCOMPARE(param.isMandatory(), false);

    // Allow the PendingReadys to delete themselves
    mLoop->processEvents();
}

QTEST_MAIN(TestProfileManager)

#include "_gen/profile-manager.cpp.moc.hpp"
