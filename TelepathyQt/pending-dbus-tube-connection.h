/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_pending_dbus_tube_connection_h_HEADER_GUARD_
#define _TelepathyQt_pending_dbus_tube_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/OutgoingDBusTubeChannel>

namespace Tp {

class PendingString;

class TP_QT_EXPORT PendingDBusTubeConnection : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingDBusTubeConnection)

public:
    virtual ~PendingDBusTubeConnection();

    QString address() const;

    bool allowsOtherUsers() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onConnectionFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onStateChanged(Tp::TubeChannelState state);
    TP_QT_NO_EXPORT void onChannelInvalidated(Tp::DBusProxy *proxy,
                                        const QString &errorName,
                                        const QString &errorMessage);

private:
    TP_QT_NO_EXPORT PendingDBusTubeConnection(PendingString *string,
            bool allowOtherUsers, const QVariantMap &parameters,
            const DBusTubeChannelPtr &object);
    TP_QT_NO_EXPORT PendingDBusTubeConnection(const QString &errorName, const QString &errorMessage,
            const DBusTubeChannelPtr &object);

    struct Private;
    friend class OutgoingDBusTubeChannel;
    friend class IncomingDBusTubeChannel;
    friend struct Private;
    Private *mPriv;
};

}

#endif
