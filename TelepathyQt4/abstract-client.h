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

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

#include <QList>
#include <QObject>
#include <QSharedDataPointer>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class ChannelClassSpecList;

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

    TELEPATHY_QT4_DEPRECATED ChannelClassList observerChannelFilter() const;
    ChannelClassSpecList observerFilter() const;

    bool shouldRecover() const;

    // FIXME: (API/ABI break) Use high-level class for observerInfo
    virtual void observeChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QVariantMap &observerInfo) = 0;

protected:
    TELEPATHY_QT4_DEPRECATED AbstractClientObserver(const ChannelClassList &channelFilter);
    TELEPATHY_QT4_DEPRECATED AbstractClientObserver(const ChannelClassList &channelFilter,
            bool shouldRecover);

    AbstractClientObserver(const ChannelClassSpecList &channelFilter, bool shouldRecover = false);

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

    TELEPATHY_QT4_DEPRECATED ChannelClassList approverChannelFilter() const;
    ChannelClassSpecList approverFilter() const;

    virtual void addDispatchOperation(const MethodInvocationContextPtr<> &context,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation) = 0;

protected:
    TELEPATHY_QT4_DEPRECATED AbstractClientApprover(const ChannelClassList &channelFilter);
    AbstractClientApprover(const ChannelClassSpecList &channelFilter);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

/*
 * TODO: use case specific subclasses:
 *  - StreamTubeHandler(QString(List) protocol(s))
 *    - handleTube(DBusTubeChannelPtr, userActionTime)
 *  - DBusTubeHandler(QString(List) serviceName(s))
 *    - handleTube(DBusTubeChannelPtr, userActionTime)
 */
class TELEPATHY_QT4_EXPORT AbstractClientHandler : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientHandler)

public:
    class Capabilities
    {
        public:
            Capabilities(const QStringList &tokens = QStringList());
            Capabilities(const Capabilities &other);
            ~Capabilities();

            Capabilities &operator=(const Capabilities &other);

            bool hasGTalkP2PNATTraversal() const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/gtalk-p2p"));
            }

            void setGTalkP2PNATTraversal()
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/gtalk-p2p"));
            }

            void unsetGTalkP2PNATTraversal()
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/gtalk-p2p"));
            }

            bool hasICEUDPNATTraversal() const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/ice-udp"));
            }

            void setICEUDPNATTraversal()
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/ice-udp"));
            }

            void unsetICEUDPNATTraversal()
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/ice-udp"));
            }

            bool hasWLM85NATTraversal() const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-8.5"));
            }

            void setWLM85NATTraversal()
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-8.5"));
            }

            void unsetWLM85NATTraversal()
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-8.5"));
            }

            bool hasWLM2009NATTraversal() const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-2009"));
            }

            void setWLM2009NATTraversal()
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-2009"));
            }

            void unsetWLM2009NATTraversal()
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/wlm-2009"));
            }

            bool hasAudioCodec(const QString &mimeSubType) const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/audio/") + mimeSubType.toLower());
            }

            void setAudioCodec(const QString &mimeSubType)
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/audio/") + mimeSubType.toLower());
            }

            void unsetAudioCodec(const QString &mimeSubType)
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/audio/") + mimeSubType.toLower());
            }

            bool hasVideoCodec(const QString &mimeSubType) const
            {
                return hasToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/video/") + mimeSubType.toLower());
            }

            void setVideoCodec(const QString &mimeSubType)
            {
                setToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/video/") + mimeSubType.toLower());
            }

            void unsetVideoCodec(const QString &mimeSubType)
            {
                unsetToken(TP_QT4_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                    QLatin1String("/video/") + mimeSubType.toLower());
            }

            bool hasToken(const QString &token) const;
            void setToken(const QString &token);
            void unsetToken(const QString &token);

            QStringList allTokens() const;

        private:

            struct Private;
            QSharedDataPointer<Private> mPriv;
    };

    virtual ~AbstractClientHandler();

    TELEPATHY_QT4_DEPRECATED ChannelClassList handlerChannelFilter() const;
    ChannelClassSpecList handlerFilter() const;

    // FIXME: (API/ABI break) Use high-level class for capabilities
    QStringList capabilities() const;

    virtual bool bypassApproval() const = 0;

    // FIXME: (API/ABI break) Use high-level class for handlerInfo
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
    TELEPATHY_QT4_DEPRECATED AbstractClientHandler(const ChannelClassList &channelFilter,
            bool wantsRequestNotification = false);
    TELEPATHY_QT4_DEPRECATED AbstractClientHandler(const ChannelClassList &channelFilter,
            const QStringList &capabilities,
            bool wantsRequestNotification = false);

    AbstractClientHandler(const ChannelClassSpecList &channelFilter,
            const Capabilities &capabilities = Capabilities(),
            bool wantsRequestNotification = false);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
