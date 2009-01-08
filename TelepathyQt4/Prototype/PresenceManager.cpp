/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 basysKom GmbH
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "TelepathyQt4/Prototype/PresenceManager.h"

#include <QCoreApplication>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDebug>
#include <QMap>
#include <QPointer>

#include <TelepathyQt4/Client/Connection>

#include <TelepathyQt4/Prototype/Account.h>
#include <TelepathyQt4/Prototype/AccountManager.h>
#include <TelepathyQt4/Prototype/ConnectionFacade.h>
#include <TelepathyQt4/Prototype/Connection.h>
#include <TelepathyQt4/Prototype/Contact.h>
#include <TelepathyQt4/Prototype/ContactManager.h>

// #define ENABLE_DEBUG_OUTPUT_

namespace
{
    const char* g_offlineStatusString      = "offline";
    const char* g_statusParamMessageString = "message";
}

using namespace TpPrototype;

class TpPrototype::PresenceManagerPrivate
{
public:
    PresenceManagerPrivate( Connection* connection,
                            Telepathy::Client::ConnectionInterface* interface )
    { init( connection, interface ); }

    Telepathy::Client::ConnectionInterface* m_pConnectionInterface;
    Telepathy::Client::ConnectionInterfaceSimplePresenceInterface* m_pSimplePresenceInterface;
    Telepathy::Client::ConnectionInterfacePresenceInterface* m_pPresenceInterface;
    QPointer<Connection> m_pConnection;
    bool m_isValid;
    QMap<QString,int> m_mapStatusStringToType;
    
    void init( Connection* connection,
               Telepathy::Client::ConnectionInterface* interface )
    {
        m_pConnectionInterface     = interface;
        m_pConnection              = connection;
        m_pSimplePresenceInterface = NULL;
        m_pPresenceInterface       = NULL;        
        m_isValid                  = true;

        m_mapStatusStringToType.insert( "offline"  , 1 /*Connection_Presence_Type_Offline*/ ); //TODO: Why is there no enum for these types?
        m_mapStatusStringToType.insert( "available", 2 /*Connection_Presence_Type_Available*/ );
        m_mapStatusStringToType.insert( "away"     , 3 /*Connection_Presence_Type_Away*/ );
        m_mapStatusStringToType.insert( "brb"      , 3 /*Connection_Presence_Type_Away*/  );
        m_mapStatusStringToType.insert( "xa"       , 4 /*Connection_Presence_Type_Extended_Away*/ );
        m_mapStatusStringToType.insert( "hidden"   , 5 /*Connection_Presence_Type_Busy*/ );
        m_mapStatusStringToType.insert( "busy"     , 6 /*Connection_Presence_Type_Busy*/  );
        m_mapStatusStringToType.insert( "dnd"      , 6 /*Connection_Presence_Type_Busy*/ );
        m_mapStatusStringToType.insert( "unknown"  , 7 /*Connection_Presence_Type_Unknown*/ );
        m_mapStatusStringToType.insert( "error"    , 8 /*Connection_Presence_Type_Error*/ );
        
    }
    
    uint localHandle()
    {
        uint self_handle = ConnectionFacade::instance()->selfHandleForConnectionInterface( m_pConnectionInterface );
        if ( self_handle == 0 )
        { m_isValid = false; }
    
        return self_handle;
    }

// Protected functions

    uint mapStatusStringToType( const QString& status ) const
    {
        return m_mapStatusStringToType.value( status );
    }

    Telepathy::SimplePresence convertToSimplePresence( const Telepathy::LastActivityAndStatuses& status ) const
    {
        Telepathy::SimplePresence simple_presence;

        QStringList status_list = status.statuses.keys();

        Q_ASSERT( status_list.count() == 1 );
        if ( status_list.count() < 1 )
        { return simple_presence; }

        simple_presence.type          = mapStatusStringToType( status_list.at( 0 ) );
        simple_presence.status        = status_list.at( 0 );
        simple_presence.statusMessage = status.statuses.value( status_list.at( 0 ) ).value( g_statusParamMessageString ).toString();
    
        return simple_presence;
    }

