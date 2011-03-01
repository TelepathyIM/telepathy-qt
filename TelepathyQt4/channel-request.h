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

#ifndef _TelepathyQt4_channel_request_h_HEADER_GUARD_
#define _TelepathyQt4_channel_request_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-channel-request.h>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/Feature>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class ChannelRequestHints;
class PendingOperation;

class TELEPATHY_QT4_EXPORT ChannelRequest : public StatefulDBusProxy,
                public OptionalInterfaceFactory<ChannelRequest>
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelRequest)

public:
    static const Feature FeatureCore;

    static ChannelRequestPtr create(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    static ChannelRequestPtr create(const AccountPtr &account,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~ChannelRequest();

    AccountPtr account() const;
    QDateTime userActionTime() const;
    QString preferredHandler() const;
    QualifiedPropertyValueMapList requests() const;
    ChannelRequestHints hints() const;

    QVariantMap immutableProperties() const;

    PendingOperation *cancel();

    ChannelPtr channel() const;

Q_SIGNALS:
    void failed(const QString &errorName, const QString &errorMessage);
    void succeeded(); // TODO API/ABI break: remove
    void succeeded(const Tp::ChannelPtr &channel);

protected:
    ChannelRequest(const QDBusConnection &bus,
            const QString &objectPath, const QVariantMap &immutableProperties,
            const AccountFactoryConstPtr &accountFactory,
            const ConnectionFactoryConstPtr &connectionFactory,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    ChannelRequest(const AccountPtr &account,
            const QString &objectPath, const QVariantMap &immutableProperties);

    Client::ChannelRequestInterface *baseInterface() const;

protected:
    // TODO: (API/ABI break) Remove connectNotify
    void connectNotify(const char *);

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TELEPATHY_QT4_NO_EXPORT void onAccountReady(Tp::PendingOperation *op);

    TELEPATHY_QT4_NO_EXPORT void onLegacySucceeded();
    TELEPATHY_QT4_NO_EXPORT void onSucceededWithChannel(const QDBusObjectPath &connPath, const QVariantMap &connProps,
            const QDBusObjectPath &chanPath, const QVariantMap &chanProps);
    TELEPATHY_QT4_NO_EXPORT void onChanBuilt(Tp::PendingOperation *op);

private:
    friend class PendingChannelRequest;

    PendingOperation *proceed();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TELEPATHY_QT4_EXPORT ChannelRequestHints
{
public:

    ChannelRequestHints();
    ChannelRequestHints(const QVariantMap &hints);
    ChannelRequestHints(const ChannelRequestHints &other);
    ~ChannelRequestHints();

    ChannelRequestHints &operator=(const ChannelRequestHints &other);

    bool isValid() const;

    bool hasHint(const QString &reversedDomain, const QString &localName) const;
    QVariant hint(const QString &reversedDomain, const QString &localName) const;
    void setHint(const QString &reversedDomain, const QString &localName, const QVariant &value);

    QVariantMap allHints() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::ChannelRequestHints);

#endif
