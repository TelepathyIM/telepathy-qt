#include "prototype.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusReply>

#include <QtGui/QPixmap>

#include <QtTest/QSignalSpy>

#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Types>

#include <TelepathyQt4/Prototype/Account.h>
#include <TelepathyQt4/Prototype/AccountManager.h>
#include <TelepathyQt4/Prototype/AvatarManager.h>
#include <TelepathyQt4/Prototype/CapabilitiesManager.h>
#include <TelepathyQt4/Prototype/ChatChannel.h>
#include <TelepathyQt4/Prototype/Connection.h>
#include <TelepathyQt4/Prototype/ConnectionFacade.h>
#include <TelepathyQt4/Prototype/Contact.h>
#include <TelepathyQt4/Prototype/ContactManager.h>
#include <TelepathyQt4/Prototype/PresenceManager.h>
#include <TelepathyQt4/Prototype/StreamedMediaChannel.h>

// #define ENABLE_DEBUG_OUTPUT_
// #define DO_INTERACTIVE_TESTS_ // Needs user interaction

// TODO: Add cleanup that removes all accounts. Start own DBUS for testing to have a real clean starting point

namespace
{
    const char* g_displayName     = "DisplayName";
    const char* g_newResourceName = "New Resource Name";

    /** Waits for count signals. Returns true if signals were received. False on timeout */
    bool waitForSignal( const QSignalSpy* spy, int count = 1 )
    {
        const int max_loop = 10;
        int loop_count = 0;
        while ( ( loop_count < max_loop )
                && ( count != spy->count() ) )
        {
            ++loop_count;
            QTest::qWait( 1000 );
        }
        return ( loop_count < max_loop );
    }

    /** */
    bool compareType( int pos, QList<QVariant>& paramList, const QVariant compareType )
    {
        if ( !( paramList.at(pos).isValid() & compareType.isValid() ) )
        { return false; }
        
        if ( !paramList.at(pos).type() == compareType.type() )
        { return false; }

        else if (! (paramList.at(pos) == compareType) )
        { return false; }
        else
        { return true; }
    }

    /** Workaround for varying strictness of object path <-> string conversion
     * in different Qt 4.5 snapshots */
    QStringList objectPathListToStringList(Telepathy::ObjectPathList list)
    {
        QStringList ret;
        foreach (QDBusObjectPath p, list)
        {
            ret << p.path();
        }
        return ret;
    }
}

void UnitTests::initTestCase()
{
}


void UnitTests::testMissionControlBindings()
{
    QSKIP( "The interface NMC4Interface is not included and therefore cannot be tested!", SkipAll );
#if 0 // The interface is currently not created. We don't use it anywhere..
    Telepathy::Client::NMC4Interface mission_control( "org.freedesktop.Telepathy.MissionControl", "/org/freedesktop/Telepathy/MissionControl", this );

    QDBusPendingReply<uint> reply = mission_control.GetPresenceActual();
    reply.waitForFinished();

    QVERIFY2( reply.isFinished(),
              "Reply from GetPresenceActual() is not finished but should be.." );
    
    QDBusError error = reply.error();
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "GetPresenceActual: error type:" << error.type()
             << "GetPresenceActual: error name:" << error.name();
#endif
 
    QVERIFY2( reply.isValid(),
              "Received invalid reply to GetPresenceActual()." );

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "GetPresenceActual: Return: " << reply.value();
#endif
    
    QDBusPendingReply<QString, QDBusObjectPath> reply2 = mission_control.GetConnection( "blah" );
    reply2.waitForFinished();
    
    QVERIFY2( reply2.isFinished(),
              "Reply from GetConnection() is not finished but should be.." );
    
    error = reply2.error();
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "GetConnection: error type:" << error.type()
             << "GetConnection: error name:" << error.name();

    qDebug() << "GetPresenceActual: Value : " << reply2.value();
#endif
    
    QVERIFY( reply2.value() == QString( "No such account blah" ) );

#endif
}

void UnitTests::testConnectToJabberServer()
{
    Telepathy::registerTypes();
    // 1. Connect to connection manager
    Telepathy::Client::ConnectionManagerInterface cm_interface( "org.freedesktop.Telepathy.ConnectionManager.gabble",
                                                                "/org/freedesktop/Telepathy/ConnectionManager/gabble",
                                                                this );
                                             

    QVariantMap parameter_map;
    parameter_map.insert( "account", "basyskom@localhost" );
    parameter_map.insert( "password", "basyskom" );
    parameter_map.insert( "server", "localhost" );
    parameter_map.insert( "resource", "Telepathy" );
    parameter_map.insert( "port", static_cast<uint>(5222) );
    
    // 2. Request a connection to the Jabber server
    QDBusPendingReply<QString, QDBusObjectPath> reply = cm_interface.RequestConnection( "jabber",
                                                                                        parameter_map );
                                                                                    
    reply.waitForFinished();
    
    if ( !reply.isValid() )
    {
        QDBusError error = reply.error();
    
        qDebug() << "RequestConnection: error type:" << error.type()
                 << "RequestConnection: error name:" << error.name();
    }
    
    QVERIFY2( reply.isValid(),
              "Received invalid reply to CreateAccount()." );

    QString connection_service_name = reply.argumentAt<0>();
    QDBusObjectPath connection_object_path = reply.argumentAt<1>();
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Connection service name: " << connection_service_name;
    qDebug() << "Connection object path : " << connection_object_path.path();
#endif
    
    Telepathy::Client::ConnectionInterface connection_interface( connection_service_name,
                                                                 connection_object_path.path(),
                                                                 this );

    // :SX
    QDBusPendingReply<> connection_connect_reply = connection_interface.Connect();
    connection_connect_reply.waitForFinished();
    
    if ( !connection_connect_reply.isValid() )
    {
        QDBusError error = connection_connect_reply.error();
    
        qDebug() << "Connect: error type:" << error.type()
                 << "Connect: error name:" << error.name();
    }
    
    QVERIFY2( connection_connect_reply.isValid(),
              "Received invalid reply to Connect()." );
                                              
    QTest::qWait( 1000 );

    QDBusPendingReply<Telepathy::ChannelInfoList> channel_info_list_reply = connection_interface.ListChannels();
    channel_info_list_reply.waitForFinished();
    
    if ( !channel_info_list_reply.isValid() )
    {
        QDBusError error = channel_info_list_reply.error();
    
        qDebug() << "ListChannels: error type:" << error.type()
                 << "ListChannels: error name:" << error.name();
    }
    
    QVERIFY2( channel_info_list_reply.isValid(),
              "Received invalid reply to ListChannels()." );

#if 0
    qDebug() << "Available channels:";
    Telepathy::ChannelInfoList channel_list = channel_info_list_reply.value();
    foreach( Telepathy::ChannelInfo channel, channel_list )
    {
        qDebug() << "Channel: " << channel.channel.path();
        qDebug() << "Type   : " << channel.channelType;
        qDebug() << "H. Type: " << channel.handleType;
        qDebug() << "Handle : " << channel.handle;
    }
#endif
    
    // x. Disconnect from jabber server
    QDBusPendingReply<> connection_reply = connection_interface.Disconnect();
    
    connection_reply.waitForFinished();
    
    if ( !connection_reply.isValid() )
    {
        QDBusError error = connection_reply.error();
    
        qDebug() << "Disconnect: error type:" << error.type()
                 << "Disconnect: error name:" << error.name();
    }
    
    QVERIFY2( connection_reply.isValid(),
              "Received invalid reply to CreateAccount()." );
    
}

