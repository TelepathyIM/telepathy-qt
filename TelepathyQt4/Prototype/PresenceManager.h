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
#ifndef TelepathyQt4_Prototype_PresenceManager_H_
#define TelepathyQt4_Prototype_PresenceManager_H_

#include <QDBusObjectPath>
#include <QObject>
#include <QPointer>

#include <TelepathyQt4/Types>

namespace Telepathy
{
    namespace Client
    {
        class ConnectionInterface;
    }
}

namespace TpPrototype {

class PresenceManagerPrivate;
class Connection;
class Contact;
class Account;

/**
 * @ingroup qt_connection
 * This class manages presence information for one connection.
 * Whenever a contact presence changes, the signal signalRemotePresencesUpdated() is emitted. This signal provides the related contact object
 * obtained from the ContactManager.
 * In order to keep the contacts updated, you just have to instantiate this class (by requesting the object with Connection::presenceManager())
 * and initialize the list of contacts once (by calling presencesForContacts() ). After this point, the presence information of the contact
 * is updated automatically if a change is signalled by the backend.<br>
 * <br>
 * <b>Info:</b> The deprecated interface "org.freedesktop.Telepathy.Channel.Interface.Presence" is used as fallback, if the interface
 * "org.freedesktop.Telepathy.Channel.Interface.SimplePresence" is not supported by the connection manager. Thus, a wide range of
 * connection managers should work with this class.

 * @see Connection
 */
class PresenceManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Validity.
     * Do not access any methods if the object is invalid!
     */
    bool isValid();

    /**
     * Supported statuses.
     * Returns the list of supported status states. The list may change if the status of the connection changes from <i>disconnected</i> to <i>connected</i>. 
     * @return The list of supported statuses.
     * @see connection::status();
     */
     Telepathy::SimpleStatusSpecMap statuses();

    /**
    * Set Presence.
    * Request that the presence status and status message are published for the connection.  Changes will be indicated by
    * signal signalOwnPresenceUpdated().
    * @param status The state to set the presence to as returned by statuses().
    * @see signalOwnPresenceUpdated().;
    */
    bool setPresence( const QString& status, const QString& statusMessage );

    /**
     * Gets local presence.
     * The local presence is returned for the connection.
     * @return Returns my current local presence or an empty item on error.
     */
    Telepathy::SimplePresence currentPresence();

    /**
     * Request presences.
     * Requests a list of presences for the given list of contacts.
     * @param contacts List of contacts.
     * @return List of presence information for contacts as <i>QMap<int,SimplePresence> </i>. The <i>int</i> represents the
     * identifier of the contact as returned by Contact::identifier(), An empty list is returned on error.
     * @todo Future: Use QList<Contact> instead QList<QPointer<Contact> > or introduce a class <i>ContactGroup</i> that handles all internally.
     * @todo Future: Telepathy::SimpleContactPresences relies of an handle (the <i>int</i>). This should be encapsulated.
     */
    Telepathy::SimpleContactPresences presencesForContacts( const QList<QPointer<TpPrototype::Contact> >& contacts );

    /**
     * Returns the connection that belongs to this presence information.
     * @return The connection object
     */
    TpPrototype::Connection* connection();

signals:
    /**
     * Presences of remote contacts are changed.
     * This signal is emitted when the presence state of a remote contact is changed.
     * @param contact The contact that changes.
     * @param presence The presence information.
     */
    void signalRemotePresencesUpdated( TpPrototype::Contact* contact, const Telepathy::SimplePresence& presence );

    /**
     * Local presence changed.
     * This signal is emitted when the local presence state was changed.
     * @param account The account that changes.
     * @param presence The presence information.
     */
    void signalOwnPresenceUpdated( const TpPrototype::Account* account, const Telepathy::SimplePresence& presence );
    
protected:
    /**
     * Constructor. The presence manager cannot be instantiated directly. Use Connection::presenceManager() for it!
     */
    PresenceManager( TpPrototype::Connection* connection,
                     Telepathy::Client::ConnectionInterface* interface,
                     QObject* parent = NULL );
    ~PresenceManager();

protected slots:
    void slotPresencesChanged( const Telepathy::SimpleContactPresences& presences );
    void slotPresencesUpdate( const Telepathy::ContactPresences& presences );
private:
    void init();
    
    TpPrototype::PresenceManagerPrivate * const d;
    friend class Connection;
    friend class ConnectionPrivate;
};
}


#endif
