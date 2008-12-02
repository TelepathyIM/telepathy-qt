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

#ifndef TelepathyQt4_Prototype_ContactManager_H_
#define TelepathyQt4_Prototype_ContactManager_H_

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Constants>

#include <QObject>
#include <QDBusObjectPath>
#include <QPointer>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif

/**
 * @defgroup qt_contact Contact Management
 * @ingroup qt_style_api
 * Classes that provide functions to handle contacts.
 */



namespace Telepathy
{
    namespace Client
    {
        class ConnectionInterface;
        class ChannelInterfaceGroupInterface;

    }
}

namespace TpPrototype {

class ContactManagerPrivate;
class Contact;
class Connection;
class ChatChannel;
class StreamedMediaChannel;

/**
 * @ingroup qt_connection
 * @ingroup qt_contact
 * This class manages all contacts.
 * This class provides the list of contacts associated with an account. It is possible to register new contacts by requestContact(), or remove contacts
 * by removeContact(). Signals providing information whether any contact changes or wether a communication channel is opened.
 * @todo We need a ContactGroup that contains the Contacts instead use QList< QPointer<> >. If a contact is removed we get a NULL element in the list instead to remove one element from the list (seil)
 */
class ContactManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Validity of this class.
     * Do not access any methods if this object is invalid.
     */
    bool isValid();

    /**
     * Number of Contacts. Returns how many contacts are available.
     * @return Number of contacts available.
     */
    int count();

     /**
      * List of contacts. The contact pointer is stored in a QPointer. If the contact is removed it is deleted
      * by the contact manager. Thus, the pointer is set to 0.
     * @todo: Return ContactGroup here, instead a list of a pointer.
      */
     QList<QPointer<TpPrototype ::Contact> > contactList();

     /**
      * List of contacts that have requested authorization from us.
      *
      * The contact pointer is stored in a QPointer. If the contact is removed it is deleted
      * by the contact manager. Thus, the pointer is set to 0.
      * @todo: Return ContactGroup here, instead a list of a pointer.
      */
      QList<QPointer<TpPrototype ::Contact> > toAuthorizeList();

     /**
      * List of contacts <i>we</i> asked to authorize us to see their presence state.
      *
      * The contact pointer is stored in a QPointer. If the contact is removed it is deleted
      * by the contact manager. Thus, the pointer is set to 0.
      * @todo: Return ContactGroup here, instead of a list of pointer.
      */
     QList<QPointer<TpPrototype ::Contact> > remoteAuthorizationPendingList();

     /**
      * List of contacts that are blocked.
      * @todo: Return ContactGroup here, instead of a list of pointer.
      */
     QList<QPointer<TpPrototype::Contact> > blockedContacts();

     /**
      * Request a contact.
      * A remote contact should be added to the list of known contacts.
      * @param id The id identifies the contact (protocol specific)
      * @return true if the handle was found else false.
      */
     bool requestContact( const QString& id );

     /**
      * Authorize contact.
      * A remote contact should be authorized.
      * @param contact The contact to autorize.
      * @return true if ...
      */
     bool authorizeContact( const Contact* contact );

     /**
      * Remove a contact.
      * The contact should be removed.
      * @param contactToRemove The contact that should be removed. Do not use this pointer after this call!
      */
     bool removeContact( const TpPrototype ::Contact* contactToRemove );

      /**
      * Block a contact.
      * The contact is moved to the list of blocked contacts. This may not be supported by the protocol/contact manager. In this case false is returned.
      * A blocked contact will not receive any presence information from the local account. Messages from this contact are ignored.
      * The signal signalContactBlocked() is emitted if this call was successful.<br>
      * <b>Hint:</b>Blocking support is currently available with the connection manager Gabble and GoogleTalk.
      * @return true if blocking is supported.
      */
     bool blockContact( const Contact* contactToBlock );

     /**
      * Block a contact.
      * The contact is moved to the list of blocked contacts. This may not be supported by the protocol/contact manager. In this case false is returned.
      * A blocked contact will not receive any presence information from the local account. Messages from this contact are ignored.<br>
      * The signal signalContactUnblocked() is emitted if this call was successful.<br>
      * <b>Hint:</b>Blocking support is currently available with the connection manager Gabble and GoogleTalk.
      * @return true if blocking is supported
      */
     bool unblockContact( const Contact* contactToUnblock );

     /**
      * Returns a contact pointer for handle id.
      * This function provides a fast lookup of a contact if a valid handle is provided.
      * @return The contact that corresponds to the handle or NULL if the handle is unknown.
      */
     QPointer<TpPrototype ::Contact> contactForHandle( uint handle );

     /**
      * The local handle.
      * The local user has a handle that is returned by this function. The local handle cannot be used to get
      * a valid contact with contactForHandle()!
      */
      uint localHandle();

signals:
    /**
     * A text channel was opened.
     * This signal is emitted when a text channel was opened for a contact. This usually means that you received a text message.<br>
     * The chat channel object can be retrieved from the contact.
     * @param contact The contact that received the text message and contains the text channel object.
     */
    void signalTextChannelOpenedForContact( TpPrototype ::Contact* contact );

    /**
     * A streamed media channel was opened.
     * This signal is emitted when a streamed media channel was opened for a contact and all interfaces were established successfully.
     * This usually means that you received a call.<br>
     * The StreamedMedia channel object can be retrieved from the contact. Use acceptIncomingStream() or rejectIncomingStream() to accept or reject.
     * @param contact The contact that received the text message and contains the StreamedMediaChannel object.
     * @see StreamMediaChannel
     */
    void signalStreamedMediaChannelOpenedForContact( TpPrototype ::Contact* contact );

