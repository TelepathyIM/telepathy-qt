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

#include "TelepathyQt4/Prototype/Connection.h"

#include <QDebug>
#include <QMetaProperty>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ConnectionManager>

#include <TelepathyQt4/Prototype/Account.h>
#include <TelepathyQt4/Prototype/AvatarManager.h>
#include <TelepathyQt4/Prototype/CapabilitiesManager.h>
#include <TelepathyQt4/Prototype/ContactManager.h>
#include <TelepathyQt4/Prototype/PresenceManager.h>

// #define ENABLE_DEBUG_OUTPUT_

namespace TpPrototype {

class ConnectionPrivate
{
public:
    ConnectionPrivate()
    { init(); }
    
    QString                                 m_serviceName;
    QString                                 m_objectPath;
    bool                                    m_isValid;
    Telepathy::ConnectionStatus             m_status;
    Telepathy::ConnectionStatusReason       m_reason;
    Telepathy::Client::ConnectionInterface* m_pInterface;
    QPointer<ContactManager>                m_pContactManager;
    QPointer<PresenceManager>               m_pPresenceManager;
    QPointer<CapabilitiesManager>           m_pCapabilitiesManager;
    QPointer<AvatarManager>                 m_pAvatarManager;
    QPointer<Account>                       m_pAccount;
    QString                                 m_connectionManager;
    QString                                 m_protocol;
    void init()
    {
        m_isValid              = false; // Will be set on true after initial initialization.
        m_status               = Telepathy::ConnectionStatusDisconnected;
        m_reason               = Telepathy::ConnectionStatusReasonNoneSpecified;
        m_pInterface           = NULL;
        m_pContactManager      = NULL;
        m_pPresenceManager     = NULL;
        m_pCapabilitiesManager = NULL;
        m_pAvatarManager       = NULL;
        m_pAccount             = NULL;
    }

    void cleanup()
    {
        m_isValid              = true;
        m_status               = Telepathy::ConnectionStatusDisconnected;
        m_reason               = Telepathy::ConnectionStatusReasonNoneSpecified;
        delete m_pInterface;
        m_pInterface           = NULL;
        delete m_pContactManager;
        m_pContactManager      = NULL;
        delete m_pPresenceManager;
        m_pPresenceManager     = NULL;
        delete m_pCapabilitiesManager;
        m_pCapabilitiesManager = NULL;
        delete m_pAvatarManager;
        m_pAvatarManager       = NULL;
    }

    void initConnectionDBUSService()
    {
        cleanup();
        
        Q_ASSERT( m_pAccount );
        Q_ASSERT( !m_pInterface ); // needs to be removed. Otherwise all following will fail
        Q_ASSERT( !m_connectionManager.isEmpty() );
        Q_ASSERT( !m_protocol.isEmpty() );
        
        Telepathy::Client::ConnectionManagerInterface cm_interface( "org.freedesktop.Telepathy.ConnectionManager." + m_connectionManager,
                                                                    "/org/freedesktop/Telepathy/ConnectionManager/" +  m_connectionManager,
                                                                    NULL );


        QVariantMap parameter_map = m_pAccount->parameters();

        // 2. Request a connection to the server

#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Protocol: " << m_protocol;
        qDebug() << "Params  : " << parameter_map;
#endif

        QDBusPendingReply<QString, QDBusObjectPath> reply = cm_interface.RequestConnection( m_protocol, parameter_map );
        reply.waitForFinished();

        if ( !reply.isValid() )
        {
            QDBusError error = reply.error();
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "initConnectionDBUSService: error type:" << error.type()
                    << " error name:" << error.name();
#endif
            m_isValid = false;
            return /*NULL*/;
        }

        QString connection_service_name = reply.argumentAt<0>();
        QDBusObjectPath connection_object_path = reply.argumentAt<1>();
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Service Name: " << connection_service_name;
        qDebug() << "Object Path : " << connection_object_path.path();
#endif
        m_serviceName = connection_service_name;
        m_objectPath  = connection_object_path.path();
        
        m_pInterface  = new Telepathy::Client::ConnectionInterface( m_serviceName,
                                                                    m_objectPath,
                                                                    NULL );

    }
};
};