// Precondition: gabble is installed
// This simple test just checks whether gabble is available.
void UnitTests::testRequestingOfConnectionManagers()
{
    QStringList cm_names = TpPrototype::ConnectionFacade::instance()->listOfConnectionManagers();
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Available CMs:" << cm_names;
#endif
    QVERIFY2( cm_names.count() != 0, "No connection managers registered on the bus!" );
    QVERIFY2( cm_names.contains( "gabble" ), "No gabble found!" );
}

void UnitTests::testAccountManager_createAccount()
{
    QVariantMap parameter_map;
    parameter_map.insert( "account", "basyskom@localhost" );
    parameter_map.insert( "password", "basyskom" );
    parameter_map.insert( "server", "localhost" );
    parameter_map.insert( "resource", "Telepathy" );
    parameter_map.insert( "port", static_cast<uint>(5222) );

    Telepathy::registerTypes();
    Telepathy::Client::AccountManagerInterface accountmanager_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                              "/org/freedesktop/Telepathy/AccountManager",
                                                                              this );
                                                      
    QSignalSpy spy_validity_changed( &accountmanager_interface, SIGNAL( AccountValidityChanged( const QDBusObjectPath&, bool ) ) );
    QCOMPARE( spy_validity_changed.isValid(), true );
                                                      
    QDBusPendingReply<QDBusObjectPath> create_reply = accountmanager_interface.CreateAccount( "gabble", "jabber", g_displayName, parameter_map );
    create_reply.waitForFinished();
        
    if ( !create_reply.isValid() )
    {
        QDBusError error = create_reply.error();
    
        qDebug() << "Disconnect: error type:" << error.type()
                 << "Disconnect: error name:" << error.name();
    }
    
    QVERIFY2( create_reply.isValid(),
              "Received invalid reply to CreateAccount()." );

    
    QTest::qWait( 2000 );
    QEXPECT_FAIL( "", "There is currently no signal emitted on AccountInterface::CreateAccount(). This needs to be analyzed further", Continue );
    QVERIFY2( spy_validity_changed.count() == 1, "CreateAccount does not emits the signal AccountValidityChanged!" );                                                 
}



// Precodition: testAcountManager_createAccount() was called to create accounts!
void UnitTests::testAccountManager_listAccount()
{
    Telepathy::Client::AccountManagerInterface accountmanager_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                              "/org/freedesktop/Telepathy/AccountManager", 
                                                                              this );
    Telepathy::registerTypes();
    QStringList object_path_list_valid = objectPathListToStringList(accountmanager_interface.ValidAccounts());
    QVERIFY2( object_path_list_valid.count() > 0, "No accounts found. Possible reason: testAcountManager_createAccount() was not called before!" );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Num of Accounts: " << object_path_list_valid.count();
    foreach( QString path, object_path_list_valid )
    {
        qDebug() << "Valid Accounts : " << path;
    }
#endif

#if 0 // Reenable this after a real cleanup of accounts is implemented. Otherwise this may fail without be an error!
    QStringList object_path_list_invalid = accountmanager_interface.InvalidAccounts();
    QVERIFY( object_path_list_invalid.count() == 0 );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Num of Invalid Accounts: " << object_path_list_invalid.count();
    foreach( QString path, object_path_list_invalid )
    {
        qDebug() << "Invalid Accounts : " << path;
    }
#endif // ENABLE_DEBUG_OUTPUT_
#endif // 0
}

void UnitTests::testAccountManager_showProperties()
{
    Telepathy::Client::AccountManagerInterface accountmanager_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                              "/org/freedesktop/Telepathy/AccountManager",
                                                                              this );
    Telepathy::registerTypes();
    QStringList object_path_list_valid = objectPathListToStringList(accountmanager_interface.ValidAccounts());
    QVERIFY2( object_path_list_valid.count() > 0, "No accounts found. Possible reason: testAcountManager_createAccount() was not called before!" );
    bool found_correct_display_name = false;
    foreach( QString path, object_path_list_valid )
    {
        Telepathy::Client::AccountInterface account_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                    path,
                                                                    this );
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "DisplayName     :" << account_interface.DisplayName();
        qDebug() << "Icon            :" << account_interface.Icon();
        qDebug() << "Account Valid   :" << account_interface.Valid();
        qDebug() << "Account Enabled :" << account_interface.Enabled();
        qDebug() << "Nickname        :" << account_interface.Nickname();
        qDebug() << "Parameters      :" << account_interface.Parameters();
        Telepathy::SimplePresence automatic_presence = account_interface.AutomaticPresence();
        qDebug() << "* Auto Presence type   :" << automatic_presence.type;
        qDebug() << "* Auto Presence status :" << automatic_presence.status;
        qDebug() << "Connection      :" << account_interface.Connection();
        qDebug() << "ConnectionStatus:" << account_interface.ConnectionStatus();
        qDebug() << "Connect. Reason :" << account_interface.ConnectionStatusReason();
        Telepathy::SimplePresence current_presence = account_interface.CurrentPresence();
        qDebug() << "* Current Presence type   :" << current_presence.type;
        qDebug() << "* Current Presence status :" << current_presence.status;
        qDebug() << "Auto Connect    :" << account_interface.ConnectAutomatically();
        Telepathy::SimplePresence requested_presence = account_interface.RequestedPresence();
        qDebug() << "* Requested Presence type   :" << requested_presence.type;
        qDebug() << "* Requested Presence status :" << requested_presence.status;
        qDebug() << "Normalized Name :" << account_interface.NormalizedName();
