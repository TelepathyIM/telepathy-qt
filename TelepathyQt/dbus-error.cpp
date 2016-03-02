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

#include <TelepathyQt/DBusError>

#include <QString>

namespace Tp
{

struct TP_QT_NO_EXPORT DBusError::Private
{
    Private(const QString &name, const QString &message)
        : name(name),
          message(message)
    {
    }

    QString name;
    QString message;
};

/**
 * \class DBusError
 * \ingroup servicesideimpl
 * \headerfile TelepathyQt/dbus-error.h <TelepathyQt/DBusError>
 *
 * \brief Small container class, containing a D-Bus error
 */

/**
 * Construct an empty DBusError
 */
DBusError::DBusError()
    : mPriv(0)
{
}

/**
 * Construct a DBusError with the given error \a name and \a message.
 *
 * \param name The D-Bus error name.
 * \param message A human-readable description of the error.
 */
DBusError::DBusError(const QString &name, const QString &message)
    : mPriv(new Private(name, message))
{
}

/**
 * Class destructor.
 */
DBusError::~DBusError()
{
    if (mPriv) {
        delete mPriv;
    }
}

/**
 * Compare this error with another one.
 *
 * \param other The other error to compare to.
 * \return \c true if the two errors have the same name and message
 * or \c false otherwise.
 */
bool DBusError::operator==(const DBusError &other) const
{
    if (!isValid() && !other.isValid()) {
        return true;
    } else if (!isValid() || !other.isValid()) {
        return false;
    }

    return mPriv->name == other.mPriv->name &&
        mPriv->message == other.mPriv->message;
}

/**
 * Compare this error with another one.
 *
 * \param other The other error to compare to.
 * \return \c false if the two errors have the same name and message
 * or \c true otherwise.
 */
bool DBusError::operator!=(const DBusError &other) const
{
    return !(*this == other);
}

/**
 * Return the D-Bus name of this error.
 *
 * \return The D-Bus name of this error.
 */
QString DBusError::name() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->name;
}

/**
 * Return the human-readable description of the error.
 *
 * \return The human-readable description of the error.
 */
QString DBusError::message() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->message;
}

/**
 * Set this DBusError to contain the given error \a name and \a message.
 *
 * \param name The D-Bus error name to set.
 * \param message The description of the error to set.
 */
void DBusError::set(const QString &name, const QString &message)
{
    if (!isValid()) {
        mPriv = new Private(name, message);
        return;
    }

    mPriv->name = name;
    mPriv->message = message;
}

/**
 * \fn bool DBusError::isValid() const
 *
 * Return whether this DBusError is set to contain an error or not.
 *
 * \return \c true if the error name and message have been set,
 * or \c false otherwise.
 */

} // Tp
