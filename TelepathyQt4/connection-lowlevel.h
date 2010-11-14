/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt4_connection_lowlevel_h_HEADER_GUARD_
#define _TelepathyQt4_connection_lowlevel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

class Connection;
class PendingChannel;
class PendingContactAttributes;
class PendingHandles;
class PendingOperation;
class PendingReady;

class TELEPATHY_QT4_EXPORT ConnectionLowlevel : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(ConnectionLowlevel)

public:
    ~ConnectionLowlevel();

    bool isValid() const;
    ConnectionPtr connection() const;

    SimpleStatusSpecMap allowedPresenceStatuses() const;
    PendingOperation *setSelfPresence(const QString &status, const QString &statusMessage);

private:
    friend class Connection;

    ConnectionLowlevel(Connection *parent);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
