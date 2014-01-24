#include <QtTest/QtTest>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/CapabilitiesBase>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/Types>

#include <TelepathyQt/test-backdoors.h>

using namespace Tp;

class TestCapabilities : public QObject
{
    Q_OBJECT

public:
    TestCapabilities(QObject *parent = 0);

private Q_SLOTS:
    void testConnCapabilities();
    void testContactCapabilities();
};

TestCapabilities::TestCapabilities(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

void TestCapabilities::testConnCapabilities()
{
    ConnectionCapabilities connCaps;
    // capabilities base
    QVERIFY(!connCaps.isSpecificToContact());
    QVERIFY(!connCaps.textChats());
    QVERIFY(!connCaps.audioCalls());
    QVERIFY(!connCaps.videoCalls());
    QVERIFY(!connCaps.videoCallsWithAudio());
    QVERIFY(!connCaps.upgradingCalls());
    QVERIFY(!connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.streamTubes());

    RequestableChannelClassSpecList rccSpecs;
    rccSpecs.append(RequestableChannelClassSpec::textChat());
    rccSpecs.append(RequestableChannelClassSpec::audioCall());
    rccSpecs.append(RequestableChannelClassSpec::videoCall());
    rccSpecs.append(RequestableChannelClassSpec::audioCallWithVideoAllowed());
    rccSpecs.append(RequestableChannelClassSpec::videoCallWithAudioAllowed());
    rccSpecs.append(RequestableChannelClassSpec::fileTransfer());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(!connCaps.isSpecificToContact());
    QVERIFY(connCaps.textChats());
    QVERIFY(connCaps.audioCalls());
    QVERIFY(connCaps.videoCalls());
    QVERIFY(connCaps.videoCallsWithAudio());
    QVERIFY(connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.streamTubes());
    QCOMPARE(connCaps.allClassSpecs(), rccSpecs);

    rccSpecs.append(RequestableChannelClassSpec::textChatroom());

    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChat());
    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatWithInvitees());
    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatroom());
    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatroomWithInvitees());
    rccSpecs.append(RequestableChannelClassSpec::contactSearch());
    rccSpecs.append(RequestableChannelClassSpec::contactSearchWithSpecificServer());
    rccSpecs.append(RequestableChannelClassSpec::contactSearchWithLimit());
    rccSpecs.append(RequestableChannelClassSpec::contactSearchWithSpecificServerAndLimit());
    rccSpecs.append(RequestableChannelClassSpec::streamTube());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(!connCaps.isSpecificToContact());
    QVERIFY(connCaps.textChats());
    QVERIFY(connCaps.audioCalls());
    QVERIFY(connCaps.videoCalls());
    QVERIFY(connCaps.videoCallsWithAudio());
    QVERIFY(connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(connCaps.conferenceTextChatrooms());
    QVERIFY(connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(connCaps.contactSearches());
    QVERIFY(connCaps.contactSearchesWithSpecificServer());
    QVERIFY(connCaps.contactSearchesWithLimit());
    QCOMPARE(connCaps.allClassSpecs(), rccSpecs);

    // start over
    rccSpecs.clear();
    rccSpecs.append(RequestableChannelClassSpec::audioCall());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.audioCalls());
    QVERIFY(!connCaps.videoCalls());
    QVERIFY(!connCaps.videoCallsWithAudio());

    rccSpecs.append(RequestableChannelClassSpec::videoCall());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.audioCalls());
    QVERIFY(connCaps.videoCalls());
    QVERIFY(!connCaps.videoCallsWithAudio());

    rccSpecs.append(RequestableChannelClassSpec::audioCallWithVideoAllowed());
    rccSpecs.append(RequestableChannelClassSpec::videoCallWithAudioAllowed());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.audioCalls());
    QVERIFY(connCaps.videoCalls());
    QVERIFY(connCaps.videoCallsWithAudio());

    // capabilities base
    QVERIFY(!connCaps.textChats());
    QVERIFY(!connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.streamTubes());

    rccSpecs.append(RequestableChannelClassSpec::textChat());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());

    rccSpecs.append(RequestableChannelClassSpec::textChatroom());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());

    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChat());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());

    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatWithInvitees());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());

    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatroom());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());

    rccSpecs.append(RequestableChannelClassSpec::conferenceTextChatroomWithInvitees());
    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(connCaps.textChats());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(connCaps.conferenceTextChatrooms());
    QVERIFY(connCaps.conferenceTextChatroomsWithInvitees());

    // capabilities base
    QVERIFY(!connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.streamTubes());
}

