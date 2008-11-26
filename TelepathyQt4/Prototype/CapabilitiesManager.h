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

#ifndef TelepathyQt4_Prototype_CapabilitiesManager_H_
#define TelepathyQt4_Prototype_CapabilitiesManager_H_

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

class CapabilitiesManagerPrivate;
class Connection;
class Contact;
class Account;

/**
 * @ingroup qt_connection
 * This class manages capabilities information for one connection.
 * Setting the right capability decide whether it is possible to handle incoming or outgoing VoIP or Video over IP channels!
 * Whenever a contact capability changes, the signal signalCapabilitiesChanged() is emitted. This signal provides the related contact object
 * obtained from the ContactManager.
 * In order to keep the contacts updated, you just have to instantiate this class (by requesting the object with Connection::capabilitiesManager())
 * and initialize the list of contacts once (by calling capabilitiesForContactList() ). After this point, the capabilty information of the contact
 * is updated automatically if a change is signalled by the backend.
 * @see Connection
 * @see StreamedMediaChannel
 */
class CapabilitiesManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Validity.
     * Do not access any methods if the object is invalid!
     */
    bool isValid();

    /**
     * Returns the connection that belongs to this capabilities information.
     * @return The connection object
     */
    TpPrototype::Connection* connection();
    
    /**
     * Set the capabilities.
     * This function sets the capabilites of the account that belongs to this connection.
     * @param capabilities List of capabilities for a specific channel. See Telepathy D-Bus spec section "Channel_Media_Capabilities"
     * @param removedChanels List of channels that are removed.
     * @return true if setting was successful
     */
    bool setCapabilities( const Telepathy::CapabilityPairList& capabilities, const QStringList& removedChannels = QStringList() );
    
    /**
     * Request capabilites.
     * Returns the capabilites of the account that belongs to this connection.
     * @return List of capabilities
     */
    Telepathy::ContactCapabilityList capabilities();
    
    /**
     * Gets the capabilities for a list of contacts and provides them to to specific contacts.
     * The capabilities can be requested from the contact object.
     */
    void capabilitiesForContactList( const QList<QPointer<Contact> >& contacts );
    
signals:
    /**
     * The capability of a contact has changed. This signal is emitted when any of the known contacts changed its capability.
     */
    void signalCapabilitiesChanged( TpPrototype::Contact* contact, const Telepathy::CapabilityChange& changedCapability );

    /**
     * My capability was changed. This signal is emmitted if the capability of one of my channes was changed.
     */
    void signalOwnCapabilityChanged( const Telepathy::CapabilityChange& changedCapability );
 
protected:
    /**
     * Constructor. The capabilities manager cannot be instantiated directly. Use Connection::CapabilitiesManager() for it!
     */
    CapabilitiesManager( TpPrototype::Connection* connection,
                     Telepathy::Client::ConnectionInterface* interface,
                     QObject* parent = NULL );
    ~CapabilitiesManager();

protected slots:
    void slotCapabilitiesChanged( const Telepathy::CapabilityChangeList& capabilities );
 
private:
    void init( TpPrototype::Connection* connection, Telepathy::Client::ConnectionInterface* interface );
    
    TpPrototype::CapabilitiesManagerPrivate * const d;
    friend class Connection;
    friend class ConnectionPrivate;
};
}

#endif
