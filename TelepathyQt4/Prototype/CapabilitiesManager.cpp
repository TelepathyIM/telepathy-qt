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

#include "TelepathyQt4/Prototype/CapabilitiesManager.h"

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

using namespace TpPrototype;

class TpPrototype::CapabilitiesManagerPrivate
{
public:
    CapabilitiesManagerPrivate()
    { init(); }

    Telepathy::Client::ConnectionInterface* m_pConnectionInterface;
    Telepathy::Client::ConnectionInterfaceCapabilitiesInterface* m_pCapabilitiesInterface;

   
    QPointer<Connection> m_pConnection;
    
    bool m_isValid;
    void init()
    {
        m_pConnectionInterface   = NULL;
        m_pCapabilitiesInterface = NULL;
        m_pConnection            = NULL;
        m_isValid                = true;
    }

};

CapabilitiesManager::CapabilitiesManager( Connection* connection,
                                  Telepathy::Client::ConnectionInterface* interface,
                                  QObject* parent ):
    QObject( parent ),
    d( new CapabilitiesManagerPrivate )
{
    init( connection, interface );
}

CapabilitiesManager::~CapabilitiesManager()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "D'tor CapabilitiesManager (" << this << ")";
#endif
    delete d;
}

bool CapabilitiesManager::isValid()
{
    return d->m_isValid;
}



TpPrototype::Connection* CapabilitiesManager::connection()
{
    return d->m_pConnection;
}

bool CapabilitiesManager::setCapabilities( const Telepathy::CapabilityPairList& capabilities, const QStringList& removedChannels )
{
    QDBusPendingReply<Telepathy::CapabilityPairList> advertise_capabilities_reply = d->m_pCapabilitiesInterface->AdvertiseCapabilities( capabilities, removedChannels );
    advertise_capabilities_reply.waitForFinished();
    
    if ( !advertise_capabilities_reply.isValid() )
    {
        QDBusError error = advertise_capabilities_reply.error();
    
        qWarning() << "CapabilitiesManager::AdvertiseCapabilities: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return false;
    }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "CapabilitiesManager::setCapabilities: " << capabilities.count();
    qDebug() << "CapabilitiesManager::setCapabilities: " << advertise_capabilities_reply.value().count();
#endif    
    
    return true;
}

Telepathy::ContactCapabilityList CapabilitiesManager::capabilities()
{
    uint self_handle = TpPrototype::ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pConnectionInterface );
    QList<uint> handle_list;
    handle_list.append( self_handle );
    QDBusPendingReply<Telepathy::ContactCapabilityList> capabilities_reply = d->m_pCapabilitiesInterface->GetCapabilities( handle_list );
    capabilities_reply.waitForFinished();

    if ( !capabilities_reply.isValid() )
    {
        QDBusError error = capabilities_reply.error();
    
        qWarning() << "CapabilitiesManager::capabilities: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return Telepathy::ContactCapabilityList();
    }
  
    Telepathy::ContactCapabilityList capabilities=capabilities_reply.value();

    return capabilities;
}


void CapabilitiesManager::capabilitiesForContactList( const QList<QPointer<Contact> >& contacts )
{
    Q_ASSERT( d->m_pCapabilitiesInterface );
    Telepathy::UIntList contact_ids;
    foreach( Contact* contact, contacts )
    {
        if ( !contact )
        { continue; }
        contact_ids.append( contact->telepathyHandle() );
    }

    QDBusPendingReply<Telepathy::ContactCapabilityList> capabilities_reply = d->m_pCapabilitiesInterface->GetCapabilities( contact_ids );
    capabilities_reply.waitForFinished();

    if ( !capabilities_reply.isValid() )
    {
        QDBusError error = capabilities_reply.error();
    
        qWarning() << "GetInterfaces: error type:" << error.type()
                << "GetInterfaces: error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return;
    }
  
    Telepathy::ContactCapabilityList capabilities=capabilities_reply.value();

    foreach( Contact* contact, contacts )
    {
        Telepathy::ContactCapabilityList contact_capabilities;
        for ( int i=0; i < capabilities.size(); i++ )
        {
            Telepathy::ContactCapability capability = capabilities.value( i );
            if ( contact && contact->telepathyHandle() == capability.handle )
            {
                contact_capabilities.append( capability );
            }
        }
        contact->setCapabilities( contact_capabilities );
    }
}

