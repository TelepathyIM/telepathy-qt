/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_contact_search_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_contact_search_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/PendingOperation>

#include <QDBusPendingCall>

namespace Tp
{

class TELEPATHY_QT4_NO_EXPORT ContactSearchChannel::PendingSearch : public PendingOperation
{
    Q_OBJECT

public:
    PendingSearch(const ContactSearchChannelPtr &chan, QDBusPendingCall call);

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onSearchStateChanged(Tp::ChannelContactSearchState state, const QString &errorName,
            const Tp::ContactSearchChannel::SearchStateChangeDetails &details);
    TELEPATHY_QT4_NO_EXPORT void watcherFinished(QDBusPendingCallWatcher*);

private:
    bool mFinished;
    QDBusError mError;
};

} // Tp

#endif
