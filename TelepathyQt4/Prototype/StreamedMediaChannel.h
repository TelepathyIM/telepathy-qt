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

#ifndef TelepathyQt4_Prototype_StreamedMediaChannel_H_
#define TelepathyQt4_Prototype_StreamedMediaChannel_H_

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Channel>

#include <QObject>
#include <QVariantMap>
#include <QPointer>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif


namespace TpPrototype {

class StreamedMediaChannelPrivate;
class Contact;
class Account;

/**
 * @ingroup qt_connection
 * StreamedMedia Channel for VoIP and Video Over IP.
 * <b>Hint:</b> You have to set the right capabilities with CapabilitiesManager::setCapabilities() before VoIP or Video over IP will work!
 * @see CapabiltiesManager
 */
class StreamedMediaChannel : public QObject
{
    Q_OBJECT
public:
    /**
     * Validity check.
     * Do not access any functions if this account is invalid.
     */
    bool isValid() const;
    
    /**
     * Destructor.
     * Deleting this object forces to drop all channels.
     */
    ~StreamedMediaChannel();

    /**
     * Accept the incoming media stream (call).
     * This function must be called to accept the incoming media stream.
     * @return Returns true if command was successful.
     */
    bool acceptIncomingStream();

    /**
     * Reject the incoming media stream (call).
     * This function should be called if an incoming call should be rejected.
     * @return Returns true if command was successful.
     */
    bool rejectIncomingStream();

    /**
     * Request outgoing media channel (call).
     * Ask remote contact to start a media channel.
     * @todo Describe what happens after this call ..
     * @param types The stream types that should be opened for this channel
     * @return Returns true if request call was successful.
     */
    bool requestChannel( QList<Telepathy::MediaStreamType> types );

    /**
     * Add contacts to the group.
     * Multiple contacts may sharing the same stream. Use this function to invite a contact to this group. The contact will
     * be added to the group after he accepted the invitation.
     * @return Returns true if command was successful.
     */
    bool addContactsToGroup( QList<QPointer<TpPrototype::Contact> > contacts );

    /**
     * Returns a list of contacts that are waiting locally for accaptance.
     * A Contact that is local pending has requested membership of the channel, but the local user of the framework must
     * accept their request before they may join.
     * @return Returns the list of contacts that are waiting for acceptance.
     */
    QList<QPointer<TpPrototype::Contact> > localPendingContacts();

    /**
     * Returns the list of members in the group.
     * The StreamedMediaChannel contains a group of members that currently part of the group.
     * @return Returns the list of contacts that are currently group members.
     */
    QList<QPointer<TpPrototype::Contact> > members();

signals:
    /**
     * Incoming channel detected.
     * This signal is emitted when a valid incoming channel was detected and all needed interfaces were established successfully.
     * Usually it does not make sense to connect to this signal, because this object might be created or removed without any prior notification. Thus,
     * there is no chance to connect before this signal is emitted.<br>
     * Use ContactManager::signalStreamedMediaChannelOpenedForContact() instead to get informed about new channels!
     * @param contact The contact that contains this StreamedMediaChannel object.
     * @see ContactManager
     */
    void signalIncomingChannel( TpPrototype::Contact* contact );

    /**
     * A stream was added.
     * If a new stream was added to this media channel.
     * @param contact The contact that belongs to this media channel
     * @param streamId The id of the stream.
     * @param streamType The type of the stream.
     */
    void signalStreamAdded( TpPrototype::Contact* contact, uint streamId, Telepathy::MediaStreamType streamType );
    
    /**
     * Stream was removed.
     * This signal is emmitted when a media stream was removed. This may happen on network errors, if the remote contact
     * rejected the call or if the established connection was closed.
     * @param streamId The id of the stream.
     */
    void signalStreamRemoved( uint streamId );

    /**
     * A remove contact was added.
     * This signal has different meaning with regard to the context:
     * <ul><li>Outgoing call: The remote contact accapted the call</li>
     *     <li><i>Please add further situations</i></li>
     * </ul>
     */
    void signalContactAdded( TpPrototype::Contact* acceptingContact );

    /**
     * Call was canceled by remote contact.
     * This signal has different meaning with regard to the context:
     * <ul><li>Ongoing chat: The remote contact disconnected</li>
     *     <li><i>Please add further situations</i></li>
     * </ul>
     */
    void signalContactRemoved( TpPrototype::Contact* rejectingContact );
protected:
    /**
     * Constructor.
     * Use Contact::streamedMediaChannel() to obtain an object of StreamedMediaChannel.
     */
    StreamedMediaChannel( Contact* contact, Telepathy::Client::ConnectionInterface* connectionInterface , QObject* parent = NULL );

    /**
     * Request a new streamed media channel.
     * This functions needs to be called if a new streamed media channel D-BUS object should be established.
     */
    void requestStreamedMediaChannel( uint handle );
    
    /**
     * Called internally to notify that a new media channel object was stablished on D-BUS.
     */
    void openStreamedMediaChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelType );

    /** Connect slots to channel signals */
    void connectSignals();

    bool addMembers( QList<uint> handles );
    
    bool removeMembers( QList<uint> handles );

protected slots:
    /**
     *  Represents the signal "slotStreamAdded" on the remote object.
     */
    void slotStreamAdded(uint streamID, uint contactHandle, uint streamType);

    /**
     * Represents the signal "StreamDirectionChanged" on the remote object.
     */
    void slotStreamDirectionChanged(uint streamID, uint streamDirection, uint pendingFlags);

    /**
     * Represents the signal "StreamError" on the remote object.
     */
    void slotStreamError(uint streamID, uint errorCode, const QString& message);

    /**
     * Represents the signal "StreamRemoved" on the remote object.
     */
    void slotStreamRemoved(uint streamID);

    /**
     * Represents the signal "StreamStateChanged" on the remote object.
     */
    void slotStreamStateChanged(uint streamID, uint streamState);

    /**
     * Represents the signal "MembersChanged" on the remote object.
     */
    void slotMembersChanged(const QString& message,
                            const Telepathy::UIntList& added,
                            const Telepathy::UIntList& removed,
                            const Telepathy::UIntList& localPending,
                            const Telepathy::UIntList& remotePending,
                            uint actor,
                            uint reason);
private:
    StreamedMediaChannelPrivate * const d;
    friend class ContactManager;
    friend class Contact;
};

} // namespace

#endif // Header guard
