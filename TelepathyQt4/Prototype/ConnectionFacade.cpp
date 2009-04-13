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

#include "TelepathyQt4/Prototype/ConnectionFacade.h"

#include <QCoreApplication>
#include <QDebug>
#include <QPointer>

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionManager>

#include <TelepathyQt4/Prototype/Account.h>
#include <TelepathyQt4/Prototype/AccountManager.h>
#include <TelepathyQt4/Prototype/Connection.h>
#include <TelepathyQt4/Prototype/DBusInterface.h>

using namespace TpPrototype;

ConnectionFacade* ConnectionFacade::m_instance = NULL;

enum {
    mgr_Param_Flag_Required = 1,
    mgr_Param_Flag_Register = 2,
    mgr_Param_Flag_Has_Default = 4,
    mgr_Param_Flag_Secret = 8
};
    

class TpPrototype::ConnectionFacadePrivate
{
public:
};

ConnectionFacade* ConnectionFacade::instance()
{
    if ( NULL == m_instance )
    {
        m_instance = new ConnectionFacade( QCoreApplication::instance() );
    }

    return m_instance;
}

ConnectionFacade::ConnectionFacade( QObject* parent ):
    QObject( parent ),
    d( new ConnectionFacadePrivate )
{
}

ConnectionFacade::~ConnectionFacade()
{
    delete d;
}

QStringList ConnectionFacade::listOfConnectionManagers()
{
    Tp::registerTypes();

    QStringList ret_list;
    
    DBusInterface* interface = new DBusInterface( NULL );
    
    QDBusReply<QStringList> reply = interface->listActivatableNames();
    
    delete interface;
    
    if ( !reply.isValid() )
    {
        QDBusError error = reply.error();
    
        qWarning() << "Disconnect: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();
                   
        return ret_list; // returns empty list
    }
    
    
    foreach( const QString& name, reply.value() )
    {
        // qDebug() << "+++Name: " << name;
        if ( !name.startsWith( "org.freedesktop.Telepathy.ConnectionManager" ) 
             || name.isEmpty() )
        { continue; }
        
        QString cm_name;
        int pos_of_last_dot = name.lastIndexOf( "." );
        Q_ASSERT( pos_of_last_dot >= 0 );
        cm_name = name.right( name.length() - ( pos_of_last_dot + 1 ) );
        ret_list << cm_name;
    }
        
    // qDebug() << "+++Names found: " << ret_list;
    return ret_list;
}

QStringList ConnectionFacade::listOfProtocolsForConnectionManager( const QString& connectionManager )
{
    Tp::registerTypes();

    QStringList empty_list;

    QStringList list_of_connection_managers = listOfConnectionManagers();
    if ( !list_of_connection_managers.contains( connectionManager ) )
    { return empty_list; } // return empty list

    Tp::Client::ConnectionManagerInterface connection_manager_interface( "org.freedesktop.Telepathy.ConnectionManager." + connectionManager,
                                                                                "/org/freedesktop/Telepathy/ConnectionManager/" + connectionManager );
    
    QDBusPendingReply<QStringList> list_protocols_reply = connection_manager_interface.ListProtocols();
    list_protocols_reply.waitForFinished();

    if ( !list_protocols_reply.isValid() )
    {
        QDBusError error = list_protocols_reply.error();
    
        qWarning() << "ListProtocols: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();
                   
        return empty_list; // returns empty list
    }
   
    return list_protocols_reply.value(); 
}

// TODO: Return the parameter list with all flags as we receive it from telepathy. This solution hides the flags and is unable
//       to handle all required types..
Tp::ParamSpecList ConnectionFacade::paramSpecListForConnectionManagerAndProtocol( const QString& connectionManager, const QString& protocol )
{
    Tp::registerTypes();

    Tp::Client::ConnectionManagerInterface connection_manager_interface( "org.freedesktop.Telepathy.ConnectionManager." + connectionManager,
                                                                                "/org/freedesktop/Telepathy/ConnectionManager/" + connectionManager );
    
    QDBusPendingReply<Tp::ParamSpecList> get_parameters_reply = connection_manager_interface.GetParameters( protocol );
    get_parameters_reply.waitForFinished();

    if ( !get_parameters_reply.isValid() )
    {
        QDBusError error = get_parameters_reply.error();
    
        qWarning() << "ListProtocols: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();
                   
        return Tp::ParamSpecList() ; // returns empty list
    }

    Tp::ParamSpecList param_spec_list = get_parameters_reply.value();

    return param_spec_list;
}

QVariantMap ConnectionFacade::parameterListForConnectionManagerAndProtocol( const QString& connectionManager, const QString& protocol )
{
    QVariantMap ret_map;
            
    foreach( const Tp::ParamSpec& item, paramSpecListForConnectionManagerAndProtocol(connectionManager, protocol)  )
    {
        ret_map.insert( item.name, item.defaultValue.variant() );
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "item: " << item.name << "flag:" << item.flags;
#endif
        if ( ( item.flags & ( mgr_Param_Flag_Required | mgr_Param_Flag_Register ) )
               && item.defaultValue.variant().toString().isEmpty() )
        {

            if ( item.defaultValue.variant().type() == QVariant::String )
            {
                ret_map.insert( item.name, QVariant("required") );
            }
        }
    }

    return ret_map;
}

QVariantMap ConnectionFacade::parameterListForProtocol( const QString& protocol, int account_number ) 
{
    Tp::registerTypes();

    QVariantMap ret_map;
    
    if (account_number==1)
    {
        ret_map.insert( "account", "basyskom@localhost" );
    }
    else  if (account_number==2)
    {
        ret_map.insert( "account", "test@localhost" );
    }
    ret_map.insert( "password", "basyskom" );
    ret_map.insert( "server", "localhost" );
    ret_map.insert( "resource", "Tp" );
    ret_map.insert( "port", static_cast<uint>(5222) );    
    return ret_map;
}

// account_number is used for test purposes only we have to delte this later
TpPrototype::Connection* ConnectionFacade::connectionWithAccount( Account* account, int account_number )
{
    Tp::registerTypes();

    if ( !account )
    { return NULL; }

    account->setParameters( parameterListForProtocol( "jabber", account_number  ) );
    
    // Get the default connection manager. "Default" is currently only the first one in the list of possible connection managers
    QStringList connection_managers = listOfConnectionManagers();
    Q_ASSERT( connection_managers.count() );
    if ( !connection_managers.count() )
    { return NULL; }
    
    return account->connection();
}

int ConnectionFacade::selfHandleForConnectionInterface( Tp::Client::ConnectionInterface* connectionInterface )
{
    if ( !connectionInterface )
    {
        return -1;
    }

    QDBusPendingReply<uint> local_handle_reply = connectionInterface->GetSelfHandle();
    local_handle_reply.waitForFinished();

    if ( !local_handle_reply.isValid() )
    {
        QDBusError error = local_handle_reply.error();

        qWarning() << "GetSelfHandle: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        return -1;
    }
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "****Self handle:" << local_handle_reply.value();
#endif
    
    return local_handle_reply.value();
}

#include "_gen/ConnectionFacade.h.moc"
