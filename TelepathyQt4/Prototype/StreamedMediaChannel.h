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
 * The StreamedMedia channel object provides a simple API to manage the signalling of streamed media. The real
 * encoding/decoding of the data is done by the telepathy stream engine. New/incoming channels are handed over to
 * this stream engine internally.<br>
 * The channel contains multiple streams each representing its own media (video, audio, ..) and provides
 * its own group channel which can be used for conferences.<br>
 * <br>
 * <b>Important:</b><br>
 * This implementation currently just supports the telepathy stream engine as backend (see http://telepathy.freedesktop.org/wiki/Streamed_Media).<br>
 * <br>
 * <b>Example opening a VoIP communication to a contact:</b><br>
 * A valid streamed media channel object is provided by the remote contact object:
 * @code
 * TpPrototype::StreamedMediaChannel* media_channel = contact->streamedMediaChannel();
 * if ( !media_channel )
 * { return; }
 * @endcode
 * Request a channel (in this case an audio channel):
 * @code
 * media_channel->requestChannel( QList<Telepathy::MediaStreamType>() << Telepathy::MediaStreamTypeAudio );
 * @endcode
 * Whether the call was accepted or rejected is signalled by the following signals:
 * @code
 * connect( media_channel, SIGNAL( signalContactAdded( TpPrototype::Contact* ) ),
 *         this, SLOT( slotStreamContactAdded(  TpPrototype::Contact* ) ) );
 * connect( media_channel, SIGNAL( signalContactRemoved( TpPrototype::Contact* ) ),
 *         this, SLOT( slotStreamContactRemoved( TpPrototype::Contact* ) ) );
 * @endcode
 * <b>Example receiving a VoIP communication from a contact:</b><br>
 * First you have to be informed about incoming calls. The ContactManager provides the signal:
 * @code
 * connect( m_pContactManager, SIGNAL( signalStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ),
 *          this, SLOT( slotStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ) );
 * @endcode
 * Then you have to accept or reject the incoming call:
 * @code
 * TpPrototype::StreamedMediaChannel* streamed_channel = contact->streamedMediaChannel();
 * streamed_channel->acceptIncomingStream();
 * @endcode
 * or
 * @code
 * streamed_channel->rejectIncomingStream();
 * @endcode
 * <b>Hint:</b><br>
 * Before using this class set the capabilities with CapabilitiesManager::setCapabilities() according your requirements!
 * @see CapabiltiesManager
 * @see ChatChannel
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

    bool requestStreams( TpPrototype::Contact* contact, QList<Telepathy::MediaStreamType> types );
    
    /**
     * Add contacts to the group.
     * Multiple contacts may sharing the same stream. Use this function to invite a contact to this group. The contact will
     * be added to the group after he accepted the invitation.
     * @return Returns true if command was successful.
     */
    bool addContactsToGroup( QList<QPointer<TpPrototype::Contact> > contacts );

    /**
     * Remove contacts from the group.
     * Multiple contacts may sharing the same stream. Use this function to remove a contact from this group.
     * @return Returns true if command was successful.
     */
    bool removeContactsFromGroup( QList<QPointer<TpPrototype::Contact> > contacts );
    
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

    /**
     * Remove streams.
     * Use this function if a certain stream in a channel should be removed. This might be necessary if
     * a video stream should be canceled without terminating the audio channel.
     */
     bool removeStreams( const QList<uint>& streamIds );

    /**
     * Request streams for the current channel.
     *
     * Request that streams be established to exchange the given types of
     * media with the given member. In general this will try and establish a
     * bidirectional stream, but on some protocols it may not be possible to
     * indicate to the peer that you would like to receive media, so a
     * send-only stream will be created initially. In the cases where the
     * stream requires remote agreement (eg you wish to receive media from
     * them), the StreamDirectionChanged signal will be emitted with the
     * MEDIA_STREAM_PENDING_REMOTE_SEND flag set, and the signal emitted again
     * with the flag cleared when the remote end has replied.
     *
     * @param types An array of stream types (values of MediaStreamType)
     * @return An array of structs (in the same order as the given stream types)
     *     containing:
     *     <ul>
     *       <li>the stream identifier</li>
     *       <li>the contact handle who the stream is with (or 0 if the stream
     *         represents more than a single member)</li>
     *       <li>the type of the stream</li>
     *       <li>the current stream state</li>
     *       <li>the current direction of the stream</li>
     *       <li>the current pending send flags</li>
     *     </ul>
     *     The returned list is empty if an error occured.
     * 
     */
     Telepathy::MediaStreamInfoList requestStreams( QList<Telepathy::MediaStreamType> types );
     
    /**
      * Begins a call to the D-Bus method "ListStreams" on the remote object.
      *
      * Returns an array of structs representing the streams currently active
      * within this channel. Each stream is identified by an unsigned integer
      * which is unique for each stream within the channel.
      *
      * @return
      *   An array of structs containing:
      *     <ul>
      *     <li>the stream identifier</li>
      *     <li>the contact handle who the stream is with (or 0 if the stream
      *       represents more than a single member)</li>
      *     <li>the type of the stream</li>
      *     <li>the current stream state</li>
      *     <li>the current direction of the stream</li>
      *     <li>the current pending send flags</li>
      *     </ul>
      *  The returned list is empty if an error occured.
     */
     Telepathy::MediaStreamInfoList listStreams();

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
     * @param channel The channel where the stream was added to.
     * @param streamId The id of the stream.
     * @param streamType The type of the stream.
     */
    void signalStreamAdded( TpPrototype::StreamedMediaChannel* channel, uint streamId, Telepathy::MediaStreamType streamType );
    
    /**
     * Stream was removed.
     * This signal is emmitted when a media stream was removed. This may happen on network errors, if the remote contact
     * rejected/removed a stream or if the established connection was closed.
     * @param streamId The id of the stream.
     */
    void signalStreamRemoved( TpPrototype::StreamedMediaChannel* channel, uint streamId );

    /**
     * A remote contact was added to the group.
     * This signal has different meaning with regard to the context:
     * <ul><li>Outgoing call: The remote contact accapted the call. <i>addedContact</i> contains the remote contact in this case.</li>
     * </ul>
     * @param addedContact The contact that was removed from the group.
     * @todo: Are there any additional situations?
     */
    void signalContactAdded( TpPrototype::StreamedMediaChannel* channel, TpPrototype::Contact* addedContact );

    /**
     * A contact was removed from the channel.
     * This signal has different meaning with regard to the context:
     * <ul><li>Ongoing chat: The remote contact disconnected or closed the chat.</li>
     *     <li>Initiating chat: The remote contact rejected the incoming call.</li>
     *     <li>Group Chat: A contact was removed from the group.</li>
     * </ul>
     * @param removedContact The contact that was removed from the group.
     */
    void signalContactRemoved( TpPrototype::StreamedMediaChannel* channel, TpPrototype::Contact* removedContact );

    /**
     * A stream changed its state
     * @param streamId The ID of the stream as returned by listStreams()
     * @param streamState The state as provided by Telepathy::MediaStreamInfo
     */
    void signalStreamStateChanged( TpPrototype::StreamedMediaChannel* channel, uint streamID, Telepathy::MediaStreamState streamState );

    /**
     * Emitted when the direction or pending flags of a stream are changed.
     * If the MEDIA_STREAM_PENDING_LOCAL_SEND flag is set, the remote user has
     * requested that we begin sending on this stream. RequestStreamDirection
     * should be called to indicate whether or not this change is acceptable.
     *
     * @param streamID The stream identifier (as defined in ListStreams)
     * @param streamDirection The new stream direction (as defined in listStreams)
     * @param pendingFlags The new pending send flags (as defined in listStreams)
     */
    void signalStreamDirectionChanged( TpPrototype::StreamedMediaChannel* channel, uint streamID, uint streamDirection, uint pendingFlags );

    /**
     * Stream error.
     * Emitted when a stream encounters an error.
     * @param streamID The stream identifier (as defined in ListStreams)
     * @param errorCode A stream error number, one of the values of MediaStreamError
     * @param message A string describing the error (for debugging purposes only)
     */
    void signalStreamError( TpPrototype::StreamedMediaChannel* channel, uint streamID, uint errorCode, const QString& message );

    /**
     * Local invitation accepted.
     * This signal is accepted if the local contact was added into the group channel
     */
    void signalLocalInvitationAccepted( TpPrototype::StreamedMediaChannel* channel );


public slots:
    /**
     * Set output volume.
     * @param streamId The id of the stream as emitted by signalStreamAdded()
     * @param volume The volume (range?)
     */
     void slotSetOutputVolume( uint streamId, uint volume );

    /**
     * Mute input.
     * @param streamId The id of the stream as emitted by signalStreamAdded()
     * @param muteState Muted on true.
     * @todo Check whether it is muted if muteState == true and correct API doc if needed.
     */
     void slotMuteInput( uint streamId, bool muteState );

    /**
     * Mute output.
     * @param streamId The id of the stream as emitted by signalStreamAdded()
     * @param muteState The volume (range?)
     * @todo Check whether it is muted if muteState == true and correct API doc if needed.
     */
     void slotMuteOutput( uint streamId, bool muteState );
     
    /**
     * Set output Window.
     * @param streamId The id of the stream as emitted by signalStreamAdded()
     * @param windowId The window id of the window that should be used to embed the output.
     * @todo Tell how to obtain a windowId
     */
     void slotSetOutputWindow( uint streamId, uint windowId );
     
    /**
     * Add a preview window.
     * @param windowId The window id of the window that should be used to show the preview image.
     * @see slotRemovePreviewWindow()
     * @todo Tell how to obtain a windowId
     */
     void slotAddPreviewWindow( uint windowId );
     
    /**
     * Remove a preview window.
     * @param windowId The window id of the window that was used to show the preview image.
     * @see slotAddPreviewWindow()
     * @todo Tell how to obtain a windowId
     */
     void slotRemovePreviewWindow( uint windowId );
     
     /**
     * Shutdown.
     * Shuts the stream engine down and deletes all internal interfaces.
     * @todo What does this function really does and how to handle this here?
     */
     void slotShutDown();

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

    /** Add members to interal group channel */
    bool addMembers( QList<uint> handles );
    
    /** Remove members from interal group channel */
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
