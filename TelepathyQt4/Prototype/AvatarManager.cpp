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

#include "TelepathyQt4/Prototype/AvatarManager.h"

#include <QCoreApplication>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDebug>
#include <QMap>
#include <QPointer>

#include <TelepathyQt4/Connection>

#include <TelepathyQt4/Prototype/Account.h>
#include <TelepathyQt4/Prototype/AccountManager.h>
#include <TelepathyQt4/Prototype/ConnectionFacade.h>
#include <TelepathyQt4/Prototype/Connection.h>
#include <TelepathyQt4/Prototype/Contact.h>
#include <TelepathyQt4/Prototype/ContactManager.h>

// #define ENABLE_DEBUG_OUTPUT_

using namespace TpPrototype;

class TpPrototype::AvatarManagerPrivate
{
public:
    AvatarManagerPrivate( Connection* connection,
                          Telepathy::Client::ConnectionInterface* interface )
    { init( connection, interface ); }

    Telepathy::Client::ConnectionInterface* m_pConnectionInterface;
    Telepathy::Client::ConnectionInterfaceAvatarsInterface* m_pAvatarsInterface;
    QPointer<Connection> m_pConnection;
    
    bool m_isValid;
    void init( Connection* connection,
               Telepathy::Client::ConnectionInterface* interface )
    {
        m_pConnection            = connection;
        m_pConnectionInterface   = interface;
        m_pAvatarsInterface      = NULL;
        m_isValid                = true;
    }

};

AvatarManager::AvatarManager( Connection* connection,
                              Telepathy::Client::ConnectionInterface* interface,
                              QObject* parent ):
    QObject( parent ),
    d( new AvatarManagerPrivate( connection, interface ) )
{
    init( connection, interface );
}

AvatarManager::~AvatarManager()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "D'tor AvatarManager (" << this << ")";
#endif
    delete d;
}

bool AvatarManager::isValid()
{
    return d->m_isValid;
}



TpPrototype::Connection* AvatarManager::connection()
{
    return d->m_pConnection;
}

bool AvatarManager::setAvatar( const TpPrototype::AvatarManager::Avatar& newValue  )
{
    QDBusPendingReply<QString> set_avatar_reply = d->m_pAvatarsInterface->SetAvatar( newValue.avatar, newValue.mimeType );
    set_avatar_reply.waitForFinished();
    
    if ( !set_avatar_reply.isValid() )
    {
        QDBusError error = set_avatar_reply.error();
    
        qWarning() << "AvatarManager::setAvatar: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return false;
    }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AvatarManager::setAvatar: token " << set_avatar_reply.value();
#endif
    
    return true;
}

void AvatarManager::requestAvatar()
{
    uint self_handle = TpPrototype::ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pConnectionInterface );
    QList<uint> contact_ids;
    contact_ids << self_handle;
    
    QDBusPendingReply<> avatars_reply = d->m_pAvatarsInterface->RequestAvatars( contact_ids );
    
    // slotAvatarRetrieved is called after this point.
}

TpPrototype::AvatarManager::AvatarRequirements AvatarManager::avatarRequirements()
{
    Q_ASSERT( d->m_pAvatarsInterface );

    TpPrototype::AvatarManager::AvatarRequirements return_data;
    return_data.isValid = false;

    QDBusPendingReply<QStringList, ushort, ushort, ushort, ushort, uint> requirements_reply = d->m_pAvatarsInterface->GetAvatarRequirements();
    requirements_reply.waitForFinished();

    if ( !requirements_reply.isValid() )
    {
        QDBusError error = requirements_reply.error();
    
        qWarning() << "AvatarManager::GetAvatarRequirements: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        d->m_isValid = false;
        return return_data;
    }

    TpPrototype::AvatarManager::AvatarRequirements ret_requirement;
    ret_requirement.mimeTypes     = qvariant_cast<QStringList>( requirements_reply.argumentAt( 0 ) );
    ret_requirement.minimumWidth  = qvariant_cast<ushort>(requirements_reply.argumentAt( 1 ) );
    ret_requirement.minimumHeight = qvariant_cast<ushort>(requirements_reply.argumentAt( 2 ) );
    ret_requirement.maximumWidth  = qvariant_cast<ushort>(requirements_reply.argumentAt( 3 ) );
    ret_requirement.maximumHeight = qvariant_cast<ushort>(requirements_reply.argumentAt( 4 ) );
    ret_requirement.maxSize       = qvariant_cast<uint>(requirements_reply.argumentAt( 5 ) );
    return_data.isValid = true;
    
    return ret_requirement;
}

