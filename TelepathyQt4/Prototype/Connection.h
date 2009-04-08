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
#ifndef TelepathyQt4_Prototype_Connection_H_
#define TelepathyQt4_Prototype_Connection_H_

#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

#include <TelepathyQt4/Constants>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif

namespace Tp
{ 
    namespace Client
    {
        class ConnectionInterface;
    }
}

/**
 * @defgroup qt_connection Connection Management
 * @ingroup qt_style_api
 * Classes that provide functions to handle connections and additional optional interfaces related to a connection.
 */



namespace TpPrototype {

class ConnectionPrivate;
class ContactManager;
class PresenceManager;
class CapabilitiesManager;
class AvatarManager;
class Account;
/**
 * @ingroup qt_connection
 * This class manages a connection.
 * The connection object provides access to optional interfaces that are related to the connection using contactManager(),
 * capabilitiesManager(), presenceManager(), avatarManager().
 * @todo In order to allow custom extensions, we need a support for register proxies in the Account object
 */
class Connection : public QObject
{
    Q_OBJECT
    Q_PROPERTY( bool valid READ isValid )
public:

    ~Connection();

    /**
     * Validity check.
     * Do not access any functions if this account is invalid.
     */
    bool isValid();

    /**
     * Connection Status.
     * @return The current connection status.
     * @see Tp::ConnectionStatus defined in constants.h
     */
    Tp::ConnectionStatus status();

    /**
     * Reason for last state change.
     * @return The reason.
     * @see Tp::ConnectionStatusReason defined in constants.h
     */
    Tp::ConnectionStatusReason reason();

   /**
    * Connect to server.
    * This call is asynchrous. Wait until signalStatusChanged() was emitted and the connection state is Tp::ConnectionStatusConnected
    * before calling contactManager() or presenceManager() will succeed.
    * @see signalStatusChanged()
    */
    bool requestConnect();

   /**
    * Disconnect. Disconnects from the server.
    */
    bool requestDisconnect();

    /**
     * Returns the contact list manager.
     * The contact list manager contains the list of contacts and provides functions like add and remove.<br>
     * <b>Note:</b> You have to request a connection with requestConnect() before a contact manager can be returned. If the connection disconnects the ContactManager will be invalid!
     * @return Pointer to the contact manager or NULL if something went wrong.
     * @see requestConnect()
     */
    ContactManager* contactManager();

    /**
     * Returns the presence manager.
     * The presence manager handles your presence state for this connection.<br>
     * <br>
     * <b>Note:</b>
     * <ul><li>You have to request a connection with requestConnect() before a presence manager can be returned.</li>
     *     <li>You have to call this function in order to get your contacts updated automatically.</li>
     * </ul>
     * @return Pointer to the presence manager or NULL if no presence handling is supported or no connection was requested.
     * @see requestConnect()
     */
    PresenceManager* presenceManager(); 
 
    /**
     * Returns the capabilities manager.
     * The capabilities manager handles your capabilities state for this connection.<br>
     * <br>
     * <b>Note:</b>
     * <ul><li>You have to request a connection with requestConnect() before a capability manager can be returned.</li>
     *     <li>You have to call this function in order to get your contacts updated automatically.</li>
     * </ul>
     * @return Pointer to the capabilities manager or NULL if no capabilities handling is supported or no connection was requested.
     * @see requestConnect()
     */
    CapabilitiesManager* capabilitiesManager();

    /**
     * Returns the avatar manager.
     * The avatar manager provides you information about the avatars of the connection.<br>
     * <br>
     * <b>Note:</b>
     * <ul><li>You have to request a connection with requestConnect() before an avatar manager can be returned.</li>
     *     <li>You have to call this function in order to get your contacts updated automatically.</li>
     * </ul>
     * @return Pointer to the avatar manager or NULL if no avatar support is available or no connection was requested.
     * @see requestConnect()
     */
    AvatarManager* avatarManager();
   
    /**
     * Returns the account for this connection.
     * Every connection belongs to an account that is returned with this call.
     * @return The account for this connection or NULL if no valid account exists.
     */
    Account* account() const;

signals:
    /**
     * Connection status changed.
     * This signal is emitted if the status of the connection was changed
     * @param connection The connection which changes its status.
     * @param newStatus The new status that is valid now.
     * @param oldStatus The old status that was valid before.
     * @see Tp::ConnectionStatus
     */
    void signalStatusChanged( TpPrototype::Connection* connection,
                              Tp::ConnectionStatus newStatus,
                              Tp::ConnectionStatus oldStatus );

protected slots:
    void slotStatusChanged( uint status, uint reason );
    void slotNewChannel( const QDBusObjectPath& objectPath, const QString& channelType, uint handleType, uint handle, bool suppressHandler );

protected:
   /**
    * Constructor.
    * The connection cannot be instantiated directly. Use Account::connection() to receive a valid connection object.
    * @param account Account to create connection with.
    */
    Connection( TpPrototype::Account* account, QObject* parent );

    /**
     * Returns the handle.
     * The handle is an internal representation to access the real data. Its format should
     * not be interpreted.
     */
    QString handle() const;

    /**
     * D-BUS interface.
     * This protected access to the D-BUS interface can be used to extend this class with special features.
     */
    Tp::Client::ConnectionInterface* interface();

    /**
     * Provides a generic handle.
     * @param handleType The type of handle required.
     * @param handlestrings An array of names of entities to request handles for
     * @return An array of integer handle numbers in the same order as the given strings
     */
    QList<uint> RequestHandles( Tp::HandleType handletype, QStringList& handlestrings);

    /**
     * Check if manager is supported.
    */
    bool managerSupported( const QString& managerName );

    template <class Manager>
            inline Manager* createManager( QPointer<Manager>& pManager, const QString& managerName )
    {
        if ( pManager && pManager->isValid() )
        { return pManager; }

        // Force to reinitialize an invalid manager
        if ( pManager && !pManager->isValid() )
        {
            delete pManager;
            pManager = NULL;
        }

        if ( status() != Tp::ConnectionStatusConnected )
        { return NULL; }

        pManager = new Manager( this, interface(), this );
        Q_ASSERT( pManager );
        
        if ( pManager->isValid() )
        { return pManager; }
            
        // Fall through: Cleanup if manager is not valid.
        delete pManager;
        pManager = NULL;
        return NULL;
    }


private:
    void init( TpPrototype::Account* account );
    void startupInit();
    ConnectionPrivate * const d;

    friend class Account;
};

} // namespace

#endif