void TestCapabilities::testContactCapabilities()
{
    ContactCapabilities contactCaps;
    // capabilities base
    QVERIFY(!contactCaps.isSpecificToContact());
    QVERIFY(!contactCaps.textChats());
    QVERIFY(!contactCaps.audioCalls());
    QVERIFY(!contactCaps.videoCalls());
    QVERIFY(!contactCaps.videoCallsWithAudio());
    QVERIFY(!contactCaps.upgradingCalls());
    QVERIFY(!contactCaps.fileTransfers());
    // contact caps specific
    QVERIFY(!contactCaps.streamTubes(QLatin1String("foobar")));
    QVERIFY(!contactCaps.streamTubes(QLatin1String("service-foo")));
    QVERIFY(!contactCaps.streamTubes(QLatin1String("service-bar")));
    QVERIFY(contactCaps.streamTubeServices().isEmpty());

    RequestableChannelClassSpecList rccSpecs;
    rccSpecs.append(RequestableChannelClassSpec::textChat());
    rccSpecs.append(RequestableChannelClassSpec::audioCall());
    rccSpecs.append(RequestableChannelClassSpec::videoCall());
    rccSpecs.append(RequestableChannelClassSpec::audioCallWithVideoAllowed());
    rccSpecs.append(RequestableChannelClassSpec::videoCallWithAudioAllowed());
    rccSpecs.append(RequestableChannelClassSpec::fileTransfer());

    contactCaps = TestBackdoors::createContactCapabilities(rccSpecs, true);
    // capabilities base
    QVERIFY(contactCaps.isSpecificToContact());
    QVERIFY(contactCaps.textChats());
    QVERIFY(contactCaps.audioCalls());
    QVERIFY(contactCaps.videoCalls());
    QVERIFY(contactCaps.videoCallsWithAudio());
    QVERIFY(contactCaps.fileTransfers());
    // contact caps specific
    QVERIFY(!contactCaps.streamTubes(QLatin1String("foobar")));
    QVERIFY(!contactCaps.streamTubes(QLatin1String("service-foo")));
    QVERIFY(!contactCaps.streamTubes(QLatin1String("service-bar")));
    QVERIFY(contactCaps.streamTubeServices().isEmpty());
    QCOMPARE(contactCaps.allClassSpecs(), rccSpecs);

    rccSpecs.append(RequestableChannelClassSpec::streamTube(QLatin1String("service-foo")));
    rccSpecs.append(RequestableChannelClassSpec::streamTube(QLatin1String("service-bar")));

    contactCaps = TestBackdoors::createContactCapabilities(rccSpecs, true);
    // capabilities base
    QVERIFY(contactCaps.isSpecificToContact());
    QVERIFY(contactCaps.textChats());
    QVERIFY(contactCaps.audioCalls());
    QVERIFY(contactCaps.videoCalls());
    QVERIFY(contactCaps.videoCallsWithAudio());
    QVERIFY(contactCaps.fileTransfers());
    // contact caps specific
    QVERIFY(!contactCaps.streamTubes(QLatin1String("foobar")));
    QVERIFY(contactCaps.streamTubes(QLatin1String("service-foo")));
    QVERIFY(contactCaps.streamTubes(QLatin1String("service-bar")));
    QStringList stubeServices = contactCaps.streamTubeServices();
    stubeServices.sort();
    QStringList expectedSTubeServices;
    expectedSTubeServices << QLatin1String("service-foo") << QLatin1String("service-bar");
    expectedSTubeServices.sort();
    QCOMPARE(stubeServices, expectedSTubeServices);
    QCOMPARE(contactCaps.allClassSpecs(), rccSpecs);

    rccSpecs.clear();
    rccSpecs.append(RequestableChannelClassSpec::streamTube(QLatin1String("service-foo")));

    contactCaps = TestBackdoors::createContactCapabilities(rccSpecs, true);
    QVERIFY(!contactCaps.streamTubes(QLatin1String("foobar")));
    QVERIFY(contactCaps.streamTubes(QLatin1String("service-foo")));
    QVERIFY(!contactCaps.streamTubes(QLatin1String("service-bar")));
    QCOMPARE(contactCaps.streamTubeServices(), QStringList() << QLatin1String("service-foo"));

    rccSpecs.append(RequestableChannelClassSpec::streamTube(QLatin1String("service-bar")));

    contactCaps = TestBackdoors::createContactCapabilities(rccSpecs, true);
    QVERIFY(!contactCaps.streamTubes(QLatin1String("foobar")));
    QVERIFY(contactCaps.streamTubes(QLatin1String("service-foo")));
    QVERIFY(contactCaps.streamTubes(QLatin1String("service-bar")));
    stubeServices = contactCaps.streamTubeServices();
    stubeServices.sort();
    expectedSTubeServices.clear();
    expectedSTubeServices << QLatin1String("service-foo") << QLatin1String("service-bar");
    expectedSTubeServices.sort();
    QCOMPARE(stubeServices, expectedSTubeServices);
}

QTEST_MAIN(TestCapabilities)

#include "_gen/capabilities.cpp.moc.hpp"
