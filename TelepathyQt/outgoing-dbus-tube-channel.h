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

#ifndef _TelepathyQt_outgoing_dbus_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt_outgoing_dbus_tube_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusTubeChannel>
#include <TelepathyQt/PendingOperation>


namespace Tp {

class PendingString;

struct PendingDBusTubeOfferPrivate;
class TP_QT_EXPORT PendingDBusTubeOffer : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingDBusTubeOffer)

public:
    virtual ~PendingDBusTubeOffer();

    QDBusServer *server();

private:
    PendingDBusTubeOffer(PendingString *string, const OutgoingDBusTubeChannelPtr &object);
    PendingDBusTubeOffer(const QString &errorName, const QString &errorMessage,
                         const OutgoingDBusTubeChannelPtr &object);

    friend struct PendingDBusTubeOfferPrivate;
    PendingDBusTubeOfferPrivate *mPriv;

    friend class OutgoingDBusTubeChannel;

// private slots:
    Q_PRIVATE_SLOT(mPriv, void __k__onOfferFinished(Tp::PendingOperation*))
    Q_PRIVATE_SLOT(mPriv, void __k__onTubeStateChanged(Tp::TubeChannelState))
};

class OutgoingDBusTubeChannelPrivate;
class TP_QT_EXPORT OutgoingDBusTubeChannel : public Tp::DBusTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(OutgoingDBusTubeChannel)
    Q_DECLARE_PRIVATE(OutgoingDBusTubeChannel)

    friend struct PendingDBusTubeOfferPrivate;

public:
    static OutgoingDBusTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~OutgoingDBusTubeChannel();

    PendingDBusTubeOffer *offerTube(const QVariantMap &parameters, bool requireCredentials = false);

    QDBusServer *server();

protected:
    OutgoingDBusTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

};

}

#endif // TP_OUTGOING_DBUS_TUBE_CHANNEL_H
