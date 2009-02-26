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
#ifndef ConnectionFacade_H_
#define ConnectionFacade_H_

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#include <TelepathyQt4/Types>

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

namespace TpPrototype
{

/**
 * @defgroup qt_style_api Prototype Qt-Style Client API
 *
 * This API provides a high level client side API that enables developers
 * to access to Telepathy on a high level of abstraction. However, the
 * implementation is problematic in places.
 *
 * Work is in progress to merge the functionality of this prototype API into
 * the main part of telepathy-qt4 while converting it to asynchronous
 * operation. Using it for new code is not recommended.
 */

/**
 * @defgroup qt_convenience Convenience Classes
 * @ingroup qt_style_api
 * Classes that provide functions that are needed for various tasks.
 */

class ConnectionFacadePrivate;
class Connection;
class Account;

/**
 * @ingroup qt_convenience
 * Class to access to Telepathy and Mission control services. This class is used to encapsulate the low level D-BUS Interface
 * to Telepathy and MissionControl. It provides a series of helper functions.<br>
 * This class follows the <i>facade</i> pattern.
 * @todo Move as much functions as possible into adequate classes.
 */
class ConnectionFacade: public QObject
{
    Q_OBJECT
public:
    static ConnectionFacade* instance();
    /**
     * Returns a list of all connection managers registered.
     * @return List of connection manager names.
     */
    QStringList listOfConnectionManagers();

    /**
     * Returns a list of supported protocols of a connection manager.
     * @return List of supported protocols.
     */
    QStringList listOfProtocolsForConnectionManager( const QString& connectionManager );

    /**
     * Returns a Telepathy::ParamSpecList from the given protocol an connection manager.
     * @param connectionManager The connection manager for the protocol.
     * @param protocol Name of the protocol that is supported by the connection manager.
     * @return A map with all supported parameters with default values. 
     */
    Telepathy::ParamSpecList paramSpecListForConnectionManagerAndProtocol( const QString& connectionManager, const QString& protocol );
    
    /**
     * Returns a list of parameters for the given protocol and connection manager.
     * @param connectionManager The connection manager for the protocol.
     * @param protocol Name of the protocol that is supported by the connection manager.
     * @return A map with all supported parameters with default values.
     */
    QVariantMap parameterListForConnectionManagerAndProtocol( const QString& connectionManager, const QString& protocol );

    /**
     * Returns a list of parameters for the given protocol.
     * @deprecated Use parameterListForConnectionManagerAndProtocol() instead!
     * @param protocol Name of the protocol.
     * @return A map with all supported parameters with default values.
     */
    QVariantMap parameterListForProtocol( const QString& protocol, int account_number=1 ) ATTRIBUTE_DEPRECATED;

    /**
     * Connects to account. Connects an account to a service using the default connection manager.
     * @deprecated Use <i>Account::connection()</i> instead!
     * @param account The account to use.
     * @return A connection. A null pointer is returned if the connection was <i>not</i> successful.
     */
    TpPrototype::Connection* connectionWithAccount( Account* account, int account_number=1 ) ATTRIBUTE_DEPRECATED;

    /**
     *  Returns the self handle. The self handle is needed by various interfaces to request information about myself
     *  @todo: This is more or less a local function and should not be part of a public API. But I don't have a better place right now!
     *  @return The local handle is usually 1 but may change on demand. -1 is returned on error.
     */
    int selfHandleForConnectionInterface( Telepathy::Client::ConnectionInterface* connectionInterface );

private:
    ConnectionFacade( QObject* parent );
    ~ConnectionFacade();

    ConnectionFacadePrivate * const d;
    static ConnectionFacade* m_instance;
};

}
#endif // ConnectionFacade_H_