#endif
        if ( account_interface.DisplayName() == g_displayName )
            found_correct_display_name = true;
    }
    // Check whether the expected account was found
    QVERIFY( found_correct_display_name );

}

// Precondition: testAcountManager_createAccount() was called to create accounts!
void UnitTests::testAccountManager_removeAccount()
{
    {
        Telepathy::Client::AccountManagerInterface accountmanager_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                                  "/org/freedesktop/Telepathy/AccountManager",
                                                                                  this );
        Telepathy::registerTypes();
        QStringList object_path_list_valid = objectPathListToStringList(accountmanager_interface.ValidAccounts());
        QVERIFY2( object_path_list_valid.count() > 0, "No accounts found. Possible reason: testAcountManager_createAccount() was not called before!" );
        
        foreach( QString path, object_path_list_valid )
        {
            Telepathy::Client::AccountInterface account_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                        path,
                                                                        this );

#if 1 // Disable to remove all accounts
            // Ignore all accounts that were not created by us
            if ( account_interface.DisplayName() != g_displayName )
            { continue; }
#endif
            
            QSignalSpy spy_removed( &account_interface, SIGNAL( Removed() ) );
            QCOMPARE( spy_removed.isValid(), true );
        
            QDBusPendingReply<> remove_reply = account_interface.Remove();
            remove_reply.waitForFinished();
        
            if ( !remove_reply.isValid() )
            {
                QDBusError error = remove_reply.error();
            
                qDebug() << "Remove: error type:" << error.type()
                        << "Remove: error name:" << error.name();
            }
            
            QVERIFY2( remove_reply.isValid(),
                    "Received invalid reply to AccountInterface::Remove()." );

            QTest::qWait( 2000 );

            QEXPECT_FAIL( "", "There is currently no signal emitted on AccountInterface::Remove(). This needs to be analyzed further!", Continue );
            QVERIFY2( spy_removed.count() == 1, "RemoveAccount does not emits the signal Removed()!" );
        }
    }
    {
        // Check whether there are really no accounts left..
        Telepathy::Client::AccountManagerInterface accountmanager_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                                  "/org/freedesktop/Telepathy/AccountManager",
                                                                                  this );
        Telepathy::registerTypes();
        QStringList object_path_list_valid = objectPathListToStringList(accountmanager_interface.ValidAccounts());
        int accounts_left = 0;
        foreach( QString path, object_path_list_valid )
        {
            Telepathy::Client::AccountInterface account_interface( "org.freedesktop.Telepathy.AccountManager",
                                                                        path,
                                                                        this );
            if ( account_interface.DisplayName() != g_displayName )
            { continue; }
            ++accounts_left;
        }
        QCOMPARE( accounts_left, 0 );
    }
}

void UnitTests::testPrototypeAccountManager()
{
    QTest::qWait( 3000 );
    
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    QVariantMap parameter_map;
    parameter_map.insert( "account", "basyskom@localhost" );
    parameter_map.insert( "password", "basyskom" );
    parameter_map.insert( "server", "localhost" );
    parameter_map.insert( "resource", "Telepathy" );
    parameter_map.insert( "port", static_cast<uint>(5222) );
#if 1// Disable this temporarily if the accounts were not deleted properly
    if ( 0 != account_manager->count() )
    {
        QList<QPointer<TpPrototype::Account> > account_list = account_manager->accountList();
        foreach( TpPrototype::Account* account, account_list )
        {
            true == account->remove();
        }
    }
    QVERIFY( 0 == account_manager->count() );

    QSignalSpy spy_create_account( account_manager, SIGNAL( signalNewAccountAvailable( TpPrototype::Account* ) ) );
    QCOMPARE( spy_create_account.isValid(), true );

    QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "Ich 1", parameter_map ) );
    QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "Ich 2", parameter_map ) );
    QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "Ich 3", parameter_map ) );

    QVERIFY2( waitForSignal( &spy_create_account, 3 ), "Received no signals after createAccount() ");
#endif
    
    QVERIFY( 3 == account_manager->count() );

    QList<QPointer<TpPrototype::Account> > account_list = account_manager->accountList();
    QVERIFY( 3 == account_list.count() );

    //qDebug() << "Parameters: " << account_list.at(0)->parameters();
    //qDebug() << "Properties: " << account_list.at(0)->properties();

    QSignalSpy spy_update_account( account_manager, SIGNAL( signalAccountUpdated( TpPrototype::Account* ) ) );
    QCOMPARE( spy_update_account.isValid(), true );

    QVariant enabled_flag = account_list.at(0)->properties().value( "Enabled" );
    QCOMPARE( enabled_flag.toBool(), false );
    QVariantMap new_properties;
    new_properties.insert( "Enabled", true );
    account_list.at(0)->setProperties( new_properties );
    enabled_flag = account_list.at(0)->properties().value( "Enabled" );
    QCOMPARE( enabled_flag.toBool(), true );

    QVariantMap new_parameter;
    QVariantMap old_parameters = account_list.at(0)->parameters();
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Old Parameters: " << old_parameters;
#endif
    new_parameter.insert( "resource", g_newResourceName );
    QVERIFY( account_list.at(0)->setParameters( new_parameter ) );
    QVariantMap updated_parameters = account_list.at(0)->parameters();
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Updated Parameters: " << updated_parameters;
#endif
    QVERIFY( old_parameters != updated_parameters );
    QVERIFY( updated_parameters.value( "resource" ) == g_newResourceName );

    QVERIFY2( waitForSignal( &spy_update_account, 2 ), "Received no signals after removeAccount() ");
    
    QSignalSpy spy_remove_account( account_manager, SIGNAL( signalAccountRemoved( TpPrototype::Account* ) ) );
    QCOMPARE( spy_remove_account.isValid(), true );

    int count = 0;
    foreach( TpPrototype::Account* account, account_list )
    {
        ++count;
        QVERIFY( true == account->remove() );
    }

    QVERIFY2( waitForSignal( &spy_remove_account, count ), "Received no signals after removeAccount() ");
    
    QVERIFY( 0 == account_manager->count() );
}

