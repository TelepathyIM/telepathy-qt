#include <QtTest/QtTest>

namespace TpPrototype
{
    class Contact;
}

/**
 * @defgroup qt_tests Unit Tests
 * @ingroup qt_style_api
 * Classes that contains test functions.
 */


/**
 * Unit Tests for the TpPrototype library.
 * The unit tests functions are testing low-level and high level features of the API.
 * Please consider the precondition information for each function.<br>
 * <b>Precondition:</b> The tests currently just work in the Basyskom virtual machine development environment.
 * @ingroup qt_tests
 */
class UnitTests: public QObject
 {
     Q_OBJECT

#ifndef GENERATING_DOXYGEN_OUTPUT_
private slots:
#else
public:
#endif
    /**
     * Startup initialization.
     */
    void initTestCase();
     
    /**
     * Testing low level Qt bindings to mission control. This is just a simple round tripping test
     */
    void testMissionControlBindings();

    /** Connects and disconnects to jabber server. Uses and testing of Qt bindings for Tp */
    void testConnectToJabberServer();

    /** Tests whether the existing connection managers can be discovered. */
    void testRequestingOfConnectionManagers();

    /**
     * Creates an account.
     * This function tests the create account functions of the AccountManager.
     */
    void testAccountManager_createAccount();

   /**
    * Checks whether the created accounts can be found.
    * This function tests the list account functions of the AccountManager.
    * <b>Precondition:</b> testAccountManager_createAccount() was executed
    */
    void testAccountManager_listAccount();

   /**
    * Request all properties from the accounts.
    * The reading and writing of properties is tested on the AccountManager.<br>
    *  <b>Precondition:</b> testAccountManager_createAccount() was executed
    */
    void testAccountManager_showProperties();

   /**
    * Removes the accounts created by testRequestingOfConnectionManagers().
    * Tests the remove account feature of the AccountManager
    * <b>Precondition:</b> testAccountManager_createAccount() was executed
    */
    void testAccountManager_removeAccount();

   /**
    * General tests of the AccountManager.
    */
    void testPrototypeAccountManager();

   /**
    * Tests contact handling.
    * This function tests the ContactManager
    */
    void testPrototype_ContactHandling();

   /**
    * Test changing own presence.
    * This function tests the PresenceManager
    */
    void testPrototype_OwnPresenceChanged();

    /**
     * Testing chat functions.
     * Tests whether there is an account with contacts ( if not both are created ),
     * can send a textmessage to contact
     */
    void testTextChatFunction();
    
    /**
     * Test connecting, disconnecting and reconnect with Connection.
     */
    void testReconnect();

    /**
     * Test capability manager.
     * General tests for setting and obtaining capabilities.
     */
    void testCapabilityManager();
 
   /**
    * Test avatar manager.
    * Avatars are set for the local account and requested for contacts. 
    * <b>Precondition:</b> The first contact has to provide an avatar. Otherwise no signal will be emitted!
    */
    void testAvatarManager();

    /**
     * Test StreamedMedia handling: Receiving.
     * <b>Info:</b> This test currently expects a call from a known contact.<br>
     * <b>Precondition:</b> The first account must support VoIP to be called.<br>
     * This test requires interaction! In order to check whether accepting and rejecting works, the contact has to call twice.
     */
    void testStreamedMedia_receiveCall();

    /**
     * Test StreamedMedia handling: Outgoing call.
     * <b>Info:</b> This test currently expects the first known contact taking the call.<br>
     * <b>Precondition:</b> The first account must support VoIP to be called. And the first contact should receive the call<br>
     * This test requires interaction! In order to check whether accepting and rejecting works, the contact will be called twice.
     */
    void testStreamedMedia_outgoingCall();

    /**
     * Simple test to check whether the signalling works as expected.
     * <b>Precondition:</b> Currently this test will just work with Gabble and GoogleTalk<br>
     */
    void testBlockingSupport();

    /** @internal Helper slot to get a valid contact pointer */
    void slotReceiveContactPointer( TpPrototype::Contact* pointer );


private:
    QPointer<TpPrototype::Contact> m_pContactPointer;
 };

