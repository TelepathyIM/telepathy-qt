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

QTEST_MAIN(TestPresence)

#include "_gen/presence.cpp.moc.hpp"
