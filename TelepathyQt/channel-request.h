/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt_channel_request_h_HEADER_GUARD_
#define _TelepathyQt_channel_request_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-channel-request.h>

#include <TelepathyQt/Constants>
#include <TelepathyQt/DBus>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/Feature>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class ChannelRequestHints;
class PendingOperation;

class TP_QT_EXPORT ChannelRequest : public StatefulDBusProxy,
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

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onAccountReady(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onLegacySucceeded();
    TP_QT_NO_EXPORT void onSucceededWithChannel(const QDBusObjectPath &connPath, const QVariantMap &connProps,
            const QDBusObjectPath &chanPath, const QVariantMap &chanProps);
    TP_QT_NO_EXPORT void onChanBuilt(Tp::PendingOperation *op);

private:
    friend class PendingChannelRequest;

    PendingOperation *proceed();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT ChannelRequestHints
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
