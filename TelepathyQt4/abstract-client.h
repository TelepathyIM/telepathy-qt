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

#ifndef _TelepathyQt4_abstract_client_h_HEADER_GUARD_
#define _TelepathyQt4_abstract_client_h_HEADER_GUARD_

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

class TELEPATHY_QT4_EXPORT AbstractClient : public RefCounted
{
    Q_DISABLE_COPY(AbstractClient)

public:
    AbstractClient();
    virtual ~AbstractClient();
};

class TELEPATHY_QT4_EXPORT AbstractClientObserver : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientObserver)

public:
    virtual ~AbstractClientObserver();

    ChannelClassList observerChannelFilter() const;

    bool recover() const;

    virtual void observeChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QVariantMap &observerInfo) = 0;

protected:
    AbstractClientObserver(const ChannelClassList &channelFilter);
    AbstractClientObserver(const ChannelClassList &channelFilter,
            bool recover);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT AbstractClientApprover : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientApprover)

public:
    virtual ~AbstractClientApprover();

    ChannelClassList approverChannelFilter() const;

    virtual void addDispatchOperation(const MethodInvocationContextPtr<> &context,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation) = 0;

protected:
    AbstractClientApprover(const ChannelClassList &channelFilter);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT AbstractClientHandler : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientHandler)

public:
    virtual ~AbstractClientHandler();

    ChannelClassList handlerChannelFilter() const;

    QStringList capabilities() const;

    virtual bool bypassApproval() const = 0;

    virtual void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const QVariantMap &handlerInfo) = 0;

    bool wantsRequestNotification() const;
    virtual void addRequest(const ChannelRequestPtr &request);
    virtual void removeRequest(const ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage);

protected:
    AbstractClientHandler(const ChannelClassList &channelFilter,
            bool wantsRequestNotification = false);
    AbstractClientHandler(const ChannelClassList &channelFilter,
            const QStringList &capabilities,
            bool wantsRequestNotification = false);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
