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

#ifndef _TelepathyQt_channel_factory_h_HEADER_GUARD_
#define _TelepathyQt_channel_factory_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusProxyFactory>
#include <TelepathyQt/SharedPtr>
#include <TelepathyQt/Types>

// For Q_DISABLE_COPY
#include <QtGlobal>
#include <QString>
#include <QVariantMap>

class QDBusConnection;

namespace Tp
{

class ChannelClassSpec;

class TP_QT_EXPORT ChannelFactory : public DBusProxyFactory
{
    Q_OBJECT
    Q_DISABLE_COPY(ChannelFactory)

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    struct TP_QT_EXPORT Constructor : public RefCounted
    {
        virtual ~Constructor() {}

        virtual ChannelPtr construct(const ConnectionPtr &conn, const QString &objectPath,
                const QVariantMap &immutableProperties) const = 0;
    };
    typedef SharedPtr<Constructor> ConstructorPtr;
    typedef SharedPtr<const Constructor> ConstructorConstPtr;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

    static ChannelFactoryPtr create(const QDBusConnection &bus);

    virtual ~ChannelFactory();

    Features featuresForTextChats(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForTextChats(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForTextChats(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForTextChats(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForTextChats(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForTextChats(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForTextChatrooms(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForTextChatrooms(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForTextChatrooms(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForTextChatrooms(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForTextChatrooms(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForTextChatrooms(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForCalls(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForCalls(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    template<typename Subclass>
    void setSubclassForCalls(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForCalls(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForCalls(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    TP_QT_DEPRECATED Features featuresForStreamedMediaCalls(const QVariantMap &additionalProps = QVariantMap()) const;
    TP_QT_DEPRECATED void addFeaturesForStreamedMediaCalls(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    TP_QT_DEPRECATED ConstructorConstPtr constructorForStreamedMediaCalls(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    TP_QT_DEPRECATED void setSubclassForStreamedMediaCalls(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForStreamedMediaCalls(SubclassCtor<Subclass>::create(), additionalProps);
    }

    TP_QT_DEPRECATED void setConstructorForStreamedMediaCalls(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForRoomLists(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForRoomLists(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForRoomLists(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForRoomLists(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForRoomLists(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForRoomLists(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingFileTransfers(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingFileTransfers(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForOutgoingFileTransfers(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForOutgoingFileTransfers(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForOutgoingFileTransfers(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForOutgoingFileTransfers(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingFileTransfers(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingFileTransfers(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForIncomingFileTransfers(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForIncomingFileTransfers(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForIncomingFileTransfers(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForIncomingFileTransfers(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingStreamTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingStreamTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForOutgoingStreamTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForOutgoingStreamTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForOutgoingStreamTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForOutgoingStreamTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingStreamTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingStreamTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForIncomingStreamTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForIncomingStreamTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForIncomingStreamTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForIncomingStreamTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingRoomStreamTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingRoomStreamTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForOutgoingRoomStreamTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForOutgoingRoomStreamTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForOutgoingRoomStreamTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForOutgoingRoomStreamTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingRoomStreamTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingRoomStreamTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForIncomingRoomStreamTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForIncomingRoomStreamTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForIncomingRoomStreamTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForIncomingRoomStreamTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingDBusTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingDBusTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForOutgoingDBusTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForOutgoingDBusTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForOutgoingDBusTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForOutgoingDBusTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingDBusTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingDBusTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForIncomingDBusTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForIncomingDBusTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForIncomingDBusTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForIncomingDBusTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForOutgoingRoomDBusTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForOutgoingRoomDBusTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForOutgoingRoomDBusTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForOutgoingRoomDBusTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForOutgoingRoomDBusTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForOutgoingRoomDBusTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForIncomingRoomDBusTubes(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForIncomingRoomDBusTubes(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForIncomingRoomDBusTubes(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForIncomingRoomDBusTubes(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForIncomingRoomDBusTubes(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForIncomingRoomDBusTubes(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForContactSearches(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForContactSearches(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForContactSearches(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForContactSearches(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForContactSearches(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForContactSearches(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features featuresForServerAuthentication(const QVariantMap &additionalProps = QVariantMap()) const;
    void addFeaturesForServerAuthentication(const Features &features,
            const QVariantMap &additionalProps = QVariantMap());

    ConstructorConstPtr constructorForServerAuthentication(
            const QVariantMap &additionalProps = QVariantMap()) const;

    template<typename Subclass>
    void setSubclassForServerAuthentication(const QVariantMap &additionalProps = QVariantMap())
    {
        setConstructorForServerAuthentication(SubclassCtor<Subclass>::create(), additionalProps);
    }

    void setConstructorForServerAuthentication(const ConstructorConstPtr &ctor,
            const QVariantMap &additionalProps = QVariantMap());

    Features commonFeatures() const;
    void addCommonFeatures(const Features &features);

    ConstructorConstPtr fallbackConstructor() const;

    template <typename Subclass>
    void setFallbackSubclass()
    {
        setFallbackConstructor(SubclassCtor<Subclass>::create());
    }

    void setFallbackConstructor(const ConstructorConstPtr &ctor);

    Features featuresFor(const ChannelClassSpec &channelClass) const;
    void addFeaturesFor(const ChannelClassSpec &channelClass, const Features &features);

    template <typename Subclass>
    void setSubclassFor(const ChannelClassSpec &channelClass)
    {
        setConstructorFor(channelClass, SubclassCtor<Subclass>::create());
    }

    ConstructorConstPtr constructorFor(const ChannelClassSpec &channelClass) const;
    void setConstructorFor(const ChannelClassSpec &channelClass, const ConstructorConstPtr &ctor);

    PendingReady *proxy(const ConnectionPtr &connection, const QString &channelPath,
            const QVariantMap &immutableProperties) const;

protected:
    ChannelFactory(const QDBusConnection &bus);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template <typename Subclass>
    struct SubclassCtor : public Constructor
    {
        static ConstructorPtr create()
        {
            return ConstructorPtr(new SubclassCtor<Subclass>());
        }

        virtual ~SubclassCtor() {}

        ChannelPtr construct(const ConnectionPtr &conn, const QString &objectPath,
                const QVariantMap &immutableProperties) const
        {
            return Subclass::create(conn, objectPath, immutableProperties);
        }
    };
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    virtual Features featuresFor(const DBusProxyPtr &proxy) const;

private:
    struct Private;
    Private *mPriv;
};

} // Tp

#endif
