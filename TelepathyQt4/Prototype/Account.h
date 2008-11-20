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

#ifndef TelepathyQt4_Prototype_Account_H_
#define TelepathyQt4_Prototype_Account_H_

#include <QObject>
#include <QPointer>
#include <QVariantMap>

namespace Telepathy {
namespace Client {
class AccountInterface;
}
}

namespace TpPrototype {

class Connection;
class AccountPrivate;

/**
 * @ingroup qt_accountmgm
 * This class manages an account.
 * @todo: Account should be more like QPersistantModelIndex. Thus we don't have to use pointer of it.
 * @todo In order to allow custom extensions, we need a support to register proxies in the AccountManager
 */
class Account : public QObject
{
    Q_OBJECT
public:
   /**
    * The parameters of this account.
    * Returns a list of all parameters of this account that were explicitly set by AccountManager::createAccount().
    * @return The properties as key,value pair.
    */
    QVariantMap parameters();

   /**
    * Properties of this account
    */
    QVariantMap properties();

    /**
     * Set Properties. Changes the given list of Properties.
     * @param properties The list of changed or new properties.
     */
    void setProperties( const QVariantMap& properties );

    /**
     * Set Parameters. Changes the given list of parameters.
     * @param parameters The list of changed or new parameters
     * @return True if successful.
     */
    bool setParameters( const QVariantMap& parameters );

    /**
     * Connection.
     * Returns a connection object that belongs to this account.<br>
     * <b>Info:</b> This class keeps ownership of this class.
     * @param parent The parent of this object. If NULL the connection is used as parent.
     * @return Connection object.
     */
    TpPrototype::Connection* connection( QObject* parent = NULL );

   /**
    * Remove account. Removes the given account.
    * <b>Note:</b> Although this call is synchronous, the internal book keeping of valid accounts is
    *              updated by DBUS signals that might need some time. Thus, calling AccountManager::count() emmediately
    *              after removing might return an incorrect value. Wait until AccountManager::signalAccountsUpdated() is emitted.
    * @return true if remove operation was successful.
    */
    bool remove();

    /**
     * Validity check.
     * Do not access any functions if this account is invalid.
     */
    bool isValid();

    /**
     * Get connection manager for this account.
     * Returns the connection manager that belongs to this account
     * @return The name of the connection manager (like gabble, ..).
     */
    QString connectionManagerName();

    /**
     * Get protocol for this account.
     * Returns the protocol that belongs to this account
     * @return The name of the protocol (like jabber, ..)
     */
    QString protocolName();
    
    /**
     * Get the current presence.
     * The presence for the current connection is returned.
     * @return The current aggregated presence.
     */
    QString currentPresence();

signals:
    /** Property were changed. This signal is emitted when properties were changed. */
    void signalPropertiesChanged( const QVariantMap& properties );

    /** About to remove. This signal is emmitted before the account is removed. */
    void signalAboutToRemove();

    /** Removed. This signal is emmitted after the account is removed. */
    void signalRemoved();

    /** Account presence was changed. Signal is emitted after the account changed its presence state.
     * @todo Implement this..
     */
    void signalPresenceChanged();
    

protected slots:
    void slotPropertiesChanged( const QVariantMap& properties );
    void slotRemoved();

protected:
    /**
     * Constructor. The account manager cannot be instantiated directly. Use AccountManager::account() for it!
     */
    Account( const QString& handle, QObject* parent );
    ~Account();

    /**
     * D-BUS interface.
     * This protected access to the D-BUS interface can be used to extend this class with special features.
     */
    Telepathy::Client::AccountInterface* interface();

    /**
     * Returns the handle.
     * The handle is an internal representation to access the real data. Its format should
     * not be interpreted.
     */
    QString handle() const;
    
private:
    void init( const QString handle );
    
    AccountPrivate * const d;
    friend class AccountManager; // TODO: Remove this friend class
    friend class AccountManagerPrivate;
};

}


#endif
