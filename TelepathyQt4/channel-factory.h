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

#ifndef _TelepathyQt4_channel_factory_h_HEADER_GUARD_
#define _TelepathyQt4_channel_factory_h_HEADER_GUARD_

#include <TelepathyQt4/DBusProxyFactory>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

// For Q_DISABLE_COPY
#include <QtGlobal>
#include <QString>
#include <QVariantMap>

class QDBusConnection;

namespace Tp
{

class ChannelClassSpec;

class TELEPATHY_QT4_EXPORT ChannelFactory : public DBusProxyFactory
{
    Q_DISABLE_COPY(ChannelFactory)

public:
    static ChannelFactoryPtr create(const QDBusConnection &bus);

    virtual ~ChannelFactory();

    Features featuresForTextChats(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForTextChats(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForTextChatrooms(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForTextChatrooms(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForMediaCalls(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForMediaCalls(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingFileTransfers(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingFileTransfers(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingFileTransfers(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingFileTransfers(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    // When merged, Tube channels should have export/import variants too like FT has for send/receive

    Features commonFeatures() const;
    void addCommonFeatures(const Features &features);

    Features featuresFor(const ChannelClassSpec &channelClass) const;
    void addFeaturesFor(const ChannelClassSpec &channelClass, const Features &features);

    PendingReady *proxy(const ConnectionPtr &connection, const QString &channelPath,
            const QVariantMap &immutableProperties) const;

protected:
    ChannelFactory(const QDBusConnection &bus);

    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    virtual Features featuresFor(const SharedPtr<RefCounted> &proxy) const;

private:
    // TODO: remove

    friend class ChannelDispatchOperation;
    friend class ClientHandlerAdaptor;
    friend class ClientObserverAdaptor;
    friend class PendingChannel;

    static ChannelPtr create(const ConnectionPtr &connection,
            const QString &channelPath, const QVariantMap &immutableProperties);

    struct Private;
    Private *mPriv;
};

} // Tp

#endif