TpPrototype::Connection::Connection( TpPrototype::Account* account, QObject* parent ):
    QObject( parent ),
    d( new ConnectionPrivate )
{

    if ( !account )
    { return; }
    
    init( account );
}

using namespace TpPrototype;
Connection::~Connection()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "D'tor Connection:" << this;
#endif
    if ( Telepathy::ConnectionStatusDisconnected != d->m_status )
    { requestDisconnect(); }
    delete d;
}


bool Connection::isValid()
{
    return d->m_isValid;
}

Telepathy::ConnectionStatus Connection::status()
{
    return d->m_status;
}

Telepathy::ConnectionStatusReason Connection::reason()
{
    return d->m_reason;
}

bool Connection::requestConnect()
{
    if ( ( Telepathy::ConnectionStatusConnecting == d->m_status )
           || ( Telepathy::ConnectionStatusConnected == d->m_status ) )
    { return false; }

    startupInit();

    if ( !d->m_pInterface )
    { return false; }
    
    d->m_status = Telepathy::ConnectionStatusConnecting;
    QDBusPendingReply<> connection_connect_reply = d->m_pInterface->Connect();
    connection_connect_reply.waitForFinished();
   
    if ( !connection_connect_reply.isValid() )
    {
        QDBusError error = connection_connect_reply.error();
    
        qWarning() << "Connect: error type:" << error.type()
                << "Connect: error name:" << error.name()
                << "error message:" << error.message();
        
        d->m_status      = Telepathy::ConnectionStatusDisconnected;
        d->m_isValid     = false;

        return false;
    }

    d->m_isValid = true;
    return true;
}

bool Connection::requestDisconnect()
{
    if ( ! d->m_pInterface || ! isValid() || ( Telepathy::ConnectionStatusDisconnected == d->m_status ) )
    { return false; }
    
    QDBusPendingReply<> connection_disconnect_reply = d->m_pInterface->Disconnect();
    connection_disconnect_reply.waitForFinished();

    if ( !connection_disconnect_reply.isValid() )
    {
        QDBusError error = connection_disconnect_reply.error();

        qWarning() << "Connect: error type:" << error.type()
                << "Connect: error name:" << error.name()
                << "error message:" << error.message();
    }

    // Always expect that we are disconnected after this point!
    d->m_status = Telepathy::ConnectionStatusDisconnected;

    // Get rid of the contact and presence manager
    delete d->m_pContactManager;
    d->m_pContactManager = NULL;
    delete d->m_pPresenceManager;
    d->m_pPresenceManager = NULL;

    return d->m_status;
}

ContactManager* Connection::contactManager()
{
    if ( Telepathy::ConnectionStatusConnected != d->m_status )
    { return NULL; }
    if ( !d->m_pContactManager )
    { d->m_pContactManager = new ContactManager( d->m_pInterface, this ); }
    return d->m_pContactManager;
}

PresenceManager* Connection::presenceManager()
{
    return createManager<PresenceManager>( d->m_pPresenceManager, "Presence" );
}

CapabilitiesManager* Connection::capabilitiesManager()
{
    return createManager<CapabilitiesManager>(d->m_pCapabilitiesManager, "Capabilities" );
}

AvatarManager* Connection::avatarManager()
{
    return createManager<AvatarManager>( d->m_pAvatarManager, "Avatars" );
}


Account* Connection::account() const
{
    if ( !d->m_pAccount
        || !d->m_pAccount->isValid() )
    { return NULL; }
    
    return d->m_pAccount;
}        

QString Connection::handle() const
{
    return d->m_objectPath;
}

Telepathy::Client::ConnectionInterface* Connection::interface()
{
    return d->m_pInterface;
}

void Connection::slotStatusChanged( uint status, uint reason )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Connection::slotStatusChanged() Status:" << status;
#endif
    Telepathy::ConnectionStatus old_status = d->m_status;
    d->m_status = static_cast<Telepathy::ConnectionStatus>( status );
    d->m_reason = static_cast<Telepathy::ConnectionStatusReason>( reason );

    emit signalStatusChanged( this, d->m_status, old_status );

    if ( account() && account()->parameters().value( "register") == true )
    {
        QVariantMap parameter_map;
        parameter_map.insert( "register", false );
        account()->setParameters( parameter_map );
    }
}