    Telepathy::SimpleContactPresences convertToSimplePresences( const Telepathy::ContactPresences& presences ) const
    {
        Telepathy::SimpleContactPresences contact_presences;

        QList<uint> keys = presences.keys();
        foreach( uint key, keys )
        {
            contact_presences.insert( key, convertToSimplePresence( presences.value( key ) ) );
        }
    
        return contact_presences;
    }

    Telepathy::MultipleStatusMap convertToMultipleStatusMap( const QString& status, const QString& statusMessage ) const
    {
        Telepathy::MultipleStatusMap status_map;

        QVariantMap protocol_specif_parameters;
        if ( !statusMessage.isEmpty() )
        { protocol_specif_parameters.insert( g_statusParamMessageString, statusMessage ); }
        
        status_map.insert( status, protocol_specif_parameters ); 

        return status_map;
    }
};

PresenceManager::PresenceManager( Connection* connection,
                                  Telepathy::Client::ConnectionInterface* interface,
                                  QObject* parent ):
    QObject( parent ),
    d( new PresenceManagerPrivate( connection, interface ) )
{
    init();
}

PresenceManager::~PresenceManager()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "D'tor PresenceManager (" << this << ")";
#endif
    delete d;
}

bool PresenceManager::isValid()
{
    return d->m_isValid;
}

Telepathy::SimpleStatusSpecMap PresenceManager::statuses()
{
    Q_ASSERT( d->m_pSimplePresenceInterface || d->m_pPresenceInterface );

    if ( d->m_pSimplePresenceInterface )
    { return d->m_pSimplePresenceInterface->Statuses(); }

    if ( d->m_pPresenceInterface )
    {
        // We return a minimum set of states that should be provided by every contact manager and protocol
        Telepathy::SimpleStatusSpecMap ret_status;
        Telepathy::SimpleStatusSpec available = { 2, 2, true };
        ret_status.insert( "available", available );
        Telepathy::SimpleStatusSpec away = { 3, 3, true };
        ret_status.insert( "away", away );
        Telepathy::SimpleStatusSpec offline = { 1, 1, true };
        ret_status.insert( "offline", offline );
        
        return ret_status;
    } 

    return Telepathy::SimpleStatusSpecMap();
}

bool PresenceManager::setPresence( const QString& status, const QString& statusMessage )
{
    Q_ASSERT( d->m_pSimplePresenceInterface || d->m_pPresenceInterface );

    if ( !d->m_pSimplePresenceInterface && !d->m_pPresenceInterface )
    { return false; }
    
    // The state "offline" is presented as possible state with statuses(). Due to unknown reasons it is not
    // a valid name for setPresence(). Thus, we handle it on this layer..
    if ( status == g_offlineStatusString )
    {
        Q_ASSERT( d->m_pConnection );

        // TODO: Think whether sending this signal here manually (and before disconnecting) is really a good idea!
        const Telepathy::SimplePresence new_presence = { d->mapStatusStringToType( "offline" ), status, statusMessage };
        emit signalOwnPresenceUpdated( d->m_pConnection->account(), new_presence );
        
        d->m_pConnection->requestDisconnect();
        return true;
    }
    
    if ( d->m_pSimplePresenceInterface )
    {
        QDBusPendingReply<> set_presence_reply = d->m_pSimplePresenceInterface->SetPresence( status, statusMessage );
        set_presence_reply.waitForFinished();

        if ( !set_presence_reply.isValid() )
        {
            QDBusError error = set_presence_reply.error();
        
            qWarning() << "SetPresence: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();
            
            d->m_isValid = false;
            return false;
        }
        return true;
    }
    
    if ( d->m_pPresenceInterface )
    {
        QDBusPendingReply<> set_status_reply = d->m_pPresenceInterface->SetStatus( d->convertToMultipleStatusMap( status, statusMessage ) );
        set_status_reply.waitForFinished();
        if ( !set_status_reply.isValid() )
        {
            QDBusError error = set_status_reply.error();
        
            qWarning() << "SetStatus: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();

            d->m_isValid = false;
            return false;
        }

#if 0 // TODO: Exame further: At some point I received the signal. Don't know why..
        // The simple presence interface is emitting a signal if I changed the local presence here but the presence interface does not!
        // We simulate this behavior here..
        const Telepathy::SimplePresence new_presence = { d->mapStatusStringToType( status ), status, statusMessage };
        emit signalOwnPresenceUpdated( d->m_pConnection->account(), new_presence );
#endif   
        return true;
    }

    // Fall through..
    return false;
}