void UnitTests::testPrototype_ContactHandling()
{

    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );
    // Create an account if there is none..
    if ( account_manager->accountList().count() < 2 )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest2", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account1 = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account1 );
    
    TpPrototype::Connection* connection = TpPrototype::ConnectionFacade::instance()->connectionWithAccount( account1, 1 );

    QVERIFY( connection );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Create First Connection";
#endif // ENABLE_DEBUG_OUTPUT_
    
    QSignalSpy spy_status_changed( connection, SIGNAL( signalStatusChanged( TpPrototype::Connection*, Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QCOMPARE( spy_status_changed.isValid(), true );
    QVERIFY( connection->requestConnect() == true );

    QVERIFY2( waitForSignal( &spy_status_changed, 2 ), "Received no signal after connectRequest ");
    
    TpPrototype::ContactManager* contact_manager  = connection->contactManager();
    QVERIFY( NULL != contact_manager );
    QVERIFY( contact_manager->isValid() );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Initialize First Contact Manager";
#endif// ENABLE_DEBUG_OUTPUT_
    
    // Make sure we got some contacts:
    QTest::qWait( 1000 );
    QList<QPointer<TpPrototype::Contact> > contacts = contact_manager->contactList();
    QVERIFY2(!contacts.isEmpty(), "No contacts found.");
    
    TpPrototype::Account* account2 = account_manager->accountList().at( 1 );
    QVERIFY( NULL != account2 );
    TpPrototype::Connection* connection2 = TpPrototype::ConnectionFacade::instance()->connectionWithAccount( account2, 2 );
    QVERIFY( connection2 );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Creation Second Connection";
#endif// ENABLE_DEBUG_OUTPUT_
    //spy_status_changed( connection2, SIGNAL( signalStatusChanged( TpPrototype::Connection*, Telepathy::ConnectionStatus, Telepathy::ConnectionStatus  ) );
    QCOMPARE( spy_status_changed.isValid(), true );
    QVERIFY( connection2->requestConnect() == true );

    // QVERIFY2( waitForSignal( &spy_status_changed, 2 ), "Received no signal after connectRequest ");
    QTest::qWait( 1000 );
    
    TpPrototype::ContactManager* contact_manager2  = connection2->contactManager();
    QVERIFY( NULL != contact_manager2 );
    QVERIFY( contact_manager2->isValid() );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Initialize Second Contact Manager";
#endif//def ENABLE_DEBUG_OUTPUT_
    // Make sure we got some contacts:
    QTest::qWait( 1000 );
    QList<QPointer<TpPrototype::Contact> > contacts2 = contact_manager2->contactList();
   // QVERIFY2(!contacts2.isEmpty(), " Account 2 No contacts found.");

#ifdef ENABLE_DEBUG_OUTPUT_
    foreach (QPointer<TpPrototype::Contact> ptr, contacts)
    { qDebug() << "Contact:" << ptr->name(); }
#endif
    
    contact_manager2->requestContact("basyskom@localhost");
    QTest::qWait( 2000 );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Contact Request by Account 2";
#endif// ENABLE_DEBUG_OUTPUT_
    
    contact_manager->requestContact("test@localhost");
    QTest::qWait( 2000 );

    contact_manager2->requestContact("basyskom@localhost");

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Contact Request by Account 1";
#endif// ENABLE_DEBUG_OUTPUT_
//    QVERIFY2( waitForSignal( &spy_contacts_added ), "Received no signal Remote Pending Contact");

    connection->requestDisconnect();
    delete connection;

    QTest::qWait( 1000 );
}

#define ENABLE_DEBUG_OUTPUT_
void UnitTests::testPrototype_OwnPresenceChanged()
{
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none..
    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
// verbinden mit erstem konto
    TpPrototype::Connection* connection = TpPrototype::ConnectionFacade::instance()->connectionWithAccount( account, 1 );
    QVERIFY( connection );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "testPrototypeContactHandling Create First Connection";
#endif

    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY( connection->requestConnect() == true );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");

    TpPrototype::PresenceManager* presence_manager = connection->presenceManager();
    QVERIFY2( NULL != presence_manager, "No presence information is supported!" );

    // The presence manager is invalid if no valid presence interface is available
    QVERIFY2( true == presence_manager->isValid(), "No compatible presence interface found!" );

    Telepathy::SimpleStatusSpecMap status_map = presence_manager->statuses();
    QVERIFY2( status_map.count() != 0, "No presence information returned!" );
    
    QStringList keys = status_map.keys();

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Possible Presence settings: ";
    foreach( QString key, keys )
    {
        qDebug() << "Name: " << key << "type: " << status_map.value( key ).type << "MaySetOnSelf: " << status_map.value( key ).type << "canHaveMessage:" << status_map.value( key ).canHaveMessage;
        
    }
#endif
    
    QVERIFY( keys.contains( "available" ) );
    QVERIFY( keys.contains( "away" ) );
    QVERIFY( keys.contains( "offline" ) );

    QSignalSpy spy_state_changed( presence_manager, SIGNAL( signalOwnPresenceUpdated( const TpPrototype::Account*, const Telepathy::SimplePresence& ) ) );
    QCOMPARE( spy_state_changed.isValid(), true );


    QStringList testDataPresenceChangeStatus;
    QStringList testDataPresenceChangeStatusMessage;
    testDataPresenceChangeStatus.append( "available" );
    testDataPresenceChangeStatus.append( "away" );
    testDataPresenceChangeStatus.append( "offline" );

    testDataPresenceChangeStatusMessage.append( "I am available" );
    testDataPresenceChangeStatusMessage.append( "I am away" );
    testDataPresenceChangeStatusMessage.append( "I am offline" );


    for ( int count = 0; count< testDataPresenceChangeStatus.count(); count++ )
    {
        
        presence_manager->setPresence( testDataPresenceChangeStatus.at( count ), testDataPresenceChangeStatusMessage.at(count) );

        QString verifyMessage = QString("Received no signal after changing presence to %1").arg(testDataPresenceChangeStatus.at(count) ) ;
        if( testDataPresenceChangeStatus.at(count) == QString("offline") )
        {
            QVERIFY2( waitForSignal( &spy_state_changed ), verifyMessage.toLatin1() );
        }
        else
        {
            // Test whether emmitted signal contains the expected data
            QVERIFY2( waitForSignal( &spy_state_changed ), verifyMessage.toLatin1() );

            QCOMPARE( false, spy_state_changed.isEmpty() );
            QList<QVariant> firstSignalEmited = spy_state_changed.takeFirst();

            QCOMPARE( firstSignalEmited.count(), 2 );

            Telepathy::SimplePresence emitedPresence = firstSignalEmited.at(1).value<Telepathy::SimplePresence>() ;
            QCOMPARE( emitedPresence.status , testDataPresenceChangeStatus.at(count) );
            QCOMPARE( emitedPresence.statusMessage , testDataPresenceChangeStatusMessage.at(count) );

            // Test whether "currentPresence()" returns the expected state as well.
            QCOMPARE( emitedPresence.status, presence_manager->currentPresence().status );
            QCOMPARE( emitedPresence.statusMessage, presence_manager->currentPresence().statusMessage );
        }
    }

    delete connection;
}
#undef ENABLE_DEBUG_OUTPUT_

void UnitTests::testTextChatFunction()
{
    // get the account Manager
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Stop if there are less than 2 accounts
    QVERIFY( account_manager->accountList().count() > 1 );

    TpPrototype::Account* acc_2 = account_manager->accountList().at( account_manager->accountList().count()-1 );
    TpPrototype::Account* acc_1 = account_manager->accountList().at( account_manager->accountList().count()-2 );

    QVERIFY( acc_2 );
    QVERIFY( acc_1 );

    qDebug() << "acc_1 = " << acc_1->parameters().value("account");
    qDebug() << "acc_2 = " << acc_2->parameters().value("account");
    
    // connect both accounts
    TpPrototype::Connection* conn_2 = acc_2->connection();
    QVERIFY( conn_2 );
    TpPrototype::Connection* conn_1 = acc_1->connection();
    QVERIFY( conn_1 );

    QSignalSpy spy_conn_1_status_changed( conn_1, SIGNAL( signalStatusChanged( TpPrototype::Connection*, Telepathy::ConnectionStatus, Telepathy::ConnectionStatus  ) ) );
    QCOMPARE( spy_conn_1_status_changed.isValid(), true );

    QTest::qWait( 1000 );
    QVERIFY( conn_1->requestConnect() == true );
    
    QSignalSpy spy_conn_2_status_changed( conn_2, SIGNAL( signalStatusChanged( TpPrototype::Connection*, Telepathy::ConnectionStatus, Telepathy::ConnectionStatus  ) ) );
    QCOMPARE( spy_conn_2_status_changed.isValid(), true );

    QTest::qWait( 1000 );
    QVERIFY( conn_2->requestConnect() == true );
    
    QVERIFY2( waitForSignal( &spy_conn_1_status_changed, 2 ), "Received no signal after connectRequest for conn_1 ");
    QVERIFY2( waitForSignal( &spy_conn_2_status_changed, 2 ), "Received no signal after connectRequest for conn_2 ");
    
    TpPrototype::ContactManager* contact_manager_1  = conn_1->contactManager();
    TpPrototype::ContactManager* contact_manager_2  = conn_2->contactManager();
    
    QVERIFY( NULL != contact_manager_1 );
    QVERIFY( contact_manager_1->isValid() );
    QVERIFY( NULL != contact_manager_2 );
    QVERIFY( contact_manager_2->isValid() );

    QVERIFY( contact_manager_1->requestContact( acc_2->parameters().value("account").toString() ) == true );
    QVERIFY( contact_manager_2->requestContact( acc_1->parameters().value("account").toString() ) == true );

    QList < QPointer<TpPrototype::Contact> > contact_list_1 = contact_manager_1->contactList();
    QVERIFY( contact_list_1.count() > 0 );
    
    TpPrototype::Contact* chat_contact = contact_list_1.at(0);

    foreach( TpPrototype::Contact* cont_insp , contact_list_1 )
    {
        if (cont_insp->name() == acc_2->parameters().value("account").toString() )
        {
             chat_contact = cont_insp;
        }
    }

    QList < QPointer<TpPrototype::Contact> > contact_list_2 = contact_manager_2->contactList();
    QVERIFY( contact_list_2.count() > 0 );
    
    TpPrototype::Contact* contact_chatty = contact_list_2.at(0);

    foreach( TpPrototype::Contact* cont_insp , contact_list_2 )
    {
        if (cont_insp->name() == acc_1->parameters().value("account").toString() )
        {
            contact_chatty = cont_insp;
        }
    }


    // Send message to account2 from account1
    QString message("get in touch with Telepathy QT4");

    QSignalSpy spy_message(contact_manager_2 , SIGNAL( signalTextChannelOpenedForContact( TpPrototype::Contact* contact ) ));

    chat_contact->chatChannel()->sendTextMessage( message );

    
    QSignalSpy spy_incoming_message( contact_chatty->chatChannel(), SIGNAL( signalTextMessageReceived( TpPrototype::ChatChannel* chatchannel, uint timestamp, uint type, uint flags, const QString& text ) ) );
                                     
    contact_chatty->chatChannel()->pendingTextMessages();

    QTest::qWait( 2000 );

    // Something's wrong here probably with QSignalSpy
    // the signals are emited but not catched by teh spies
    QVERIFY2( waitForSignal( &spy_incoming_message, 2 ), "Received no Signal from chatChannel() after checking for pendingMessages() ");
    
    QVERIFY2( waitForSignal( &spy_message, 2 ), "Received no Signal from contact_maanger_2 after sending a message from acc_1");

    QCOMPARE(spy_message.count(), 1);
    
    QList<QVariant> arguments = spy_message.takeFirst();

    qDebug() << arguments.at(0);

    
    
//    qRegisterMetaType<TpPrototype::Contact*>("TpPrototype::Contact*");
//    TpPrototype::Contact* receiving_contact = qvariant_cast<TpPrototype::Contact*>(arguments.at(0));

    // contact -> chatchannel ->

    // signal des chatchannel abfangen

    // chatchannel->pending()

    
    

    // prüfen ob empfangene nachricht - gesendetet enthält

    // umgekehrt auch testen

}

void UnitTests::testReconnect()
{
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none..
    if ( account_manager->accountList().isEmpty() )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "*** Create Account";
#endif
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map ) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "*** Account Created";
#endif

    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "*** Request connection object for account";
#endif
    TpPrototype::Connection* connection = TpPrototype::ConnectionFacade::instance()->connectionWithAccount( account, 1 );
    QVERIFY( connection );
    
    qDebug() << "testPrototypeContactHandling Create First Connection";
    QSignalSpy spy_status_changed( connection, SIGNAL( signalStatusChanged( TpPrototype::Connection*, Telepathy::ConnectionStatus, Telepathy::ConnectionStatus  ) ) );
    QCOMPARE( spy_status_changed.isValid(), true );

    QTest::qWait( 1000 );
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "*** Request connect for the first time";
#endif
    QVERIFY( connection->requestConnect() == true );

    QTest::qWait( 1000 );

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "*** Request _dis_connect";
#endif
    QVERIFY( connection->requestDisconnect() );
    
    QTest::qWait( 1000 );
        // Now go back online..
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "*** Request connect for the _second_ time";
#endif
    QVERIFY( connection->requestConnect() == true );
    QTest::qWait( 1000 );
    
    QVERIFY( connection->presenceManager()->setPresence( "available", "Back online" ) == true );

    delete connection;
    
    QTest::qWait( 1000 );
    
}

#define ENABLE_DEBUG_OUTPUT_
void UnitTests::testCapabilityManager()
{
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none..
    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
// verbinden mit erstem konto
    TpPrototype::Connection* connection = account->connection();
    QVERIFY( connection );
    // The connection shouldn't provide a capability manager if no connection is available!
    QVERIFY( connection->capabilitiesManager() == NULL );

    QVERIFY( connection->requestConnect() == true );
    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");
    
    TpPrototype::CapabilitiesManager* cap_manager = connection->capabilitiesManager();
    QVERIFY( cap_manager );

    // Need to get the list of my contacts to request their capability:
    QVERIFY( connection->contactManager() );
    QList<QPointer<TpPrototype::Contact> > contact_list = connection->contactManager()->contactList();

    // TODO: Create contacts if the following fails.
    QVERIFY2( contact_list.count() != 0, "Account has no contacts assigned! Cannot request any capabilities!" );

    cap_manager->capabilitiesForContactList( contact_list );

    foreach( TpPrototype::Contact* contact, contact_list )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Contact: " << contact->name();
#endif
        foreach( Telepathy::ContactCapability cap, contact->capabilities() )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "capabilitiesChannelType:" << cap.channelType << "capabilitiesGenericFlags:" << cap.genericFlags << "capabilitiesTypeSpecificFlags:" << cap.typeSpecificFlags;
#endif
            // Check minimum requirement: A text channel capability is available
            QVERIFY( cap.channelType == QString( "org.freedesktop.Telepathy.Channel.Type.Text" ) );
        }
    }

    foreach( Telepathy::ContactCapability cap, cap_manager->capabilities() )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "My capabilities: capabilitiesChannelType:" << cap.channelType << "capabilitiesGenericFlags:" << cap.genericFlags << "capabilitiesTypeSpecificFlags:" << cap.typeSpecificFlags;