void Connection::slotNewChannel(const QDBusObjectPath& objectPath, const QString& channelType, uint handleType, uint handle, bool suppressHandler)
{
    Q_UNUSED( suppressHandler );

    QString tmp_objectpath=objectPath.path();
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Connection:: slotNewChannel";
    qDebug() << "Connection:: slotNewChannel ObjectPath"<< tmp_objectpath;
    qDebug() << "Connection:: slotNewChannel ChannelType"<< channelType;
    qDebug() << "Connection:: slotNewChannel handleType"<< handleType <<"handle"<<handle;
#endif

    // Ignore signals if no contact manager is available..
    if ( !contactManager() )
    {
        // Q_ASSERT( contactManager() ); //FIXME: We have to understand why this can happen and how to avoid it!
        qWarning() << "Connection::slotNewChannel: Receiving signals but don't get a contact manager!";
        return;
    }
    
    if ( handleType == Telepathy::HandleTypeContact )
    {
        contactManager()->openTextChannel(handle,handleType,objectPath.path(),channelType);
    }
  
    if (channelType==QString("org.freedesktop.Telepathy.Channel.Type.ContactList"))
    {
        if (tmp_objectpath.contains("/subscribe"))
        { 
            contactManager()->openSubscribedContactsChannel(handle,objectPath,channelType);
        }
    }
    
    if (channelType==QString("org.freedesktop.Telepathy.Channel.Type.ContactList"))
    {
      if (tmp_objectpath.contains("/known"))
      {         
          contactManager()->openKnownContactsChannel(handle,objectPath,channelType);
      }
    }
    if (channelType==QString("org.freedesktop.Telepathy.Channel.Type.ContactList"))
    {
        if (tmp_objectpath.contains("/publish"))
        {
            contactManager()->openPublishContactsChannel(handle,objectPath,channelType);
        }
    }

    if (channelType==QString("org.freedesktop.Telepathy.Channel.Type.ContactList"))
    {
        if (tmp_objectpath.contains("/deny"))
        { 
            contactManager()->openDenyContactsChannel(handle,objectPath,channelType);
        }
    }

    if (channelType==QString("org.freedesktop.Telepathy.Channel.Type.StreamedMedia"))
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Connection::slotNewChannel(): Stream media channel opened!!";
#endif
        contactManager()->openStreamedMediaChannel( handle, handleType, objectPath.path(), channelType );
    }
}

QList<uint> Connection::RequestHandles( Telepathy::HandleType handletype, QStringList& handlestrings)
{
    Telepathy::registerTypes();
    return d->m_pInterface->RequestHandles( handletype,handlestrings);
}


bool Connection::managerSupported( const QString& managerName )
{
    QDBusPendingReply<QStringList> interfaces_reply = interface()->GetInterfaces();
    interfaces_reply.waitForFinished();

    if ( !interfaces_reply.isValid() )
    {
        QDBusError error = interfaces_reply.error();
        
        qWarning() << "Connection::GetInterfaces: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        return false;
    }

    foreach( const QString& interface, interfaces_reply.value() )
    {
        if ( interface.endsWith( managerName ) )
        {
            return true;
        }
    }
    return false;
}

void Connection::init( TpPrototype::Account* account )
{
    Telepathy::registerTypes();

    if ( !account )
    { return; }

    d->m_pAccount          = account;
    d->m_connectionManager = account->connectionManagerName();
    d->m_protocol          = account->protocolName();

    d->m_isValid = true;
}

// Called immediately before connection attempts
void Connection::startupInit()
{
    d->initConnectionDBUSService();

    if ( !d->m_isValid )
    { return; }
    
    Q_ASSERT( d->m_pInterface );


    connect( d->m_pInterface, SIGNAL( NewChannel( const QDBusObjectPath&, const QString&, uint, uint, bool ) ),
             this, SLOT( slotNewChannel( const QDBusObjectPath&, const QString&, uint, uint, bool ) ) );
    connect( d->m_pInterface, SIGNAL( StatusChanged( uint, uint ) ),
             this, SLOT( slotStatusChanged( uint, uint ) ) );
}