// TODO: This function is doing the same as presencesForContacts() but just for one contact! Merge both functions to remove code duplication!
Telepathy::SimplePresence PresenceManager::currentPresence()
{
    Q_ASSERT( d->m_pSimplePresenceInterface || d->m_pPresenceInterface );

    if ( !d->m_pConnection || ( !d->m_pSimplePresenceInterface && !d->m_pPresenceInterface ) )
    {
        return Telepathy::SimplePresence();
    }

    Telepathy::UIntList uid_list;
    uid_list << d->localHandle();

    if ( d->m_pSimplePresenceInterface )
    {
        QDBusPendingReply<Telepathy::SimpleContactPresences> get_presence_reply = d->m_pSimplePresenceInterface->GetPresences( uid_list );
        get_presence_reply.waitForFinished();
        
        if ( !get_presence_reply.isValid() )
        {
            QDBusError error = get_presence_reply.error();
        
            qWarning() << "GetPresences: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();

            d->m_isValid = false;
            return Telepathy::SimplePresence();
        }

        Telepathy::SimpleContactPresences presence_list = get_presence_reply.value();
        QList<Telepathy::SimplePresence> value_list = presence_list.values();
        
        if ( value_list.count() == 0 )
        { return Telepathy::SimplePresence(); }

        return value_list.at( 0 );
    }

    if ( d->m_pPresenceInterface )
    {
        QDBusPendingReply<Telepathy::ContactPresences> get_presence_reply = d->m_pPresenceInterface->GetPresence( uid_list );
        get_presence_reply.waitForFinished();
        
        if ( !get_presence_reply.isValid() )
        {
            QDBusError error = get_presence_reply.error();
        
            qWarning() << "GetPresence: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();

            d->m_isValid = false;
            return Telepathy::SimplePresence();
        }

        Telepathy::ContactPresences presence_list = get_presence_reply.value();
        QList<Telepathy::LastActivityAndStatuses> value_list = presence_list.values();
        
        if ( value_list.count() == 0 )
        { return Telepathy::SimplePresence(); }

        return d->convertToSimplePresence( value_list.at( 0 ) );
    }

    // Fall through..
    return Telepathy::SimplePresence();
}

