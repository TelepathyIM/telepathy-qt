/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#ifndef _TelepathyQt_pending_ready_h_HEADER_GUARD_
#define _TelepathyQt_pending_ready_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusProxyFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/SharedPtr>

#include <QSet>

namespace Tp
{

class TP_QT_EXPORT PendingReady: public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingReady);

public:
    ~PendingReady();

    DBusProxyPtr proxy() const;

    Features requestedFeatures() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onNestedFinished(Tp::PendingOperation *);

private:
    friend class Connection;
    friend class DBusProxyFactory;
    friend class ReadinessHelper;

    TP_QT_NO_EXPORT PendingReady(const SharedPtr<RefCounted> &object, const Features &requestedFeatures);
    TP_QT_NO_EXPORT PendingReady(const SharedPtr<DBusProxyFactory> &factory,
            const DBusProxyPtr &proxy, const Features &requestedFeatures);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