#endif
            // Check minimum requirement: A text channel capability is available
        QVERIFY( cap.channelType == QString( "org.freedesktop.Telepathy.Channel.Type.Text" ) );
    }
        

    // Now checking setting of capabilities and whether we receive a signal after that..
    QSignalSpy spy_own_capability_changed( cap_manager, SIGNAL( signalOwnCapabilityChanged( const Telepathy::CapabilityChange& ) ) );
    QCOMPARE( spy_own_capability_changed.isValid(), true );

    Telepathy::CapabilityPair new_capability = { "org.freedesktop.Telepathy.Channel.Type.StreamedMedia", 15 }; // See Telepathy D-Bus spec section "Channel_Media_Capabilities"
    Telepathy::CapabilityPairList capability_list;
    capability_list << new_capability;
    QVERIFY( cap_manager->setCapabilities( capability_list ) );

    QVERIFY2( waitForSignal( &spy_own_capability_changed ), "Received no signal after changing my capability! ");

    bool found_media_stream_channel = false;
    foreach( Telepathy::ContactCapability cap, cap_manager->capabilities() )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "My changed capabilities: capabilitiesChannelType:" << cap.channelType << "capabilitiesGenericFlags:" << cap.genericFlags << "capabilitiesTypeSpecificFlags:" << cap.typeSpecificFlags;
