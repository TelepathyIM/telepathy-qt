/*
 * This file is part of TelepathyQt4
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

#ifndef _TelepathyQt_pending_dbus_tube_offer_h_HEADER_GUARD_
#define _TelepathyQt_pending_dbus_tube_offer_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/OutgoingDBusTubeChannel>

namespace Tp {

class PendingString;

class TP_QT_EXPORT PendingDBusTubeOffer : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingDBusTubeOffer)

public:
    virtual ~PendingDBusTubeOffer();

    QString address() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onOfferFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onTubeStateChanged(Tp::TubeChannelState state);

private:
    PendingDBusTubeOffer(PendingString *string, const OutgoingDBusTubeChannelPtr &object);
    PendingDBusTubeOffer(const QString &errorName, const QString &errorMessage,
                         const OutgoingDBusTubeChannelPtr &object);

    struct Private;
    friend class OutgoingDBusTubeChannel;
    friend struct Private;
    Private *mPriv;
};

}

#endif
