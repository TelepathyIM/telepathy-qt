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

#include "TelepathyQt4/Prototype/AccountManager.h"

#include <QCoreApplication>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDebug>
#include <QMap>
#include <QPointer>

#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Types>

#include <TelepathyQt4/Prototype/Account.h>

// #define ENABLE_DEBUG_OUTPUT_

using namespace TpPrototype;

AccountManager* TpPrototype::AccountManager::m_pInstance = NULL;

class TpPrototype::AccountManagerPrivate
{
public:
    AccountManagerPrivate()
    { init(); }

    Telepathy::Client::AccountManagerInterface* m_pInterface;
    Telepathy::Client::DBus::PropertiesInterface* m_propertiesInterface;
    QMap<QString, QPointer<Account> >  m_validAccountHandles;

    void init()
    {
        m_pInterface = NULL;
    }

    Telepathy::ObjectPathList validAccounts()
    {
        Q_ASSERT(m_propertiesInterface);

        QDBusPendingReply<QDBusVariant> get = m_propertiesInterface->Get(
                "org.freedesktop.Telepathy.AccountManager", "ValidAccounts");
        get.waitForFinished();

        if (!get.isValid())
        {
            qWarning().nospace() << get.error().name() << ": "
                << get.error().message();
            return Telepathy::ObjectPathList();
        }

        Telepathy::ObjectPathList paths = qdbus_cast<Telepathy::ObjectPathList>(
                get.value().variant());

        if (paths.size() == 0)
        {
            // maybe the AccountManager is buggy, like Mission Control
            // 5.0.beta45, and returns an array of strings rather than
            // an array of object paths?

            QStringList wronglyTypedPaths = qdbus_cast<QStringList>(
                    get.value().variant());

            if (wronglyTypedPaths.size() > 0)
            {
                qWarning() << "AccountManager returned wrong type "
                    "(expected 'ao', got 'as'); workaround active";

                Q_FOREACH (QString path, wronglyTypedPaths)
                {
                    paths << QDBusObjectPath(path);
                }
            }
        }

        return paths;
    }

    void removeAccount( const QString& handle )
    {
        if ( m_validAccountHandles.contains( handle ) )
        {
            delete m_validAccountHandles.value( handle );
            m_validAccountHandles.remove( handle );
        }
    }

    void connectAccountToManager( const TpPrototype::Account* account, TpPrototype::AccountManager* dest )
    {
        QObject::connect( account, SIGNAL( signalRemoved() ),
                          dest, SLOT( slotAccountRemoved() ) );
        QObject::connect( account, SIGNAL( signalPropertiesChanged( const QVariantMap& ) ),
                          dest, SLOT( slotAccountUpdated() ) );
        QObject::connect( account, SIGNAL( signalPresenceChanged() ),
                          dest, SLOT( slotAccountUpdated() ) );
    }

};

AccountManager::AccountManager( QObject* parent ):
    QObject( parent ),
    d( new AccountManagerPrivate )
{
    init();
}

AccountManager* AccountManager::instance()
{
    if ( NULL == m_pInstance )
    {
        m_pInstance = new AccountManager( QCoreApplication::instance() );
        Q_ASSERT( m_pInstance );
    }

    return m_pInstance;
}


AccountManager::~AccountManager()
{
    delete d;
}

int AccountManager::count()
{
    return d->m_validAccountHandles.count();
}

//  TODO: Define a accountlist container class that stores the accounts and is doing the book keeping..
QList<QPointer<Account> > AccountManager::accountList()
{
    QList<QPointer<Account> > ret_list;

    Account* account = NULL;
    QStringList handles = d->m_validAccountHandles.keys(); 
    foreach( const QString& handle, handles )
    {
        account = d->m_validAccountHandles.value( handle );
        Q_ASSERT( account );
        if ( !account )
        {
            qWarning() << "Found handles that points to no object!";
            d->removeAccount( handle );
            continue;
        }
        ret_list.append( QPointer<Account>( account ) );
    }

    return ret_list;
}