#endif
        // Check whether the Stream Media channel is registered succesfully
        if ( cap.channelType == QString( "org.freedesktop.Telepathy.Channel.Type.StreamedMedia" ) )
        { found_media_stream_channel = true; }
    }
    // QEXPECT_FAIL( "", "The registered channel was not found. We need to investigate why!", Continue );
    QVERIFY( found_media_stream_channel );
    
    connection->requestDisconnect();

    delete connection;

    QTest::qWait( 1000 );
}
#undef ENABLE_DEBUG_OUTPUT

void UnitTests::testAvatarManager()
{
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none..
    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
// verbinden mit erstem konto
    TpPrototype::Connection* connection = account->connection();
    QVERIFY( connection );

    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY( connection->requestConnect() == true );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");
    
    TpPrototype::AvatarManager* avatar_manager = connection->avatarManager();
    QVERIFY( avatar_manager );

    // Get Avatar requirements
    TpPrototype::AvatarManager::AvatarRequirements avatar_requirements = avatar_manager->avatarRequirements();
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Avatar requirements: ";
    qDebug() << "mimeTypes     :" << avatar_requirements.mimeTypes;
    qDebug() << "minimumWidth  :" << avatar_requirements.minimumWidth;
    qDebug() << "minimumHeight :" << avatar_requirements.minimumHeight;
    qDebug() << "maximumWidth  :" << avatar_requirements.maximumWidth;
    qDebug() << "maximumHeight :" << avatar_requirements.maximumHeight;
    qDebug() << "maxSize       :" << avatar_requirements.maxSize;
#endif
    // Check whether previous call failed.
    QEXPECT_FAIL( "", "This issue was reported on bugs.freedesktop.org: #18202", Continue );
    QVERIFY( avatar_requirements.isValid );

    // Set own avatar.
    QString abs_top_srcdir = QString::fromLocal8Bit(::getenv("abs_top_srcdir"));
    QVERIFY2(!abs_top_srcdir.isEmpty(),
            "Put $abs_top_srcdir in your environment");
    QPixmap avatar( abs_top_srcdir + "/tests/prototype/avatar.png" );
    QVERIFY( !avatar.isNull() );
    
    QByteArray bytes;
    QBuffer buffer( &bytes );
    buffer.open( QIODevice::WriteOnly );
    avatar.save( &buffer, "PNG" );
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Avatar size is: " << bytes.size();
#endif
    
    TpPrototype::AvatarManager::Avatar local_avatar;
    local_avatar.avatar = bytes;
    local_avatar.mimeType = "image/png";

    QEXPECT_FAIL( "", "This fails on gabble but works well on Salut. Issue was reported on bugs.freedesktop.org: #18303", Continue );
    QVERIFY( avatar_manager->setAvatar( local_avatar ) );
    
    // Request own avatar
    QSignalSpy spy_request_local_avatar( avatar_manager, SIGNAL( signalOwnAvatarChanged( TpPrototype::AvatarManager::Avatar ) ) );
    QVERIFY( spy_request_local_avatar.isValid() );
    avatar_manager->requestAvatar();
    QVERIFY2( waitForSignal( &spy_request_local_avatar ), "Received no signal after requesting the local avatar!" );
    
    // Request Avatar of a contact
    // This might fail if the first contact does not provide an avatar. In this case we will receive no signal.. :(
    QList<QPointer<TpPrototype::Contact> > contact_list;
    foreach( TpPrototype::Contact* contact, connection->contactManager()->contactList() )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Request avatar for contact: " << contact->name();
#endif
        contact_list << contact;
        break; // Test one contact should be ok for now..
    }
    QSignalSpy spy_request_contact_avatar( avatar_manager, SIGNAL( signalAvatarChanged( TpPrototype::Contact* ) ) );
    QVERIFY( spy_request_contact_avatar.isValid() );
    avatar_manager->avatarForContactList( contact_list );
    // QEXPECT_FAIL( "", "This test may fail if the first contact has no avatar.", Continue );
    QVERIFY2( waitForSignal( &spy_request_contact_avatar ), "Received no signal after requesting an avatar of a contact! " );

    connection->requestDisconnect();
    delete connection;
}

