/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_incoming_dbus_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt_incoming_dbus_tube_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusTubeChannel>
#include <TelepathyQt/PendingOperation>

namespace Tp {

class PendingString;

class PendingDBusTubeAcceptPrivate;
class TP_QT_EXPORT PendingDBusTubeAccept : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingDBusTubeAccept)

public:
    virtual ~PendingDBusTubeAccept();

    QDBusConnection connection() const;

private:
    PendingDBusTubeAccept(PendingString *string, const IncomingDBusTubeChannelPtr &object);
    PendingDBusTubeAccept(const QString &errorName, const QString &errorMessage,
                          const IncomingDBusTubeChannelPtr &object);

    friend class PendingDBusTubeAcceptPrivate;
    PendingDBusTubeAcceptPrivate *mPriv;

    friend class IncomingDBusTubeChannel;

// private slots:
    Q_PRIVATE_SLOT(mPriv, void onAcceptFinished(Tp::PendingOperation*))
    Q_PRIVATE_SLOT(mPriv, void onStateChanged(Tp::TubeChannelState))
};


class IncomingDBusTubeChannelPrivate;
class TP_QT_EXPORT IncomingDBusTubeChannel : public DBusTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(IncomingDBusTubeChannel)
    Q_DECLARE_PRIVATE(IncomingDBusTubeChannel)

    friend class PendingDBusTubeAcceptPrivate;

public:
    static IncomingDBusTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~IncomingDBusTubeChannel();

    PendingDBusTubeAccept *acceptTube(bool requireCredentials = false);

    QDBusConnection connection() const;

protected:
    IncomingDBusTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

};

}

#endif