#if 0
TpPrototype::Account* account( int num )
{
    if ( d->m_validAccountHandles.at(
}
#endif


QList<QPointer<Account> > AccountManager::accountListOfEnabledAccounts()
{
    QList<QPointer<Account> > ret_list;
    
    if ( accountList().count() != 0)
    {
        foreach (const QPointer<Account>& account, accountList() )
        {
            if ( account->properties().value( "Enabled" ) == true )
            {
                ret_list.append( account );
            }
        }
    }
    return ret_list;
}

bool AccountManager::createAccount( const QString& connectionManager, const QString& protocol, const QString& displayName, const QVariantMap& _parameters )
{
    QVariantMap parameters = _parameters;

    //HACK: Set server for google talk which cannot be set after the account was created.
    //     This should be removed after fixing the create account workflow!
    if ( parameters.value( "account" ).toString().contains( "google" ) )
    {  parameters.insert( "server", "talk.google.com" ); }

    if ( parameters.contains( "port" ) && parameters.value( "port" ).type() != QVariant::UInt )
    {
        qWarning() << "We got the wrong type of the port. We correct it here manually";
        parameters.insert( "port", qvariant_cast<uint>( parameters.value( "port" ) ) );
    }

    // empty Parameterlists should not be send to create account. Otherwise strange things may happen.. 
    if ( parameters.size() > 0 )
    {
        QStringList keys = parameters.keys();
        foreach( const QString& key, keys )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "createAccount--> Key:" << key << "value:" << parameters.value( key );
#endif
            if (parameters.value(key).toString().isEmpty() )
            {
#ifdef ENABLE_DEBUG_OUTPUT_
                qDebug() << "Remove Key:" << key << "value:" << parameters.value( key );
#endif
                parameters.remove( key );
            }
        }
    }

    QDBusPendingReply<QDBusObjectPath> create_reply = d->m_pInterface->CreateAccount( connectionManager,
                                                                                      protocol,
                                                                                      displayName,
                                                                                      parameters );
    create_reply.waitForFinished();
        
    if ( !create_reply.isValid() )
    {
        QDBusError error = create_reply.error();
    
        qWarning() << "Disconnect: error type:" << error.type()
                << "Disconnect: error name:" << error.name()
                << "error message:" << error.message();

        return false;
    }

    // Fall through
    return true;
}

void AccountManager::slotAccountValidityChanged( const QDBusObjectPath& account, bool valid )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AccountManager::slotAccountValidityChanged: " << valid;
#endif
    bool update_occurred = false;
    if ( valid )
    {
        // Add account to the list if it is not already stored
        if ( !d->m_validAccountHandles.contains( account.path() )
             || !d->m_validAccountHandles.value( account.path() ) )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "AccountManager::slotAccountValidityChanged: Add new account to list";
#endif
            TpPrototype::Account* new_account = new Account( account.path(), this );
            d->connectAccountToManager( new_account, this );
            d->m_validAccountHandles.insert( account.path(), QPointer<Account>( new_account ) );
            update_occurred = true;
            emit signalNewAccountAvailable( new_account );
        }
    }
    else
    {
        if ( d->m_validAccountHandles.contains( account.path() ) )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "AccountManager::slotAccountValidityChanged: Remove account from list";
#endif
            emit signalAboutToRemoveAccount( d->m_validAccountHandles.value( account.path() ) );
            d->removeAccount( account.path() );
            emit signalAccountRemoved();
            update_occurred = true;
        }
    }
    if ( update_occurred )
    { emit signalAccountsUpdated(); }
}

void AccountManager::slotAccountRemoved( const QDBusObjectPath& account )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AccountManager::slotAccountRemoved() <external>";
#endif
    Account* account_object = d->m_validAccountHandles.value( account.path() );
    if ( !account_object )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "AccountManager::slotAccountRemoved():Account already removed";
#endif
        return;
    }

    // These signals are emitted if an account was removed extarnally.
    // The signals were already emitted by slotAccountRemoved() if the account was removed locally
    emit signalAboutToRemoveAccount( account_object );
    d->removeAccount( account.path() );
    emit signalAccountRemoved();
    emit signalAccountsUpdated();
}


void AccountManager::slotAccountUpdated()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AccountManager::slotPropertiesChanged()";
#endif
    Account* account = qobject_cast<Account*>( sender() );
    Q_ASSERT( account );

    emit signalAccountUpdated( account );
    emit signalAccountsUpdated();
}

// Called when an Account was removed
void AccountManager::slotAccountRemoved()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "AccountManager::slotAccountRemoved sender:" << sender();
#endif
    Account* account = qobject_cast<Account*>( sender() );
    Q_ASSERT( account );
    if ( account )
    {
        QString handle = account->handle();
        emit signalAboutToRemoveAccount( account );
        d->removeAccount( handle );
        emit signalAccountRemoved();
        emit signalAccountsUpdated();
    }
}

void AccountManager::init()
{
    Telepathy::registerTypes();
    d->m_pInterface = new Telepathy::Client::AccountManagerInterface( "org.freedesktop.Telepathy.AccountManager",
                                                                           "/org/freedesktop/Telepathy/AccountManager",
                                                                           this );
    Q_ASSERT( d->m_pInterface );
    if ( ! d->m_pInterface )
    { return; }
        
    if ( !d->m_pInterface->isValid() )
    {
        qWarning() << "Unable to connect to AccountManagerInterface: MissionControl seems to be missing!";
    }
    //Q_ASSERT( d->m_pInterface->isValid() );

    d->m_propertiesInterface = new Telepathy::Client::DBus::PropertiesInterface(
            "org.freedesktop.Telepathy.AccountManager",
            "/org/freedesktop/Telepathy/AccountManager",
            this);
    Q_ASSERT( d->m_propertiesInterface );

    // It might be better to use layzy initializing here.. (ses)
    Telepathy::ObjectPathList account_handles = d->validAccounts();

    foreach( const QDBusObjectPath& account_handle, account_handles )
    {

        Account* account = new Account( account_handle.path(), this );
        d->connectAccountToManager( account, this );
        d->m_validAccountHandles.insert( account_handle.path(), QPointer<Account>( account ) );
        emit signalNewAccountAvailable( account );
        emit signalAccountsUpdated();
    }

    connect( d->m_pInterface, SIGNAL( AccountValidityChanged( const QDBusObjectPath&, bool ) ),
             this, SLOT( slotAccountValidityChanged( const QDBusObjectPath&, bool ) ) );
    connect( d->m_pInterface, SIGNAL( AccountRemoved(const QDBusObjectPath& ) ),
             this, SLOT( slotAccountRemoved( const QDBusObjectPath& ) ) );
  

}

#include "_gen/AccountManager.h.moc"
