#include <QtTest/QtTest>

#include <TelepathyQt4/ProfileManager>

using namespace Tp;

class TestProfileManager : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProfileManager();
};

void TestProfileManager::testProfileManager()
{
    ProfileManagerPtr pm = ProfileManager::create();
    QCOMPARE(pm->profiles().isEmpty(), false);
    QCOMPARE(pm->profiles().count(), 1);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile")).isNull(), false);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile-file-not-found")).isNull(), true);
    QCOMPARE(pm->profileForService(QLatin1String("test-profile-non-im-type")).isNull(), true);
    QCOMPARE(pm->profilesForCM(QLatin1String("testprofilecm")).isEmpty(), false);
    QCOMPARE(pm->profilesForCM(QLatin1String("testprofilecm")).count(), 1);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("testprofileproto")).isEmpty(), false);
    QCOMPARE(pm->profilesForProtocol(QLatin1String("testprofileproto")).count(), 1);
}

QTEST_MAIN(TestProfileManager)

#include "_gen/profile-manager.cpp.moc.hpp"