void CapabilitiesManager::slotCapabilitiesChanged( const Telepathy::CapabilityChangeList& capabilities )
{
    if ( !d->m_pConnection )
    {
        qWarning() << "CapabilitiesManager::slotCapabilitiesChanged(): Received a Capabilities changed signal but no connection object exists!";
        return;
    }

    Q_ASSERT( d->m_pCapabilitiesInterface );
    Q_ASSERT( d->m_pCapabilitiesInterface->isValid() );

    QPointer<TpPrototype::Contact> contact;
    for (int i=0; i<capabilities.size(); i++)
    {

        Telepathy::CapabilityChange changed_capability = capabilities.value( i );
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "CapabilityChange "<< i<< "handle" <<changed_capability.handle;
            qDebug() << "CapabilityChange"<< i << "CannelType" <<changed_capability.channelType;
            qDebug() << "CapabilityChange "<< i << "Generic Flags" <<changed_capability.oldGenericFlags;
            qDebug() << "CapabilityChange "<< i << "Generic Flags" <<changed_capability.newGenericFlags;
            qDebug() << "CapabilityChange "<< i << "Type Specific Flags" <<changed_capability.oldTypeSpecificFlags;
            qDebug() << "CapabilityChange "<< i << "Type Specific Flags" <<changed_capability.newTypeSpecificFlags;
#endif
        
        if ( !d->m_pConnection->contactManager() )
        {
            qWarning() << "CapabilitiesManager::slotCapabilitiesChanged(): Unable to request contact manager!";
            return;
        }

        uint self_handle = ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pConnectionInterface );
        foreach( const Telepathy::CapabilityChange& changed_capability, capabilities )
        {
            if ( changed_capability.handle == self_handle )
            {
                emit signalOwnCapabilityChanged( changed_capability );
            }
            else
            {
                foreach( Contact* contact, d->m_pConnection->contactManager()->contactList() )
                {
                    if ( contact && contact->telepathyHandle() == changed_capability.handle )
                    {
                        // Modify stored list of capabilities
                        Telepathy::ContactCapabilityList contact_capabilities = contact->capabilities();
                        for ( int i = 0; i < contact_capabilities.size(); ++i )
                        {
                            if ( contact_capabilities.at(i).channelType == changed_capability.channelType )
                            {
                                contact_capabilities.removeAt( i );
                            }
                        }

                        Telepathy::ContactCapability new_capability;
                        new_capability.handle            = changed_capability.handle;
                        new_capability.channelType       = changed_capability.channelType;
                        new_capability.genericFlags      = changed_capability.newGenericFlags;
                        new_capability.typeSpecificFlags = changed_capability.newTypeSpecificFlags;
                        contact_capabilities.append( new_capability );
                        
                        contact->setCapabilities( contact_capabilities );
                        emit signalCapabilitiesChanged( contact, changed_capability );
                    }
                }
            }
        }
    }
}


void CapabilitiesManager::init( Connection* connection,
                            Telepathy::Client::ConnectionInterface* interface )
{
    Q_ASSERT( interface );

    if ( !interface || !connection )
    {
        d->m_isValid = false;
        return;
    }
    
    Telepathy::registerTypes();
    d->m_pConnectionInterface = interface;
    d->m_pConnection          = connection;
    QDBusPendingReply<QStringList> interfaces_reply = d->m_pConnectionInterface->GetInterfaces(); 
    interfaces_reply.waitForFinished();

    if ( !interfaces_reply.isValid() )
    {
        QDBusError error = interfaces_reply.error();
    
        qWarning() << "GetInterfaces: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return;
    }
    QString capabilities_interface_name;
    bool found_capabilities_support = false;

    
    foreach( const QString& interface, interfaces_reply.value() )
    {
        if ( interface.endsWith( ".Capabilities" ) )
        {
            found_capabilities_support = true;
            capabilities_interface_name = interface;
            break;
        }     
    }
    if ( !found_capabilities_support )
    {
        d->m_isValid = false;
        qWarning( "CapabilitiesManager::init(): Connection Manager does not support the Interface \"Capabilities\". Other interfaces are not supported!" );
        return;
    }

#ifdef ENABLE_DEBUG_OUTPUT_
    //qDebug() << "Connection interface :" << d->m_pConnectionInterface->connection().interface()->path();
    qDebug() << "Interface Name: " << capabilities_interface_name;
#endif
    d->m_pCapabilitiesInterface = new Telepathy::Client::ConnectionInterfaceCapabilitiesInterface(d->m_pConnectionInterface->service(),d->m_pConnectionInterface->path(),this );
    
    connect( d->m_pCapabilitiesInterface, SIGNAL( CapabilitiesChanged( const Telepathy::CapabilityChangeList& ) ),
             this, SLOT( slotCapabilitiesChanged( const Telepathy::CapabilityChangeList& ) ) );
   
}
            