    /**
     * A Contact was added.
     * A new remote Contact was added but needs to be accepted remotely.
     * @see signalContactRemotePending()
     */
    void signalContactAdded( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );
    
    /**
     * A Contact is pending locally.
     * A Contact that is local pending have requested membership of the channel, but the local user of the framework must
     * accept their request before they may join.
     */
    void signalContactLocalPending( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );
    
    /**
     * A Contact is pending remotely.
     * A Contact that is remote pending list have been invited to the channel, but the remote user has not
     * accepted the invitation.
     */
    void signalContactRemotePending( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );
    
    /**
     * A Contact was subscribed.
     * This signal is emitted if a new contact was subscribed.
     */
    void signalContactSubscribed( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );
    
    /**
     * A Contact was removed.
     * This signal is emitted if a contact was removed.
     * @param contact The removed contact. This object will be delted after this call!
     */
    void signalContactRemoved( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );
    
    /**
     * @todo: Add doc! (seil)
     */
    void signalContactKnown( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );

    /**
     * @todo: Add doc!
     * Contact was blocked.
     * This signal is emitted after a contact was blocked.
     */
    void signalContactBlocked( TpPrototype::ContactManager* contactManager, TpPrototype::Contact* contact );

    /**
     * Contact was unblocked.
     * This signal is emitted after a contact was unblocked.
     */
    void signalContactUnblocked( TpPrototype::ContactManager* contactManager, TpPrototype::Contact* contact );

    /**
     * @todo: Add doc! (seil)
     */
    void signalForModelContactSubscribed( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );

    /**
     * @todo: Add doc! (seil)
     */
    void signalForModelContactRemoved( TpPrototype ::ContactManager* contactManager, TpPrototype ::Contact* contact );

    /**
     * Members changed.
     * This signal is emitted whenever one of the internal lists of contacts (member, local pending, remote pending) changes.
     * @param contactManager The contact manager that handles the contacts.
     * @param message A string message from the server, or blank if not
     * @param members List of contacts currently listed as members.
     * @param localPending Contacts that are waiting for a local approval.
     * @param remotePending Contacs that are waiting for a remote approval.
     * @param actor The contact that causes the change. May be NULL if unkown.
     * @param reason The reason of the change.
     */
    void signalMembersChanged( TpPrototype ::ContactManager* contactManager,
                               const QString& message,
                               QList<QPointer<TpPrototype ::Contact> > members,
                               QList<QPointer<TpPrototype ::Contact> > localPending,
                               QList<QPointer<TpPrototype ::Contact> > remotePending,
                               TpPrototype ::Contact* actor,
                               Telepathy::ChannelGroupChangeReason reason );

protected:
    /**
     * Constructor. The contact manager cannot be instantiated directly. Use Connection::contactManager() for it!
     */
    ContactManager( Telepathy::Client::ConnectionInterface* connection,
                    QObject* parent = NULL );
    ~ContactManager();

    
    void openSubscribedContactsChannel(uint handle, const QDBusObjectPath& objectPath, const QString& channelType);

     /**
     * ..
      */
    void openPublishContactsChannel(uint handle, const QDBusObjectPath& objectPath, const QString& channelType);
     
     /**
     * ..
      */
    void openKnownContactsChannel(uint handle, const QDBusObjectPath& objectPath, const QString& channelType);

    /**
     * ..
     */
    void openDenyContactsChannel(uint handle, const QDBusObjectPath& objectPath, const QString& channelType);

    void openTextChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelType );

    void openStreamedMediaChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelType );

protected slots:

    void slotMembersChanged(const QString& message,
                            const Telepathy::UIntList& members_added,
                            const Telepathy::UIntList& members_removed,
                            const Telepathy::UIntList& local_pending,
                            const Telepathy::UIntList& remote_pending,
                            uint actor, uint reason);
    void slotKnownMembersChanged(const QString& message,
                            const Telepathy::UIntList& members_added,
                            const Telepathy::UIntList& members_removed,
                            const Telepathy::UIntList& local_pending,
                            const Telepathy::UIntList& remote_pending,
                            uint actor, uint reason);
    void slotPublishedMembersChanged(const QString& message,
                                 const Telepathy::UIntList& members_added,
                                 const Telepathy::UIntList& members_removed,
                                 const Telepathy::UIntList& local_pending,
                                 const Telepathy::UIntList& remote_pending,
                                 uint actor, uint reason);
    void slotSubscribedMembersChanged(const QString& message,
                                     const Telepathy::UIntList& members_added,
                                     const Telepathy::UIntList& members_removed,
                                     const Telepathy::UIntList& local_pending,
                                     const Telepathy::UIntList& remote_pending,
                                     uint actor, uint reason);       
    void slotDeniedMembersChanged(const QString& message,
                                      const Telepathy::UIntList& members_added,
                                      const Telepathy::UIntList& members_removed,
                                      const Telepathy::UIntList& local_pending,
                                      const Telepathy::UIntList& remote_pending,
                                      uint actor, uint reason);
private:
    void init( Telepathy::Client::ConnectionInterface* connection );

    ContactManagerPrivate * const d;
    friend class Connection;
    friend class ConnectionPrivate;
    friend class Contact;
};

}

#endif
