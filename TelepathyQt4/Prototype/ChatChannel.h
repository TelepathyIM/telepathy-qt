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

#ifndef TelepathyQt4_Prototype_ChatChannel_H_
#define TelepathyQt4_Prototype_ChatChannel_H_

#include <QObject>
#include <QPointer>
#include <QVariantMap>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif

namespace Telepathy {
namespace Client{
    class ConnectionInterface;
}
}

namespace TpPrototype {

class ChatChannelPrivate;
class Contact;
class Account;

/**
 * @ingroup qt_connection
 * Chat Channel.
 * This class provides the interface to send or receive text messages.
 */
class ChatChannel : public QObject
{
    Q_OBJECT
public:
    /**
     * Validity check.
     * Do not access any functions if this account is invalid.
     */
    bool isValid() const;
    
    /**
     * Send a text message.
     * This function sends a text message to the contact that belongs to this channel.
     */
    void sendTextMessage( const QString& text );

    /**
     * Fetch pending text messages.
     * Force to refetch all messages that were sent while the account was offline.
     * A signal signalTextMessageReceived() will be emitted for every message.
     * @see signalTextMessageReceived()
     */
    void pendingTextMessages();
        
    /**
     * Destructor.
     * Deleting this object forces to drop all channels.
     */
    ~ChatChannel();

signals:
    /**
     * A new text message was received.
     * This signal is emmitted right after receiving a new test message.
     */
    void signalTextMessageReceived( TpPrototype::ChatChannel* chatchannel, uint timestamp, uint type, uint flags, const QString& text );
    
    /**
     * A text message was sent.
     * This signal is emmitted right after test message was delivered.
     */
    void signalTextMessageSent( TpPrototype::ChatChannel* chatchannel, uint timestamp, uint type, const QString& text );

protected:
   /**
    * Constructor.
    * Use Contact::chatChannel() to obtain an object of ChatChannel.
    */
    ChatChannel( Contact* contact, Telepathy::Client::ConnectionInterface* connectionInterface , QObject* parent = NULL );
    
    void requestTextChannel(uint handle);
    void openTextChannel(uint handle, uint handleType, const QString& channelPath, const QString& channelType );

protected slots:
        void slotSentText(uint timestamp, uint type, const QString& text );
        void slotLostMessage();
        void slotSendError(uint error, uint timestamp, uint type, const QString& text );
        void slotReceivedText(uint ID, uint timestamp, uint sender, uint type, uint flags, const QString& text);
private:
    ChatChannelPrivate * const d;
    friend class ContactManager;
    friend class Contact;
};

} // namespace

#endif // Header guard
