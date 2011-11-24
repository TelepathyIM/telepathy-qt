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

#ifndef _TelepathyQt_abstract_client_h_HEADER_GUARD_
#define _TelepathyQt_abstract_client_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

#include <QList>
#include <QObject>
#include <QSharedDataPointer>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class ClientRegistrar;
class ChannelClassSpecList;

class TP_QT_EXPORT AbstractClient : public RefCounted
{
    Q_DISABLE_COPY(AbstractClient)

public:
    AbstractClient();
    virtual ~AbstractClient();

    bool isRegistered() const;

private:
    friend class ClientRegistrar;

    void setRegistered(bool registered);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractClientObserver : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientObserver)

public:
    class ObserverInfo
    {
    public:
        ObserverInfo(const QVariantMap &info = QVariantMap());
        ObserverInfo(const ObserverInfo &other);
        ~ObserverInfo();

        ObserverInfo &operator=(const ObserverInfo &other);

        bool isRecovering() const { return qdbus_cast<bool>(allInfo().value(QLatin1String("recovering"))); }

        QVariantMap allInfo() const;

    private:
        struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    virtual ~AbstractClientObserver();

    ChannelClassSpecList observerFilter() const;

    bool shouldRecover() const;

    virtual void observeChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const ChannelDispatchOperationPtr &dispatchOperation,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const ObserverInfo &observerInfo) = 0;

protected:
    AbstractClientObserver(const ChannelClassSpecList &channelFilter, bool shouldRecover = false);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractClientApprover : public virtual AbstractClient
{
    Q_DISABLE_COPY(AbstractClientApprover)

public:
    virtual ~AbstractClientApprover();

    ChannelClassSpecList approverFilter() const;

    virtual void addDispatchOperation(const MethodInvocationContextPtr<> &context,
            const ChannelDispatchOperationPtr &dispatchOperation) = 0;

protected:
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
class TP_QT_EXPORT AbstractClientHandler : public virtual AbstractClient
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

        bool hasGTalkP2PNATTraversalToken() const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/gtalk-p2p"));
        }

        void setGTalkP2PNATTraversalToken()
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/gtalk-p2p"));
        }

        void unsetGTalkP2PNATTraversalToken()
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/gtalk-p2p"));
        }

        bool hasICEUDPNATTraversalToken() const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/ice-udp"));
        }

        void setICEUDPNATTraversalToken()
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/ice-udp"));
        }

        void unsetICEUDPNATTraversalToken()
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/ice-udp"));
        }

        bool hasWLM85NATTraversalToken() const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-8.5"));
        }

        void setWLM85NATTraversalToken()
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-8.5"));
        }

        void unsetWLM85NATTraversalToken()
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-8.5"));
        }

        bool hasWLM2009NATTraversalToken() const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-2009"));
        }

        void setWLM2009NATTraversalToken()
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-2009"));
        }

        void unsetWLM2009NATTraversalToken()
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/wlm-2009"));
        }

        bool hasAudioCodecToken(const QString &mimeSubType) const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/audio/") + mimeSubType.toLower());
        }

        void setAudioCodecToken(const QString &mimeSubType)
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/audio/") + mimeSubType.toLower());
        }

        void unsetAudioCodecToken(const QString &mimeSubType)
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/audio/") + mimeSubType.toLower());
        }

        bool hasVideoCodecToken(const QString &mimeSubType) const
        {
            return hasToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/video/") + mimeSubType.toLower());
        }

        void setVideoCodecToken(const QString &mimeSubType)
        {
            setToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
                QLatin1String("/video/") + mimeSubType.toLower());
        }

        void unsetVideoCodecToken(const QString &mimeSubType)
        {
            unsetToken(TP_QT_IFACE_CHANNEL_INTERFACE_MEDIA_SIGNALLING +
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

    class HandlerInfo
    {
    public:
        HandlerInfo(const QVariantMap &info = QVariantMap());
        HandlerInfo(const HandlerInfo &other);
        ~HandlerInfo();

        HandlerInfo &operator=(const HandlerInfo &other);

        QVariantMap allInfo() const;

    private:
        struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    virtual ~AbstractClientHandler();

    ChannelClassSpecList handlerFilter() const;

    Capabilities handlerCapabilities() const;

    virtual bool bypassApproval() const = 0;

    virtual void handleChannels(const MethodInvocationContextPtr<> &context,
            const AccountPtr &account,
            const ConnectionPtr &connection,
            const QList<ChannelPtr> &channels,
            const QList<ChannelRequestPtr> &requestsSatisfied,
            const QDateTime &userActionTime,
            const HandlerInfo &handlerInfo) = 0;

    bool wantsRequestNotification() const;
    virtual void addRequest(const ChannelRequestPtr &request);
    virtual void removeRequest(const ChannelRequestPtr &request,
            const QString &errorName, const QString &errorMessage);

protected:
    AbstractClientHandler(const ChannelClassSpecList &channelFilter,
            const Capabilities &capabilities = Capabilities(),
            bool wantsRequestNotification = false);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::AbstractClientObserver::ObserverInfo);
Q_DECLARE_METATYPE(Tp::AbstractClientHandler::Capabilities);
Q_DECLARE_METATYPE(Tp::AbstractClientHandler::HandlerInfo);

#endif
