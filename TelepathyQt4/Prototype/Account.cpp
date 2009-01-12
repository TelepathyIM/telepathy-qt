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

#include "TelepathyQt4/Prototype/Account.h"

#include <QDebug>
#include <QMetaProperty>

#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/AccountManager>

#include <TelepathyQt4/Prototype/Connection.h>
#include <TelepathyQt4/Prototype/PresenceManager.h>

#define ENABLE_SALUT_WORKAROUND_ // Salut shows incorrect object path.

namespace {
    const int   g_protocolPosition = 6;
    const int   g_connectionManagerPosition = 5;
    const char* g_offline = "offline";
}

using namespace TpPrototype;

class TpPrototype::AccountPrivate
{
public:
    AccountPrivate()
    { init(); }
    
    QString m_handle;
    bool m_isValid;
    Telepathy::Client::AccountInterface* m_pInterface;
    QVariantMap                     m_parameters;
    QVariantMap                     m_properties;
    QString                         m_connectionManagerName;
    QString                         m_protocolName;
    QPointer<TpPrototype::Connection> m_pConnection;

    void init()
    {
        m_isValid    = true;
        m_pInterface = NULL;
        m_pConnection= NULL;
    }
};

Account::Account( const QString& handle, QObject* parent ):
    QObject( parent ),
    d( new AccountPrivate )
{
    init( handle );
}

Account::~Account()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "D'tor Account:" << this;
#endif
    delete d;
}

QVariantMap Account::parameters()
{
    if ( d->m_parameters.isEmpty() )
    {
        d->m_parameters = d->m_pInterface->Parameters();
    }

    return d->m_parameters;
}

QVariantMap Account::properties()
{
    Q_ASSERT( d->m_pInterface );
    if ( d->m_properties.isEmpty() )
    {
        for ( int i = 0; i < d->m_pInterface->metaObject()->propertyCount(); ++i )
        {
            const char* property_name = d->m_pInterface->metaObject()->property( i ).name();
            if ( QString( property_name ) == "objectName" )
            { continue; }
            d->m_properties.insert( property_name, d->m_pInterface->property( property_name ) );
        }
    }
                 
    return d->m_properties;
}

void Account::setProperties( const QVariantMap& properties )
{
    // Update local property cache and save property to DBUS service
    QStringList keys = properties.keys();
    foreach( const QString& key, keys )
    {
        d->m_pInterface->setProperty( qPrintable( key ), properties.value( key ) );
        d->m_properties.insert( key, properties.value( key ) );
    }
}

bool Account::setParameters( const QVariantMap& parameters )
{
    if ( parameters.isEmpty() )
    { return false; }
    
    QDBusPendingReply<> reply = d->m_pInterface->UpdateParameters( parameters, QStringList() );
    reply.waitForFinished();

    if ( !reply.isValid() )
    {
        QDBusError error = reply.error();

        qWarning() << "setParameters: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        return false;
    }

    // Update local cache.
    QStringList keys = parameters.keys();
    foreach( const QString& key, keys )
    {
        d->m_parameters.insert( key, parameters.value( key ) );
    }
    // fall through..
    return true;
}

TpPrototype::Connection* Account::connection( QObject *parent )
{
    if ( !d->m_pConnection )
    {
        d->m_pConnection = new Connection( this, parent ? parent : this );
    }
    return d->m_pConnection;
}

bool Account::remove()
{
    Q_ASSERT( d->m_pInterface );

    emit signalAboutToRemove();
    
    QDBusPendingReply<> remove_reply = d->m_pInterface->Remove();
    remove_reply.waitForFinished();

    if ( !remove_reply.isValid() )
    {
        QDBusError error = remove_reply.error();

        qWarning() << "Remove: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        return false;
    }

    // slotRemoved();

    emit signalRemoved();
    
    // Fall through
    return true;
}

bool Account::isValid()
{
    return d->m_isValid;
}

QString Account::connectionManagerName()
{
    return d->m_connectionManagerName;
}

QString Account::protocolName()
{
    return d->m_protocolName;
}

QString Account::currentPresence()
{
    if ( !connection()->presenceManager() )
    {
        return g_offline;
    }
    return connection()->presenceManager()->currentPresence().status;
}

QString Account::handle() const
{
    return d->m_handle;
}

Telepathy::Client::AccountInterface* Account::interface()
{
    return d->m_pInterface;
}

void Account::slotPropertiesChanged( const QVariantMap& properties )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Account::slotPropertiesChanged(): " << properties;
#endif
    QStringList keys = properties.keys();
    foreach( const QString& key, keys )
    {
        d->m_properties.insert( key, properties.value( key ) );
    }
}

void Account::slotRemoved()
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Account::slotRemoved(): sender:" << sender();
#endif
    d->m_isValid = false;
}

void Account::init( const QString handle )
{
    d->m_handle = handle;
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Handle:" << d->m_handle;
#endif
    d->m_pInterface = new Telepathy::Client::AccountInterface( "org.freedesktop.Telepathy.AccountManager",
                                                                    d->m_handle,
                                                                    this );

    QStringList object_path_elements = d->m_pInterface->path().split('/');
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Object path: " << d->m_pInterface->path();
    qDebug() << "Elements   : " << object_path_elements;
#endif
    d->m_connectionManagerName = object_path_elements.at( g_connectionManagerPosition );
    d->m_protocolName          = object_path_elements.at( g_protocolPosition );

#ifdef ENABLE_SALUT_WORKAROUND_
    // Salut workaround..
    // Salut has a corrupt object path that results in an invalid protocol name
    if ( d->m_protocolName == "local_2dxmpp" )
    { d->m_protocolName = "local_xmpp"; }
#endif

    // "-" is not allowed on as DBus object path. Thus "-" is mapped to "_".
    // Revert the mapping to get a correct protocol name
    d->m_protocolName.replace( '_', '-' );

    connect( d->m_pInterface, SIGNAL( AccountPropertyChanged( const QVariantMap& ) ),
             this, SLOT( slotPropertiesChanged( const QVariantMap& ) ) );
    connect( d->m_pInterface, SIGNAL( AccountPropertyChanged( const QVariantMap& ) ),
             this, SIGNAL( signalPropertiesChanged( const QVariantMap& ) ) );
    connect( d->m_pInterface, SIGNAL( Removed() ),
             this, SLOT( slotRemoved() ) );
}

#include "_gen/Account.h.moc"
