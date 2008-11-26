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

#include "TelepathyQt4/Prototype/ChatChannel.h"

#include <QDebug>
#include <QMetaProperty>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Channel>

#include <TelepathyQt4/Prototype/Contact.h>

using namespace TpPrototype;

class TpPrototype::ChatChannelPrivate
{
public:
    ChatChannelPrivate()
    { init(); }
    
    QPointer<TpPrototype::Contact>                          m_pContact;
    Telepathy::Client::ChannelTypeTextInterface*          m_pTextChannel;
    QPointer<Telepathy::Client::ConnectionInterface>      m_pConnectionInterface;

    bool m_isValid;
    bool m_areSignalsConnected;

private:
    void init()
    {
        m_pContact              = NULL;
        m_pConnectionInterface  = NULL;
        m_pTextChannel          = NULL;
        m_isValid               = true;
        m_areSignalsConnected   = false;
    }
};

ChatChannel::ChatChannel( Contact* contact, Telepathy::Client::ConnectionInterface* connectionInterface, QObject* parent ):
        QObject( parent ),
        d(new ChatChannelPrivate())
{
    Telepathy::registerTypes();

    d->m_pContact = contact;
    d->m_pConnectionInterface = connectionInterface;

    requestTextChannel( d->m_pContact->telepathyHandle() );
}

ChatChannel::~ChatChannel()
        { delete d; }

bool ChatChannel::isValid() const
{ return d->m_isValid; }


void ChatChannel::sendTextMessage( const QString& text )
{

    if ( d->m_pTextChannel == NULL )
    {
        requestTextChannel( d->m_pContact->telepathyHandle() );
    }

    Q_ASSERT( d->m_pTextChannel );
    if ( !d->m_pTextChannel )
    {
        qWarning() << "ChatChannel::sendTextMessage: Action ignored due to missing text channel!";
    }
         
    d->m_pTextChannel->Send( 0, text ); // TODO: Remove this magic number!

}

void ChatChannel::pendingTextMessages()
{
    if ( d->m_pTextChannel == NULL )
    {
        requestTextChannel( d->m_pContact->telepathyHandle() );
    }

    Q_ASSERT( d->m_pTextChannel );
    if ( !d->m_pTextChannel )
    {
        qWarning() << "ChatChannel::pendingTextMessages: Action ignored due to missing text channel!";
    }
    
    QDBusPendingReply<Telepathy::PendingTextMessageList> reply= d->m_pTextChannel->ListPendingMessages( true );
    const Telepathy::PendingTextMessageList chatMessages = reply.value();
    Telepathy::PendingTextMessage messageshandle;
    for ( int i=0; i < chatMessages.size(); i++ )
    {
        messageshandle = chatMessages.at(i);
        slotReceivedText( messageshandle.identifier,
                          messageshandle.unixTimestamp,
                          messageshandle.sender,
                          messageshandle.messageType,
                          messageshandle.flags,
                          messageshandle.text );
    }
}

// Called if a new text channel shall be established.
void ChatChannel::requestTextChannel( uint handle )
{
    QDBusPendingReply<QDBusObjectPath> reply0 =
            d->m_pConnectionInterface->RequestChannel( "org.freedesktop.Telepathy.Channel.Type.Text",
                                             Telepathy::HandleTypeContact, handle, true );
    reply0.waitForFinished();
    if (!reply0.isValid())
    {
        QDBusError error = reply0.error();
        qWarning() << "Get ContactListChannel: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();
        d->m_isValid = false;
        return;
    }

    QDBusObjectPath channel_path=reply0.value();
    openTextChannel( handle, 1,channel_path.path(), "org.freedesktop.Telepathy.Channel.Type.Text" ); // TODO: Remove magic number
}

// Called if a new text channel was notified by the connection channel
void ChatChannel::openTextChannel(uint handle, uint handleType, const QString& channel_path, const QString& channelType)
{
    QString channel_service_name = d->m_pConnectionInterface->service();
    qDebug() << "ContactManager Channel Services Name" << channel_service_name;
    qDebug() << "ContactManager Channel Path" << channel_path;
    // This channel may never be closed!
    d->m_pTextChannel = new Telepathy::Client::ChannelTypeTextInterface( channel_service_name,
                                                                     channel_path,
                                                                     this );
    if (!d->m_pTextChannel->isValid())
    {
        qDebug() << "Failed to connect channel interface class to D-Bus object.";
        delete d->m_pTextChannel;
        d->m_pTextChannel = NULL;
        d->m_isValid = false;
        return;
    }
    else
    {
        if ( d->m_areSignalsConnected )
        { return; }
         
        d->m_areSignalsConnected = true;
        
        qDebug() << "Success WE got a valid Text channel";
        //ChatChannel * pChatChannel=new ChatChannel();
        //pChatChannel->setChannel(d->m_groupTextChannel);
        //pChatChannel->setContact(channel_contact);
        QString messagesender;
        messagesender = d->m_pContact->name();

        qDebug() << "*************************************";
        
        connect(d->m_pTextChannel, SIGNAL(Received(uint , uint , uint , uint , uint , const QString& )),
                this, SLOT(slotReceivedText(uint , uint , uint , uint , uint , const QString& )));
        connect(d->m_pTextChannel, SIGNAL(Sent(uint , uint , const QString& )),
                this, SLOT(slotSentText(uint , uint , const QString& )));
        connect(d->m_pTextChannel, SIGNAL(LostMessage()),
                this, SLOT(slotLostMessage()));
        connect(d->m_pTextChannel, SIGNAL(SendError(uint , uint , uint , const QString& )),
                this, SLOT(slotSendError(uint , uint , uint , const QString& )));

    /*  QDBusPendingReply<Telepathy::PendingTextMessageList>  reply= d->m_groupTextChannel->ListPendingMessages(false);
                                const Telepathy::PendingTextMessageList chatMessages=reply.value();
                                Telepathy::PendingTextMessage messageshandle;
                                for (int i=0; i<chatMessages.size(); i++)
                                {
                                messageshandle= chatMessages.at(i);
                                pChatChannel->slotReceivedText( messageshandle.identifier, messageshandle.unixTimestamp, messageshandle.sender, messageshandle.messageType, messageshandle.flags, messageshandle.text);

                }     */
    }
}


void ChatChannel::slotReceivedText( uint id, uint timestamp, uint sender, uint type, uint flags, const QString& text )
{
    qDebug() << "ChatChannel: Reveived text:" << text;
    QList<uint> message_ids;
    message_ids.append( id );
    d->m_pTextChannel->AcknowledgePendingMessages( message_ids );
    emit signalTextMessageReceived( this , timestamp, type, flags, text );
}

void ChatChannel::slotSentText(uint timestamp, uint type, const QString& text )
{
    qDebug() << "ChatChannel: Sent text:" << text;
    emit signalTextMessageSent( this, timestamp, type, text );
}

void ChatChannel::slotLostMessage()
{
}

void ChatChannel::slotSendError(uint error, uint timestamp, uint type, const QString& text )
{
}
