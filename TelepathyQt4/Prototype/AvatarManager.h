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
#ifndef TelepathyQt4_Prototype_AvatarManager_H_
#define TelepathyQt4_Prototype_AvatarManager_H_

#include <QDBusObjectPath>
#include <QObject>
#include <QPointer>

#include <TelepathyQt4/Types>

namespace Tp
{
    namespace Client
    {
        class ConnectionInterface;
    }
}

namespace TpPrototype {

class AvatarManagerPrivate;
class Connection;
class Contact;
class Account;

/**
 * @ingroup qt_connection
 * This class manages avatar information for one connection.
 * Whenever a contact avatar changes, the signal signalAvatarChanged() is emitted. This signal provides the related contact object
 * obtained from the ContactManager.
 * In order to keep the contacts updated, you just have to instantiate this class (by requesting the object with Connection::avatarManager())
 * and initialize the list of contacts once (by calling avatarForContactList() ). After this point, the avatar of the contact
 * is updated automatically if a change is signalled by the backend.
 * @see Connection
 */
class AvatarManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * The required Avatar format.
     */
    struct AvatarRequirements
    {
        /** The list of supported Mimetypes */
        QStringList mimeTypes;
        /** The minimum image width */
        uint      minimumWidth;
        /** The minmum image height */
        uint      minimumHeight;
        /** The maximum image width */
        uint      maximumWidth;
        /** The maximum image height */
        uint      maximumHeight;
        /** The maximum size */
        uint        maxSize;
        /** data validity */
        bool        isValid;
    };

    /**
     * The Avatar
     */
    struct Avatar
    {
        /** The avatar data */
        QByteArray avatar;
        /** The mimetype of this data */
        QString    mimeType;
        /** The id associated with this avatar */
        QString token;
    };
    
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
     * Set local Avatar.
     * This function sets the capabilites of the account that belongs to this connection.
     * @param newValue The new avatar. The content of <i>token</i> is ignored.
     * @return true if setting was successful
     */
    bool setAvatar( const TpPrototype::AvatarManager::Avatar& newValue );
    
    /**
     * Request local Avatar.
     * Requests the avatar of the account that belongs to this connection.
     * <b>Info:</b> The signal signalOwnAvatarChanged() is called asynchonously after this call.
     */
    void requestAvatar();

    /**
     * Get the required format of avatars on this connection.
     * @return The requirements of supported avatars.
     * @see AvatarRequirements
     */
    AvatarRequirements avatarRequirements();
    
    /**
     * Get the avatar for a list of contacts.
     * The avatars can be requested from the contact object.
     * <b>Info:</b> The signal signalAvatarChanged() is called asynchonously after this call for every contact in this list.
     * @param List of contacts to request the avatars from.
     * @see signalAvatarChanged()
     */
    void avatarForContactList( const QList<QPointer<Contact> >& contacts );
    
signals:
    /**
     * The avatar of a contact was changed. This signal is emitted when any of the known contacts changed its avatar.
     */
    void signalAvatarChanged( TpPrototype::Contact* contact );

    /**
     * My avatar was changed.
     * This signal is emmitted if the local avatar was changed.
     */
    void signalOwnAvatarChanged( TpPrototype::AvatarManager::Avatar avatar );
 
protected:
    /**
     * Constructor. The capabilities manager cannot be instantiated directly. Use Connection::AvatarManager() for it!
     */
    AvatarManager( TpPrototype::Connection* connection,
                   Tp::Client::ConnectionInterface* interface,
                   QObject* parent = NULL );
    ~AvatarManager();

protected slots:
    void slotAvatarUpdated( uint contact, const QString& newAvatarToken );
    void slotAvatarRetrieved( uint contact, const QString& token, const QByteArray& avatar, const QString& type );
 
private:
    void init( TpPrototype::Connection* connection, Tp::Client::ConnectionInterface* interface );
    
    TpPrototype::AvatarManagerPrivate * const d;
    friend class Connection;
    friend class ConnectionPrivate;
};
}

Q_DECLARE_METATYPE( TpPrototype::AvatarManager::Avatar );
Q_DECLARE_METATYPE( TpPrototype::AvatarManager::AvatarRequirements );
#endif
