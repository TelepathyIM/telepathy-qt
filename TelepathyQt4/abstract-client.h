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

#ifndef _TelepathyQt4_cli_abstract_client_h_HEADER_GUARD_
#define _TelepathyQt4_cli_abstract_client_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class PendingClientOperation;

class AbstractClient : public QObject, public RefCounted
{
    Q_OBJECT

public:
    virtual ~AbstractClient();

protected:
    AbstractClient();

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class AbstractClientHandler : public AbstractClient
{
    Q_OBJECT

public:
    virtual ~AbstractClientHandler();

    ChannelClassList channelFilter() const;
    virtual bool bypassApproval() const = 0;
    virtual QList<ChannelPtr> handledChannels() const = 0;

    virtual void handleChannels(PendingClientOperation *operation,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const QVariantMap &handlerInfo) = 0;

    bool isListeningRequests() const;
    virtual void addRequest(const QString &requestObjectPath,
            const QVariantMap &requestProperties);
    virtual void removeRequest(const QString &requestObjectPath,
            const QString &errorName, const QString &errorMessage);

protected:
    AbstractClientHandler(const ChannelClassList &channelFilter,
            bool listenRequests = false);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