// TODO: See comment for currentPresence! Merge both to remove code duplication
Telepathy::SimpleContactPresences PresenceManager::presencesForContacts( const QList<QPointer<Contact> >& contacts )
{
    Q_ASSERT( d->m_pSimplePresenceInterface || d->m_pPresenceInterface );

    if ( !d->m_pConnection || ( !d->m_pSimplePresenceInterface && !d->m_pPresenceInterface ) )
    { return Telepathy::SimpleContactPresences(); }
    
    Telepathy::UIntList contact_ids;
    foreach( Contact* contact, contacts )
    {
        if ( !contact )
        { continue; }
        contact_ids.append( contact->telepathyHandle() );
    }

    if ( d->m_pSimplePresenceInterface )
    {
        QDBusPendingReply<Telepathy::SimpleContactPresences> get_presence_reply = d->m_pSimplePresenceInterface->GetPresences( contact_ids );
        get_presence_reply.waitForFinished();

        if ( !get_presence_reply.isValid() )
        {
            QDBusError error = get_presence_reply.error();
        
            qWarning() << "GetPresences: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();

            return Telepathy::SimpleContactPresences();
        }
    
        Telepathy::SimpleContactPresences presences=get_presence_reply.value();
        QList<uint> keys = presences.keys();
        QPointer<TpPrototype::Contact> contact;
        foreach( uint key, keys )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "presencesForContacts Key"<<key;
#endif
            for (int i=0; i<contacts.size(); i++)
            {

                contact=contacts.at( i);
#ifdef ENABLE_DEBUG_OUTPUT_
                qDebug() << "Contact "<< key << "Status" <<presences.value( key ).status << "Message:" << presences.value( key ).statusMessage;
#endif
                if (contact->telepathyHandle()==key)
                {
                    contact->setPresenceType(presences.value( key ).type);
                    contact->setPresenceStatus(presences.value( key ).status);
                    contact->setPresenceMessage(presences.value( key ).statusMessage);
                }
            }
        }
        
        return get_presence_reply.value();
    }
    
    if ( d->m_pPresenceInterface )
    {
        QDBusPendingReply<Telepathy::ContactPresences> get_presence_reply = d->m_pPresenceInterface->GetPresence( contact_ids );
        get_presence_reply.waitForFinished();
        
        if ( !get_presence_reply.isValid() )
        {
            QDBusError error = get_presence_reply.error();
        
            qWarning() << "GetPresence: error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();

            d->m_isValid = false;
            return Telepathy::SimpleContactPresences();
        }

        Telepathy::ContactPresences presences = get_presence_reply.value();
        QList<uint> keys = presences.keys();
        QPointer<TpPrototype::Contact> contact;
        foreach( uint key, keys )
        {
            //qDebug() << "presencesForContacts Key"<<key;
            for ( int i=0; i<contacts.size(); i++ )
            {
                contact=contacts.at( i);
                //qDebug() << "Contact "<< key << "Status" <<presences.value( key ).status;            
                if (contact->telepathyHandle()==key)
                {
                    Telepathy::SimplePresence simple_presence = d->convertToSimplePresence( presences.value( key ) );
                    contact->setPresenceType( simple_presence.type );
                    contact->setPresenceStatus( simple_presence.status );
                    contact->setPresenceMessage( simple_presence.statusMessage );
                }
            }
        }
        
        return d->convertToSimplePresences( presences );
    }

    // Fall through..
    return Telepathy::SimpleContactPresences();
}

// Protected slots
    
// Called by the _simple_ presence interface if presence were updated
void PresenceManager::slotPresencesChanged( const Telepathy::SimpleContactPresences& presences )
{
    if ( !d->m_pConnection )
    {
        qWarning() << "PresenceManager::slotPresencesChanged(): Received a presence changed signal but no connection object exists!";
        return;
    }
        
    QList<uint> keys = presences.keys();
    foreach( uint key, keys )
    {
        Telepathy::SimplePresence changed_presence = presences.value( key );
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Contact ID: "  << key
                 << "Type: "        << changed_presence.type
                 << "Status:"       << changed_presence.status
                 << "StatusMessage:"<< changed_presence.statusMessage;
#endif
        
        if ( d->localHandle() == key )
        {
            if ( !d->m_pConnection->account() )
            {
                qWarning() << "PresenceManager::slotPresencesChanged(): Connection without account!";
                return;
            }
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "PresenceManager::slotPresencesChanged() -> signalOwnPresenceUpdated";
#endif
            emit signalOwnPresenceUpdated( d->m_pConnection->account(), changed_presence );
        } 
        else
        {
            if ( !d->m_pConnection->contactManager() )
            {
                qWarning() << "PresenceManager::slotPresencesChanged(): Unable to request contact manager!";
                return;
            }

            Contact* found_contact = NULL;
            foreach( Contact* contact, d->m_pConnection->contactManager()->contactList() )
            {
                if ( contact && contact->telepathyHandle() == key )
                { found_contact = contact; }
            }
            if ( found_contact )
            {
                found_contact->setPresenceType(changed_presence.type);
                found_contact->setPresenceStatus(changed_presence.status);
                found_contact->setPresenceMessage(changed_presence.statusMessage);

#ifdef ENABLE_DEBUG_OUTPUT_
                qDebug() << "PresenceManager::slotPresencesChanged() -> signalRemotePresencesUpdated";
#endif
                emit signalRemotePresencesUpdated( found_contact, changed_presence );
            }
            else
            {
                qWarning() << "PresenceManager::slotPresencesChanged: Received a signal that a non existing contact changed its presence!";
            }
        }
    }
}