void AvatarManager::avatarForContactList( const QList<QPointer<Contact> >& contacts )
{
    Q_ASSERT( d->m_pAvatarsInterface );
    Telepathy::UIntList contact_ids;
    foreach( Contact* contact, contacts )
    {
        if ( !contact )
        { continue; }
        contact_ids.append( contact->telepathyHandle() );
    }

    QDBusPendingReply<> avatars_reply = d->m_pAvatarsInterface->RequestAvatars( contact_ids );
   
    // slotAvatarRetrieved is called after this point.
}


void AvatarManager::slotAvatarUpdated( uint contactHandle, const QString& newAvatarToken )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AvatarManager::slotAvatarUpdated(" << contactHandle << "," << newAvatarToken << ")";
#endif
    if ( !d->m_pConnection )
    {
        qWarning() << "AvatarManager::slotAvatarUpdated(): Received a Avatar changed signal but no connection object exists!";
        return;
    }

    Q_ASSERT( d->m_pAvatarsInterface );
    Q_ASSERT( d->m_pAvatarsInterface->isValid() );
    Q_ASSERT( d->m_pConnection );
    Q_ASSERT( d->m_pConnection->contactManager() );

    if ( !d->m_pConnection
         || !d->m_pConnection->contactManager() )
    {
        qWarning() << "AvatarManager::slotAvatarUpdated(): Unable to request contact manager or connection is not valid!";
        return;
    }

    uint self_handle = ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pConnectionInterface );

    if ( self_handle == contactHandle )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "AvatarManager::slotAvatarUpdated(): Ignored that my avatar is updated.";
#endif

        //emit void signalOwnAvatarChanged();
    }
    else
    {
        foreach( TpPrototype::Contact* contact, d->m_pConnection->contactManager()->contactList() )
        {
            if ( ( contact->telepathyHandle() == contactHandle )
                  && ( contact->avatar().token != newAvatarToken ) )
            {
                // Request avatar for this contact.
                avatarForContactList( QList<QPointer<Contact> >() << QPointer<Contact>( contact ) );
            }
        }
    }
}

// Called after avatarForContactList() is called.
void AvatarManager::slotAvatarRetrieved( uint contactHandle, const QString& token, const QByteArray& avatar, const QString& type )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "slotAvatarRetrieved: Handle: " << contactHandle << "Token:" << token << "Type: " << type;
#endif
    
    TpPrototype::AvatarManager::Avatar new_avatar;
    new_avatar.avatar   = avatar;
    new_avatar.token    = token;
    new_avatar.mimeType = type;

    uint self_handle = ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pConnectionInterface );
    if ( self_handle == contactHandle )
    {
        emit signalOwnAvatarChanged( new_avatar );
        return;
    }   

    foreach( TpPrototype::Contact* contact, d->m_pConnection->contactManager()->contactList() )
    {
        // Find contact for handle and check whether the token has changed. If not, the avatar was not changed.
        if ( ( contact->telepathyHandle() == contactHandle )
               && ( contact->avatar().token != token ) )
        {
            contact->setAvatar( new_avatar );
            emit signalAvatarChanged( contact );
        }
    }   
}

void AvatarManager::init( Connection* connection,
                          Telepathy::Client::ConnectionInterface* interface )
{
    Q_ASSERT( interface );

    if ( !interface || !connection )
    {
        d->m_isValid = false;
        return;
    }
    
    Telepathy::registerTypes();
    qRegisterMetaType<TpPrototype::AvatarManager::Avatar>();
    
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
    
    QString avatar_interface_name;
    bool found_avatar_support = false;
    foreach( const QString& interface, interfaces_reply.value() )
    {
        if ( interface.endsWith( ".Avatars" ) )
        {
            found_avatar_support = true;
            avatar_interface_name = interface;
            break;
        }     
    }
    if ( !found_avatar_support )
    {
        d->m_isValid = false;
        qWarning( "AvatarManager::init(): Connection Manager does not support the Interface \"Avatars\". Other interfaces are not supported!" );
        return;
    }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Interface Name: " << avatar_interface_name;
#endif
    d->m_pAvatarsInterface = new Telepathy::Client::ConnectionInterfaceAvatarsInterface( d->m_pConnectionInterface->service(), d->m_pConnectionInterface->path(), this );
    
    connect( d->m_pAvatarsInterface, SIGNAL( AvatarUpdated(uint, const QString&) ),
             this, SLOT( slotAvatarUpdated(uint, const QString&) ) );
    connect( d->m_pAvatarsInterface, SIGNAL( AvatarRetrieved(uint , const QString& , const QByteArray& , const QString& ) ),
             this, SLOT( slotAvatarRetrieved( uint , const QString& , const QByteArray& , const QString& ) ) );
   
}

#include "_gen/AvatarManager.h.moc"
