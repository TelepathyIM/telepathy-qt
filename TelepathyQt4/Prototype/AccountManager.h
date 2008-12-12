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
#ifndef TelepathyQt4_Prototype_AccountManager_H_
#define TelepathyQt4_Prototype_AccountManager_H_

#include <QDBusObjectPath>
#include <QObject>
#include <QPointer>

#include <TelepathyQt4/Prototype/Account.h>

#ifdef DEPRECATED_ENABLED__
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define ATTRIBUTE_DEPRECATED
#endif


namespace TpPrototype {
// 
class AccountManagerPrivate;

/**
 * @defgroup qt_accountmgm Account Management
 * @ingroup qt_style_api
 * Classes that provide functions to handle accounts.
 */


/**
 * This class manages all accounts.
 * The account manager provides access to the list of accounts. Additionally you can create and remove accounts.<br>
 * Use the Connectionfacade to obtain a list of valid parameters for a protocol or the list of available connection managers.<br>
 * @see ConnectionFacade
 * @ingroup qt_accountmgm
 * @todo Integrate all functions related to Accounts from the ConnectionFacade into this class.
 */
class AccountManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Returns pointer to the instance of this class.
     * @return Instance pointer.
     */
    static AccountManager* instance();
    
    /**
     * Number of Accounts. Returns how many accounts are available.
     * @return Number of accounts available.
     * @deprecated Use accountList().count() instead.
     */
    int count() ATTRIBUTE_DEPRECATED;

     /**
      * List of accounts. The account pointer is stored in a QPointer. If the account is removed it is deleted
      * by the account manager. Thus, the pointer is set to 0.
      * @todo: Return QList<Account> here, instead of a pointer. 
      */
     QList<QPointer<Account> > accountList();

     /**
      * List of enabled accounts
      */
     QList<QPointer<Account> > accountListOfEnabledAccounts();

     /**
      * Create account. This function creates an account with the given parameters.<br>
      * <b>Note:</b> Although this call is synchronous, the internal book keeping of valid accounts is
      *              updated by DBUS signals that might need some time. Thus, calling count() emmediately after create might
      *              return an incorrect value. Wait until signalAccountsUpdated() is emitted.
      * @param connectionManager The name of the connection manager, e.g. &quot;salut&quot;.
      * @param protocol The protocol, e.g. &quot;local-xmpp&quot;. 
      * @param parameters List of parameters needed to create the account
      * @param displayName The name of the account to display.
      * @return true when creating was successful.
      */
     bool createAccount( const QString& connectionManager, const QString& protocol, const QString& displayName, const QVariantMap& parameters );
      
     /**
      * Remove account. Removes the given account.
      * @param account The pointer to the account. The pointer is not accessable after this call!
      * @return true if remove operation was successful.
      */
     bool removeAccount( Account* account );
       
signals:
    /**
     * Some changes occurred on the account data. This signal is emitted if the internal data of the account manager
     * is changed (accounts were created or removed).<br>
     * It is suggested to refetch all locally stored data after this signal.
     */
    void signalAccountsUpdated();

    /**
     * A new account is available.
     * This signal is emitted if a new account was created by a contact manager
     */
    void signalNewAccountAvailable( TpPrototype::Account* account );

    /**
     * An account was removed.
     * This signal is emitted after an account was removed. Do not use <i>account</i> after receiving this signal!
     */
    void signalAccountRemoved( TpPrototype::Account* account );
    
    /**
     * An account was updated.
     * This signal is emitted after an account was updated.
     */
    void signalAccountUpdated( TpPrototype::Account* account );
    
protected:
    /**
     * Constructor. The account manager cannot be instantiated directly. Use instance() for it!
     */
    AccountManager( QObject* parent = NULL );
    ~AccountManager();

protected slots:
    void slotAccountValidityChanged( const QDBusObjectPath& account, bool valid );
    void slotAccountRemoved( const QDBusObjectPath& account );
    void slotAccountRemoved();
    void slotAccountUpdated();

private:
    void init();
    
    AccountManagerPrivate * const d;
    static AccountManager* m_pInstance;
};

}

#endif