// Called by the presence interface (that one without "simple" prefix) if any presences were updated
void PresenceManager::slotPresencesUpdate( const Telepathy::ContactPresences& presences )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "PresenceManager::slotPresencesUpdate()";
    QList<uint> keys = presences.keys();
    foreach( uint key, keys )
    {
        qDebug() << "Contact ID  : "  << key
                 << "LastActivity: "  << presences.value( key ).lastActivity;
        Telepathy::MultipleStatusMap statuses = presences.value( key ).statuses; // QMap<QString, QVariantMap>
        QList<QString> status_keys = statuses.keys();
        foreach( QString status_key, status_keys )
        {
            qDebug() << "Status:" << status_key << "Map:" << statuses.value( status_key );
        }
   }
#endif
   slotPresencesChanged( d->convertToSimplePresences( presences ) );
}

TpPrototype::Connection * PresenceManager::connection()
{
    return d->m_pConnection;
}

void PresenceManager::init()
{
    Q_ASSERT( d->m_pConnectionInterface );

    if ( !d->m_pConnectionInterface || !d->m_pConnection )
    {
        d->m_isValid = false;
        return;
    }
    
    Telepathy::registerTypes();
    QDBusPendingReply<QStringList> interfaces_reply = d->m_pConnectionInterface->GetInterfaces();
    interfaces_reply.waitForFinished();

    if ( !interfaces_reply.isValid() )
    {
        QDBusError error = interfaces_reply.error();
    
        qWarning() << "GetInterfaces: error type:" << error.type()
                   << "GetInterfaces: error name:" << error.name();

        d->m_isValid = false;
        return;
    }
    
    QString simplepresence_interface_name;
    bool found_simple_presence_support = false;
    QString presence_interface_name;
    bool found_presence_support = false;
    
    foreach( const QString& interface, interfaces_reply.value() )
    {
        if ( interface.endsWith( ".SimplePresence" ) )
        {
            found_simple_presence_support = true;
            simplepresence_interface_name = interface;
        }
        if ( interface.endsWith( ".Presence" ) )
        {
            found_presence_support = true;
            presence_interface_name = interface;
        }
    }
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Simple Presence Interface Name: " << simplepresence_interface_name;
    qDebug() << "Presence Interface Name       : " << presence_interface_name;
#endif

    // FIXME: The following line shound be commented out!!
    // found_simple_presence_support = false; // Override to test the (not simple) presence interface implementation
    
    if ( found_simple_presence_support )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug( "PresenceManager::init(): Connection Manager provides the Interface \"SimplePresence\". I will use this one!" );
#endif
        d->m_pSimplePresenceInterface = new Telepathy::Client::ConnectionInterfaceSimplePresenceInterface( d->m_pConnectionInterface->service(),
                                                                                                           d->m_pConnectionInterface->path(),
                                                                                                           this );
        Q_ASSERT( d->m_pSimplePresenceInterface );
        Q_ASSERT( d->m_pSimplePresenceInterface->isValid() );

        connect( d->m_pSimplePresenceInterface, SIGNAL( PresencesChanged( const Telepathy::SimpleContactPresences& ) ),
                 this, SLOT( slotPresencesChanged( const Telepathy::SimpleContactPresences& ) ) );

        return;
    }
    
    if ( found_presence_support )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug( "PresenceManager::init(): Connection Manager provides the Interface \"Presence\". I will use this one!" );
#endif
        d->m_pPresenceInterface = new Telepathy::Client::ConnectionInterfacePresenceInterface( d->m_pConnectionInterface->service(),
                                                                                               d->m_pConnectionInterface->path(),
                                                                                               this );
        Q_ASSERT( d->m_pPresenceInterface );
        Q_ASSERT( d->m_pPresenceInterface->isValid() );

        connect( d->m_pPresenceInterface, SIGNAL( PresenceUpdate(const Telepathy::ContactPresences& ) ),
                 this, SLOT( slotPresencesUpdate( const Telepathy::ContactPresences& ) ) );

        return;

    }
    
    // Fall through: No valid presence interface was found..
    d->m_isValid = false;
    qWarning( "PresenceManager::init(): Connection Manager neither supports the Interface \"Presence\" nor the Interface \"SimplePresence\". Other interfaces are not supported!" );
    return;
}

#include "_gen/PresenceManager.h.moc"
