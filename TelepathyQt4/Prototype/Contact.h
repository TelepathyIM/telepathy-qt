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

#ifndef TelepathyQt4_Prototype_Contact_H_
#define TelepathyQt4_Prototype_Contact_H_

#include <QObject>
#include <QPointer>
#include <QVariantMap>

#include <TelepathyQt4/Channel>

#include <TelepathyQt4/Prototype/AvatarManager.h>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif

namespace Telepathy
{
    namespace Client
    {
        class ConnectionInterface;
    }
}

namespace TpPrototype {

class ContactPrivate;
class ContactManager;
class ChatChannel;
class StreamedMediaChannel;

/**
 * This class handles information related to a contact.
 * Contact objects are always owned by ContactManager. Therefore, there is no way to create or remove a contact without using the ContactManager object.<br>
 * Other Managers (like PresenceManager, AvatarManager) are accessing the contacts inside of the contact manager to update its information.
 * Whether a contact was updated is signalled by the Managers but the new information is stored in the contact object and can be retrieved from it.
 * @ingroup qt_connection
 * @ingroup qt_contact
 * @todo Do not provide a function for every presence parameter. Use SimplePresence instead.
 * @todo Use implicit sharing instead of expolicit sharig!
 * @see CapabilitiesManager, AvatarManager, PresenceManager
 */
class Contact : public QObject
{
    Q_OBJECT
public:
    enum ContactTypes
    {
        CT_Subscribed = 0,
        CT_LocalPending,
        CT_RemotePending,
        CT_Removed,
        CT_Known,
        CT_Blocked
    };

    /**
     * Validity.
     * Do not access any method if this function returns false. 
     */
    bool isValid() const;
    
    /**
     * Returns the contact type.
     * @return The contact type.
     */
    ContactTypes type() const;

    /**
     * Set type.
     * @param type The new type
     * @see ContactTypes
     */
    void setType( ContactTypes type );
    
    /**
     * Returns the telepathy internal handle.
     * @return The telepathy internal handle.
     */
    uint telepathyHandle() const;
    
    /**
     * Returns the telepathy internal handle type.
     * @return The telepathy internal handle type.
     */
    uint telepathyHandleType() const;

    /**
     * Returns the name of the contact.
     * @return The name of the contact as it is used by protocol  (E.g. user@jabber.org).
     * @todo Make this private. We need an encapsulation for </i>PresenceManager::presencesForContacts()</i> first. (ses)
     *       THUN: Stefan, this is not possible as this information is required to work with all telepathy classes
     *       unknown to this lib.
     */
    QString name() const;

    /**
     * Returns the presence information of this contact.
     * This presence information is automatically updated after the presence information was requested for the first time (PresenceManager::presencesForContacts()).<br>
     * <b>Note:</b> Use <i>isPresenceStateAvailable()</i> to check whether the returned presence is valid!
     * @todo Implement this!
     * @see PresenceManager
     */
    Telepathy::SimplePresence presence() { return Telepathy::SimplePresence(); /* TODO: Implement me ! */ };
    
    /**
     * @todo: Add Doc and get/setr in presence manager
     * @deprecated Use <i>presence()</i> instead.
     *
     */
    uint presenceType() ATTRIBUTE_DEPRECATED; 

    /**
     * @todo: Add Doc and get/setr in presence manager
     * @deprecated Use <i>presence()</i> instead.
     *
     */
    QString presenceStatus() ATTRIBUTE_DEPRECATED;

    /**
     * Returns whether there is any presence information available.
     * This functionreturns <i>false</i> if there is no valid presence information available.
     * @return <i>true</i> if valid resence information is available. Otherwise <i>false</i> is returned.
     */
    bool isPresenceStateAvailable();

    /**
     * @todo: Add Doc and get/setr in presence manager
     * @deprecated Use <i>presence()</i> instead.
     *
     */
    QString presenceMessage() ATTRIBUTE_DEPRECATED; 

    /**
     * @todo: Add Doc and get/setr in presence manager
     *
     */
    Telepathy::ContactCapabilityList capabilities() const;
            

    /**
     * Get Avatar for this contact.
     * This function does not return valid information after the avatar was not requested by avatarForContactList()
     * @see AvatarManager
     */ 
    TpPrototype::AvatarManager::Avatar avatar() const;
    
    /**
     * Get chat channel for this contact.
     * This function provides the chat channel object for this contact. It contains all information to do a text chat
     * @return The channel object.
     */
    TpPrototype::ChatChannel* chatChannel();

    /**
     * Get channel for media streaming.
     * This function returns the media streaming object for this contact. It contains all information and functions for VoIP and Video Over IP communication.
     */
    TpPrototype::StreamedMediaChannel* streamedMediaChannel();

    /**
     * Get the contact manager where this object is stored.
     * @return The contact manager that owns this object.
     */
     TpPrototype::ContactManager* contactManager();

protected:
    /**
     * Contstuctor.
     * This object is never created directly. Use ContactManager to create or request a contact.
     * @see ContactManager
     */
    Contact( const uint & handle, const QString & name, ContactTypes type, Telepathy::Client::ConnectionInterface* connectionInterface, TpPrototype::ContactManager* contactManager );

    ~Contact();

     /**
      * @todo: Add Doc and get/setr in presence manager
      */
    void setPresenceType( uint _presenceType);
    
    /**
     * @todo: Add Doc and get/setr in presence manager
     */
    void setPresenceStatus( QString _presenceStatus);

    /**
     * @todo: Add Doc and get/setr in presence manager
     */
    void setPresenceMessage( QString _presenceMessage);

    /**
     * @todo: Add Doc and get/setr in presence manager
     */
    void setCapabilities( const Telepathy::ContactCapabilityList& capabilityList );

    /**
     * Set the avatar.
     * This avatar is set by the AvatarManager.
     *
     */
    void setAvatar( const TpPrototype::AvatarManager::Avatar& avatar );

    /**
     * D-BUS interface.
     * This protected access to the D-BUS interface can be used to extend this class with special features.
     */
    Telepathy::Client::ConnectionInterface* interface();


private:
    ContactPrivate * const d;

    friend class ContactManager;
    friend class AvatarManager;
    friend class PresenceManager;
    friend class CapabilitiesManager;
};

} // namespace

// Q_DECLARE_METATYPE( TpPrototype::Contact )

#endif // Header guard
