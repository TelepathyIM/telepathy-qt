#include <QtTest/QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/Types>

using namespace Tp;

class TestPresence : public QObject
{
    Q_OBJECT

public:
    TestPresence(QObject *parent = 0);

private Q_SLOTS:
    void testPresence();
    void testPresenceSpec();
};

TestPresence::TestPresence(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

#define TEST_PRESENCE(pr, prValid, prType, prStatus, prStatusMessage) \
{ \
    QVERIFY(pr.isValid() == prValid); \
    QCOMPARE(pr.type(), prType); \
    QCOMPARE(pr.status(), prStatus); \
    QCOMPARE(pr.statusMessage(), prStatusMessage); \
}

void TestPresence::testPresence()
{
    Presence pr;
    TEST_PRESENCE(pr, false, ConnectionPresenceTypeUnknown, QString(), QString());
    pr.setStatus(ConnectionPresenceTypeAvailable, QLatin1String("available"),
            QLatin1String("I am available"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAvailable, QLatin1String("available"), QLatin1String("I am available"));

    pr = Presence::available();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAvailable, QLatin1String("available"), QString());
    pr = Presence::available(QLatin1String("I am available"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAvailable, QLatin1String("available"), QLatin1String("I am available"));

    pr = Presence::away();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAway, QLatin1String("away"), QString());
    pr = Presence::away(QLatin1String("I am away"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAway, QLatin1String("away"), QLatin1String("I am away"));

    pr = Presence::brb();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAway, QLatin1String("brb"), QString());
    pr = Presence::brb(QLatin1String("I am brb"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeAway, QLatin1String("brb"), QLatin1String("I am brb"));

    pr = Presence::busy();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeBusy, QLatin1String("busy"), QString());
    pr = Presence::busy(QLatin1String("I am busy"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeBusy, QLatin1String("busy"), QLatin1String("I am busy"));

    pr = Presence::xa();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeExtendedAway, QLatin1String("xa"), QString());
    pr = Presence::xa(QLatin1String("I am xa"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeExtendedAway, QLatin1String("xa"), QLatin1String("I am xa"));

    pr = Presence::hidden();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeHidden, QLatin1String("hidden"), QString());
    pr = Presence::hidden(QLatin1String("I am hidden"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeHidden, QLatin1String("hidden"), QLatin1String("I am hidden"));

    pr = Presence::offline();
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeOffline, QLatin1String("offline"), QString());
    pr = Presence::offline(QLatin1String("I am offline"));
    TEST_PRESENCE(pr, true, ConnectionPresenceTypeOffline, QLatin1String("offline"), QLatin1String("I am offline"));
}

#define TEST_PRESENCE_SPEC_FULL(specStatus, specType, specMaySetOnSelf, specCanHaveMessage) \
{ \
    SimpleStatusSpec bareSpec; \
    bareSpec.type = specType; \
    bareSpec.maySetOnSelf = specMaySetOnSelf; \
    bareSpec.canHaveMessage = specCanHaveMessage; \
\
    PresenceSpec spec(specStatus, bareSpec); \
    TEST_PRESENCE_SPEC(spec, true, specStatus, specType, specMaySetOnSelf, specCanHaveMessage); \
}

#define TEST_PRESENCE_SPEC(spec, specValid, specStatus, specType, specMaySetOnSelf, specCanHaveMessage) \
{ \
    QVERIFY(spec.isValid() == specValid); \
    if (specValid) { \
        QCOMPARE(spec.presence(), Presence(specType, specStatus, QString())); \
        TEST_PRESENCE(spec.presence(), true, specType, specStatus, QString()); \
        QCOMPARE(spec.presence(QLatin1String("test message")), Presence(specType, specStatus, QLatin1String("test message"))); \
        TEST_PRESENCE(spec.presence(QLatin1String("test message")), true, specType, specStatus, QLatin1String("test message")); \
    } else { \
        QVERIFY(!spec.presence().isValid()); \
    } \
    QCOMPARE(spec.maySetOnSelf(), specMaySetOnSelf); \
    QCOMPARE(spec.canHaveStatusMessage(), specCanHaveMessage); \
\
    if (specValid) { \
        SimpleStatusSpec bareSpec; \
        bareSpec.type = specType; \
        bareSpec.maySetOnSelf = specMaySetOnSelf; \
        bareSpec.canHaveMessage = specCanHaveMessage; \
        QCOMPARE(spec.bareSpec(), bareSpec); \
    } else { \
        QCOMPARE(spec.bareSpec(), SimpleStatusSpec()); \
    } \
}

void TestPresence::testPresenceSpec()
{
    PresenceSpec spec;
    TEST_PRESENCE_SPEC(spec, false, QString(), ConnectionPresenceTypeUnknown, false, false);

    TEST_PRESENCE_SPEC_FULL(QLatin1String("available"), ConnectionPresenceTypeAvailable, true, true);
    TEST_PRESENCE_SPEC_FULL(QLatin1String("brb"), ConnectionPresenceTypeAway, true, true);
    TEST_PRESENCE_SPEC_FULL(QLatin1String("away"), ConnectionPresenceTypeAway, true, true);
    TEST_PRESENCE_SPEC_FULL(QLatin1String("xa"), ConnectionPresenceTypeExtendedAway, false, false);
    TEST_PRESENCE_SPEC_FULL(QLatin1String("offline"), ConnectionPresenceTypeOffline, true, false);
}

QTEST_MAIN(TestPresence)

#include "_gen/presence.cpp.moc.hpp"