void UnitTests::testStreamedMedia_receiveCall()
{
    // Connect to first account.
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none.. TODO: Add avalid account here that supports VoIP calls.
    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
// verbinden mit erstem konto
    TpPrototype::Connection* connection = account->connection();
    QVERIFY( connection );

    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY( connection->requestConnect() == true );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");

    // The StreamedMedia channel is stored in the calling contact and new channels are signalled by the ContactManager.
    TpPrototype::ContactManager* contact_manager = connection->contactManager();
    QVERIFY( NULL != contact_manager );

    // Start Support of VoIP
    TpPrototype::CapabilitiesManager* capabilities_manager = connection->capabilitiesManager();
    QVERIFY( NULL != capabilities_manager );

    Telepathy::CapabilityPair new_capability = { "org.freedesktop.Telepathy.Channel.Type.StreamedMedia",
        Telepathy::ChannelMediaCapabilityAudio
        | Telepathy::ChannelMediaCapabilityVideo
        | Telepathy::ChannelMediaCapabilityNATTraversalGTalkP2P  }; // See Telepathy D-Bus spec section "Channel_Media_Capabilities"
    Telepathy::CapabilityPairList capability_list;
    capability_list << new_capability;
    capabilities_manager->setCapabilities( capability_list );
    
    // Now wait for a calling user..
    qDebug() << "Wait for call.. (timout after 10 seconds)";
    connect( contact_manager, SIGNAL( signalStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ),
             this, SLOT( slotReceiveContactPointer( TpPrototype::Contact*  ) ) );
    QSignalSpy spy_wait_for_call( contact_manager, SIGNAL( signalStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ) );
    QCOMPARE( spy_wait_for_call.isValid(), true );
    QVERIFY2( waitForSignal( &spy_wait_for_call ), "Timout waiting for call..");

    // Get StreamedMediaChannel
    // The channel is contained in the contact that was emitted with the signal signalStreamedMediaChannelOpenedForContact().
    QList<QVariant> parameters = spy_wait_for_call.takeFirst();
    QVERIFY( parameters.count() == 1 );

    TpPrototype::Contact* calling_contact = m_pContactPointer/*qobject_cast<TpPrototype::Contact*>( parameters.at(0) )*/;
    QVERIFY( NULL != calling_contact );
    
    // Get StreamedMediaChannel and accept call.
    qDebug() << "Accept-Call..";
    TpPrototype::StreamedMediaChannel* media_channel = calling_contact->streamedMediaChannel();
    QVERIFY( NULL != media_channel );
    media_channel->acceptIncomingStream();

    qDebug() << "Reject call after 5 seconds..";
    QTest::qWait( 5000 );
    // Reject the call now.
    media_channel->rejectIncomingStream();

    QTest::qWait( 5000 );
    
    // Now try get a working connection again to check whether the cleanup works..
    qDebug() << "Wait for second call.. (timout after 10 seconds)";
    QSignalSpy spy_wait_for_call_attempt2( contact_manager, SIGNAL( signalStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ) );
    QCOMPARE( spy_wait_for_call_attempt2.isValid(), true );
    QVERIFY2( waitForSignal( &spy_wait_for_call_attempt2 ), "Timout waiting for call (attempt2)..");

    // Get StreamedMediaChannel and accept call.
    qDebug() << "Accept-Call..";
    media_channel = calling_contact->streamedMediaChannel();
    QVERIFY( NULL != media_channel );
    QVERIFY( media_channel->acceptIncomingStream() );

    qDebug() << "Reject call after 5 seconds..";
    QTest::qWait( 5000 );
    // Reject the call now.
    QVERIFY( media_channel->rejectIncomingStream() );

    connection->requestDisconnect();
    delete connection;

}

