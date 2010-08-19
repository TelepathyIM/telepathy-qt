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
}

QTEST_MAIN(TestProfile)

#include "_gen/profile.cpp.moc.hpp"
