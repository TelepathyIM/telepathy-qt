#include <QtTest/QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/CapabilitiesBase>
#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/Types>

#include <TelepathyQt4/test-backdoors.h>

using namespace Tp;

class TestCapabilities : public QObject
{
    Q_OBJECT

public:
    TestCapabilities(QObject *parent = 0);

private Q_SLOTS:
    void testConnCapabilities();
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
    QVERIFY(!connCaps.streamedMediaCalls());
    QVERIFY(!connCaps.streamedMediaAudioCalls());
    QVERIFY(!connCaps.streamedMediaVideoCalls());
    QVERIFY(!connCaps.streamedMediaVideoCallsWithAudio());
    QVERIFY(!connCaps.upgradingStreamedMediaCalls());
    QVERIFY(!connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceStreamedMediaCalls());
    QVERIFY(!connCaps.conferenceStreamedMediaCallsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.contactSearch());
    QVERIFY(!connCaps.contactSearchWithSpecificServer());
    QVERIFY(!connCaps.contactSearchWithLimit());
    QVERIFY(!connCaps.streamTubes());

    RequestableChannelClassSpecList rccSpecs;
    rccSpecs.append(RequestableChannelClassSpec::textChat());
    rccSpecs.append(RequestableChannelClassSpec::streamedMediaCall());
    rccSpecs.append(RequestableChannelClassSpec::streamedMediaAudioCall());
    rccSpecs.append(RequestableChannelClassSpec::streamedMediaVideoCall());
    rccSpecs.append(RequestableChannelClassSpec::streamedMediaVideoCallWithAudio());
    rccSpecs.append(RequestableChannelClassSpec::fileTransfer());

    connCaps = TestBackdoors::createConnectionCapabilities(rccSpecs);
    // capabilities base
    QVERIFY(!connCaps.isSpecificToContact());
    QVERIFY(connCaps.textChats());
    QVERIFY(connCaps.streamedMediaCalls());
    QVERIFY(connCaps.streamedMediaAudioCalls());
    QVERIFY(connCaps.streamedMediaVideoCalls());
    QVERIFY(connCaps.streamedMediaVideoCallsWithAudio());
    QVERIFY(connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(!connCaps.textChatrooms());
    QVERIFY(!connCaps.conferenceStreamedMediaCalls());
    QVERIFY(!connCaps.conferenceStreamedMediaCallsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChats());
    QVERIFY(!connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(!connCaps.conferenceTextChatrooms());
    QVERIFY(!connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(!connCaps.contactSearches());
    QVERIFY(!connCaps.contactSearchesWithSpecificServer());
    QVERIFY(!connCaps.contactSearchesWithLimit());
    QVERIFY(!connCaps.contactSearch());
    QVERIFY(!connCaps.contactSearchWithSpecificServer());
    QVERIFY(!connCaps.contactSearchWithLimit());
    QVERIFY(!connCaps.streamTubes());
    QCOMPARE(connCaps.allClassSpecs(), rccSpecs);

    rccSpecs.append(RequestableChannelClassSpec::textChatroom());
    rccSpecs.append(RequestableChannelClassSpec::conferenceStreamedMediaCall());
    rccSpecs.append(RequestableChannelClassSpec::conferenceStreamedMediaCallWithInvitees());
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
    QVERIFY(connCaps.streamedMediaCalls());
    QVERIFY(connCaps.streamedMediaAudioCalls());
    QVERIFY(connCaps.streamedMediaVideoCalls());
    QVERIFY(connCaps.streamedMediaVideoCallsWithAudio());
    QVERIFY(connCaps.fileTransfers());
    // conn caps specific
    QVERIFY(connCaps.textChatrooms());
    QVERIFY(connCaps.conferenceStreamedMediaCalls());
    QVERIFY(connCaps.conferenceStreamedMediaCallsWithInvitees());
    QVERIFY(connCaps.conferenceTextChats());
    QVERIFY(connCaps.conferenceTextChatsWithInvitees());
    QVERIFY(connCaps.conferenceTextChatrooms());
    QVERIFY(connCaps.conferenceTextChatroomsWithInvitees());
    QVERIFY(connCaps.contactSearches());
    QVERIFY(connCaps.contactSearchesWithSpecificServer());
    QVERIFY(connCaps.contactSearchesWithLimit());
    QVERIFY(connCaps.contactSearch());
    QVERIFY(connCaps.contactSearchWithSpecificServer());
    QVERIFY(connCaps.contactSearchWithLimit());
    QCOMPARE(connCaps.allClassSpecs(), rccSpecs);
}

QTEST_MAIN(TestCapabilities)

#include "_gen/capabilities.cpp.moc.hpp"
