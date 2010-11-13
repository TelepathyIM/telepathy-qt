/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_pending_variant_h_HEADER_GUARD_
#define _TelepathyQt4_pending_variant_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>
#include <TelepathyQt4/PendingOperation>

#include <QVariant>

namespace Tp
{

class TELEPATHY_QT4_EXPORT PendingVariant : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingVariant);

public:
    PendingVariant(QDBusPendingCall call, const SharedPtr<RefCounted> &object);
    ~PendingVariant();

    QVariant result() const;

private Q_SLOTS:
    void watcherFinished(QDBusPendingCallWatcher*);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
