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

#include "TelepathyQt4/Prototype/Contact.h"

#include <QDebug>
#include <QMetaProperty>

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Constants>

#include <TelepathyQt4/Prototype/ChatChannel.h>
#include <TelepathyQt4/Prototype/ContactManager.h>
#include <TelepathyQt4/Prototype/StreamedMediaChannel.h>

using namespace TpPrototype;

class TpPrototype::ContactPrivate
{
public:
    ContactPrivate( const uint & handle, const QString & name, Contact::ContactTypes type,
                    Telepathy::Client::ConnectionInterface* connectionInterface, TpPrototype::ContactManager* contactManager )
    { init(handle, name, type, connectionInterface, contactManager); }
    ~ContactPrivate()
    {}
    uint                                      m_handle;
    bool                                      m_isValid;
    bool                                      m_isPresenceInitialized;
    Contact::ContactTypes                     m_type;
    QString                                   m_name;
    uint                                      m_presenceType;
    QString                                   m_presenceStatus;
    QString                                   m_presenceMessage;
    Telepathy::ContactCapabilityList          m_capabilityList;
    TpPrototype::AvatarManager::Avatar          m_avatar;
    Telepathy::Client::ConnectionInterface*   m_pConnectionInterface;
    QPointer<TpPrototype::ChatChannel>          m_pChatChannel;
    QPointer<TpPrototype::StreamedMediaChannel> m_pStreamedMediaChannel;
    QPointer<TpPrototype::ContactManager>       m_pContactManager;
    
private:
    void init( uint handle, const QString & name, Contact::ContactTypes type,
               Telepathy::Client::ConnectionInterface* connectionInterface, TpPrototype::ContactManager* contactManager )
    {
        Q_ASSERT( connectionInterface );
        Q_ASSERT( contactManager );
        
        m_handle                  = handle;
        m_name                    = name;
        m_type                    = type;
        m_presenceType            = 0;
        m_presenceStatus          = "unknown";
        m_presenceMessage         = "";
        m_pChatChannel            = NULL;
        m_pStreamedMediaChannel   = NULL;
        m_pConnectionInterface    = connectionInterface;
        m_pContactManager         = contactManager;
        m_isPresenceInitialized   = false;
        
        if ( 0 == m_handle )
            m_isValid = false;
        else
            m_isValid = true;
    }
};

uint Contact::telepathyHandle() const
{ return d->m_handle; }

uint Contact::telepathyHandleType() const
{ return Telepathy::HandleTypeContact; }

QString Contact::name() const
{ return d->m_name; }

Contact::ContactTypes Contact::type() const
{ return d->m_type; }

void Contact::setType( ContactTypes type )
{
    d->m_type = type;
}

bool Contact::isValid() const
{ return d->m_isValid; }

void Contact::setPresenceType( uint _presenceType)
{ d->m_presenceType=_presenceType; }

uint Contact::presenceType()
{
    d->m_isPresenceInitialized = true;
    return d->m_presenceType;
}

void Contact::setPresenceStatus( QString _presenceStatus)
{
    d->m_isPresenceInitialized = true;
    d->m_presenceStatus=_presenceStatus;
}

QString Contact::presenceStatus()
{ return d->m_presenceStatus; }

bool Contact::isPresenceStateAvailable()
{ return d->m_isPresenceInitialized; }

void Contact::setPresenceMessage( QString _presenceMessage)
{ d->m_presenceMessage=_presenceMessage; }

QString Contact::presenceMessage()
{ return d->m_presenceMessage; }

void Contact::setCapabilities( const Telepathy::ContactCapabilityList& capabilityList )
{ d->m_capabilityList = capabilityList; }

Telepathy::ContactCapabilityList Contact::capabilities() const
{ return d->m_capabilityList; }

void Contact::setAvatar( const TpPrototype::AvatarManager::Avatar& avatar )
{
    d->m_avatar = avatar;
}

Telepathy::Client::ConnectionInterface* Contact::interface()
{
    return d->m_pConnectionInterface;
}

TpPrototype::AvatarManager::Avatar Contact::avatar() const
{
    return d->m_avatar;
}

ChatChannel* Contact::chatChannel()
{
    if ( !d->m_pChatChannel )
    {
        d->m_pChatChannel = new ChatChannel( this, d->m_pConnectionInterface, this );
        Q_ASSERT( d->m_pChatChannel->isValid() );
    }
    return d->m_pChatChannel;
}

TpPrototype::StreamedMediaChannel* Contact::streamedMediaChannel()
{
    if ( !d->m_pStreamedMediaChannel )
    {
        d->m_pStreamedMediaChannel = new StreamedMediaChannel( this, d->m_pConnectionInterface, this );
        Q_ASSERT( d->m_pStreamedMediaChannel->isValid() );
    }
    return d->m_pStreamedMediaChannel;
    
}

TpPrototype::ContactManager* Contact::contactManager()
{
    return d->m_pContactManager;
}


Contact::Contact( const uint & handle, const QString & url, ContactTypes type,
                  Telepathy::Client::ConnectionInterface* connectionInterface, TpPrototype::ContactManager* contactManager ) :
    QObject( contactManager ),
    d( new ContactPrivate( handle, url, type, connectionInterface, contactManager ) )
{   
}

Contact::~Contact()
{ delete d; }