void UnitTests::testStreamedMedia_outgoingCall()
{
    // Connect to first account.
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    // Create an account if there is none.. TODO: Add avalid account here that supports VoIP calls.
    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "ContactHandlingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
    
    // Connect to first account
    TpPrototype::Connection* connection = account->connection();
    QVERIFY( connection );

    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY( connection->requestConnect() == true );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");

    // The StreamedMedia channel is stored in the calling contact and new channels are signalled by the ContactManager.
    TpPrototype::ContactManager* contact_manager = connection->contactManager();
    QVERIFY( NULL != contact_manager );


    if ( contact_manager->contactList().count() == 0 )
    {
        QSignalSpy spy_for_contacts( contact_manager, SIGNAL( signalMembersChanged( TpPrototype::ContactManager*,
                                                                                    const QString& ,
                                                                                    QList<QPointer<TpPrototype::Contact> > ,
                                                                                    QList<QPointer<TpPrototype::Contact> > ,
                                                                                    QList<QPointer<TpPrototype::Contact> > ,
                                                                                    TpPrototype::Contact* ,
                                                                                    Telepathy::ChannelGroupChangeReason )
                                                            ) );
        QCOMPARE( spy_for_contacts.isValid(), true );
        waitForSignal( &spy_for_contacts );
    }

    // Get StreamedMediaChannel and call the first contact.
    QList<QPointer<TpPrototype::Contact> > contact_list = contact_manager->contactList();

    QVERIFY2( contact_list.count() > 0, "No contacts were found to call.." );
    TpPrototype::Contact* contact = contact_list.at( 0 );
    
    qDebug() << "Calling: " << contact->name();
    TpPrototype::StreamedMediaChannel* media_channel = contact->streamedMediaChannel();
    QVERIFY( NULL != media_channel );
   
    media_channel->requestChannel( QList<Telepathy::MediaStreamType>() << Telepathy::MediaStreamTypeAudio );

    QTest::qWait( 50000 );

#if 0
    media_channel->requestChannel( QList<Telepathy::MediaStreamType>() << Telepathy::MediaStreamTypeAudio );

    QTest::qWait( 10000 );
#endif
    
    connection->requestDisconnect();
    delete connection;

}


void UnitTests::testBlockingSupport()
{
#if 0 // blocking not merged from prototype so far

     // Connect to first account.
    TpPrototype::AccountManager* account_manager = TpPrototype::AccountManager::instance();
    QVERIFY( NULL != account_manager );

    if ( account_manager->accountList().isEmpty() )
    {
        QVariantMap parameter_map = TpPrototype::ConnectionFacade::instance()->parameterListForProtocol( "jabber" );
        QVERIFY( true == account_manager->createAccount( "gabble", "jabber", "BlockingTest", parameter_map) );
        QTest::qWait( 1000 );
        
        QVERIFY( account_manager->accountList().count() != 0 );
    }

    TpPrototype::Account* account = account_manager->accountList().at( 0 );
    QVERIFY( NULL != account );
    // Connect to first account
    TpPrototype::Connection* connection = account->connection();
    QVERIFY( connection );

    QSignalSpy spyConnectionStatusChanged( connection, SIGNAL(signalStatusChanged (TpPrototype::Connection* , Telepathy::ConnectionStatus, Telepathy::ConnectionStatus ) ) );
    QVERIFY( connection->requestConnect() == true );
    QVERIFY2( waitForSignal( &spyConnectionStatusChanged, 2 ), "Received no signal after connectRequest ");

    // The StreamedMedia channel is stored in the calling contact and new channels are signalled by the ContactManager.
    TpPrototype::ContactManager* contact_manager = connection->contactManager();
    QVERIFY( NULL != contact_manager );


    if ( contact_manager->contactList().count() == 0 )
    {
        QSignalSpy spy_for_contacts( contact_manager, SIGNAL( signalMembersChanged( TpPrototype::ContactManager*,
                                     const QString& ,
                                     QList<QPointer<TpPrototype::Contact> > ,
                                     QList<QPointer<TpPrototype::Contact> > ,
                                     QList<QPointer<TpPrototype::Contact> > ,
                                     TpPrototype::Contact* ,
                                     Telepathy::ChannelGroupChangeReason )
                                                            ) );
        QCOMPARE( spy_for_contacts.isValid(), true );
        waitForSignal( &spy_for_contacts );
    }
   
    QVERIFY2( contact_manager->contactList().count() > 0, "No contacts found!" );

#if 1 // Disable this if a contact was bloccked and should be unblocked
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "block contact: " << contact_manager->contactList().at(0)->name();
#endif
    
    QSignalSpy spy_for_blocking_signals( contact_manager, SIGNAL( signalContactBlocked( TpPrototype::ContactManager* ,
                                                                    TpPrototype::Contact*  ) ) );
    QCOMPARE( spy_for_blocking_signals.isValid(), true );
    
    contact_manager->blockContact( contact_manager->contactList().at( 0 ) );

    
    QVERIFY2( waitForSignal( &spy_for_blocking_signals ), "Blocking of contact failed" );

#endif

    QTest::qWait( 1000 );
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "_un_block contact: " << contact_manager->contactList().at(0)->name();
#endif
    QSignalSpy spy_for_unblocking_signals( contact_manager, SIGNAL( signalContactUnblocked( TpPrototype::ContactManager* ,
                                         TpPrototype::Contact*  ) ) );
    QCOMPARE( spy_for_unblocking_signals.isValid(), true );
    
    contact_manager->unblockContact( contact_manager->contactList().at( 0 ) );

    QVERIFY2( waitForSignal( &spy_for_unblocking_signals ), "Unblocking of contact failed!" );

    delete connection;
#endif
}



void UnitTests::slotReceiveContactPointer( TpPrototype::Contact* pointer )
{
    qDebug() << "slotReceiveContactPointer:" << pointer;
    m_pContactPointer = pointer;
}
    


QTEST_MAIN( UnitTests )
#include "_gen/prototype.h.moc"
