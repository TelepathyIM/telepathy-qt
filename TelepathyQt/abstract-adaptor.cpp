/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
 * @license LGPL 2.1
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

#include <TelepathyQt/AbstractAdaptor>

#include "TelepathyQt/_gen/abstract-adaptor.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <QDBusConnection>

namespace Tp
{

struct TP_QT_NO_EXPORT AbstractAdaptor::Private
{
    Private(const QDBusConnection &dbusConnection, QObject *adaptee)
        : dbusConnection(dbusConnection),
          adaptee(adaptee)
    {
    }

    QDBusConnection dbusConnection;
    QObject *adaptee;
};

/**
 * \class AbstractAdaptor
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/abstract-adaptor.h <TelepathyQt/AbstractAdaptor>
 *
 * \brief Base class for all the low-level service-side adaptors.
 *
 * This class serves as the parent for all the generated low-level service-side
 * adaptors. Adaptors provide the interface of an object on the bus.
 *
 * The implementation of this interface should be provided in a special object
 * called the adaptee. The adaptee is meant to provide properties, signals
 * and slots that are connected automatically with the adaptor using Qt's meta-object
 * system.
 */

/**
 * Construct a new AbstractAdaptor that operates on the given
 * \a dbusConnection and redirects calls to the given \a adaptee.
 *
 * \param dbusConnection The D-Bus connection to use.
 * \param adaptee The class the provides the implementation of the calls.
 * \param parent The QObject parent of this adaptor.
 */
AbstractAdaptor::AbstractAdaptor(const QDBusConnection &dbusConnection,
        QObject *adaptee, QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mPriv(new Private(dbusConnection, adaptee))
{
    setAutoRelaySignals(false);
}

/**
 * Class destructor.
 */
AbstractAdaptor::~AbstractAdaptor()
{
    delete mPriv;
}

/**
 * Return the D-Bus connection associated with this adaptor.
 *
 * \return The D-Bus connection associated with this adaptor.
 */
QDBusConnection AbstractAdaptor::dbusConnection() const
{
    return mPriv->dbusConnection;
}

/**
 * Return the adaptee object, i.e. the object that provides the implementation
 * of this adaptor.
 *
 * \return The adaptee object.
 */
QObject *AbstractAdaptor::adaptee() const
{
    return mPriv->adaptee;
}

}
