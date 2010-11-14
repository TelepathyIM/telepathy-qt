/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008, 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008, 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_connection_h_HEADER_GUARD_
#define _TelepathyQt4_connection_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/_gen/cli-connection.h>

#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/DBusProxy>
#include <TelepathyQt4/OptionalInterfaceFactory>
#include <TelepathyQt4/ReadinessHelper>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/SharedPtr>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QSet>
#include <QString>
#include <QStringList>

namespace Tp
{

class Channel;
class ConnectionCapabilities;
class ConnectionLowlevel;
class Contact;
class ContactManager;
class PendingChannel;
class PendingContactAttributes;
class PendingHandles;
class PendingOperation;
class PendingReady;

class TELEPATHY_QT4_EXPORT Connection : public StatefulDBusProxy,
                   public OptionalInterfaceFactory<Connection>
{
    Q_OBJECT
    Q_DISABLE_COPY(Connection)

public:
    static const Feature FeatureCore;
    static const Feature FeatureSelfContact;
    static const Feature FeatureSimplePresence;
    static const Feature FeatureRoster;
    static const Feature FeatureRosterGroups;
    static const Feature FeatureAccountBalance; // TODO unit tests for this

    static ConnectionPtr create(const QString &busName,
            const QString &objectPath,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);
    static ConnectionPtr create(const QDBusConnection &bus,
            const QString &busName, const QString &objectPath,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    virtual ~Connection();

    ChannelFactoryConstPtr channelFactory() const;
    ContactFactoryConstPtr contactFactory() const;

    QString cmName() const;
    QString protocolName() const;

    ConnectionStatus status() const;
    ConnectionStatusReason statusReason() const;

    class ErrorDetails
    {
        public:
            ErrorDetails();
            ErrorDetails(const QVariantMap &details);
            ErrorDetails(const ErrorDetails &other);
            ~ErrorDetails();

            ErrorDetails &operator=(const ErrorDetails &other);

            bool isValid() const { return mPriv.constData() != 0; }

            bool hasDebugMessage() const
            {
                return allDetails().contains(QLatin1String("debug-message"));
            }

            QString debugMessage() const
            {
                return qdbus_cast<QString>(allDetails().value(QLatin1String("debug-message")));
            }

#if 0
            /*
             * TODO: these are actually specified in a draft interface only. Probably shouldn't
             * include them yet.
             */
            bool hasExpectedHostname() const
            {
                return allDetails().contains(QLatin1String("expected-hostname"));
            }

            QString expectedHostname() const
            {
                return qdbus_cast<QString>(allDetails().value(QLatin1String("expected-hostname")));
            }

            bool hasCertificateHostname() const
            {
                return allDetails().contains(QLatin1String("certificate-hostname"));
            }

            QString certificateHostname() const
            {
                return qdbus_cast<QString>(allDetails().value(QLatin1String("certificate-hostname")));
            }
#endif

            QVariantMap allDetails() const;

        private:
            friend class Connection;

            struct Private;
            friend struct Private;
            QSharedDataPointer<Private> mPriv;
    };

    const ErrorDetails &errorDetails() const;

    uint selfHandle() const;
    ContactPtr selfContact() const;

    CurrencyAmount accountBalance() const;

    ConnectionCapabilities capabilities() const;

    PendingReady *requestConnect(const Features &requestedFeatures = Features());
    PendingOperation *requestDisconnect();

    PendingHandles *requestHandles(HandleType handleType, const QStringList &names);
    PendingHandles *referenceHandles(HandleType handleType, const UIntList &handles);

    ContactManagerPtr contactManager() const;

#if defined(BUILDING_TELEPATHY_QT4) || defined(TP_QT4_ENABLE_LOWLEVEL_API)
    ConnectionLowlevelPtr lowlevel();
    ConnectionLowlevelConstPtr lowlevel() const;
#endif

Q_SIGNALS:
    void statusChanged(Tp::ConnectionStatus newStatus);

    void selfHandleChanged(uint newHandle);
    // FIXME: might not need this when Renaming is fixed and mapped to Contacts
    void selfContactChanged();

    void accountBalanceChanged(const Tp::CurrencyAmount &accountBalance);

protected:
    Connection(const QDBusConnection &bus, const QString &busName,
            const QString &objectPath,
            const ChannelFactoryConstPtr &channelFactory,
            const ContactFactoryConstPtr &contactFactory);

    Client::ConnectionInterface *baseInterface() const;

private Q_SLOTS:
    void onStatusReady(uint status);
    void onStatusChanged(uint status, uint reason);
    void onConnectionError(const QString &error, const QVariantMap &details);
    void gotMainProperties(QDBusPendingCallWatcher *watcher);
    void gotStatus(QDBusPendingCallWatcher *watcher);
    void gotInterfaces(QDBusPendingCallWatcher *watcher);
    void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    void gotCapabilities(QDBusPendingCallWatcher *watcher);
    void gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher);
    void gotSimpleStatuses(QDBusPendingCallWatcher *watcher);
    void gotSelfContact(Tp::PendingOperation *op);
    void gotContactListsHandles(Tp::PendingOperation *op);
    void gotContactListChannel(Tp::PendingOperation *op);
    void contactListChannelReady();
    void onNewChannels(const Tp::ChannelDetailsList &channelDetailsList);
    void onContactListGroupChannelReady(Tp::PendingOperation *op);
    void gotChannels(QDBusPendingCallWatcher *watcher);

    void doReleaseSweep(uint handleType);

    void onSelfHandleChanged(uint);

    void gotBalance(QDBusPendingCallWatcher *watcher);
    void onBalanceChanged(const Tp::CurrencyAmount &);

private:
    class PendingConnect;
    friend class ConnectionLowlevel;
    friend class PendingChannel;
    friend class PendingConnect;
    friend class PendingContactAttributes;
    friend class PendingContacts;
    friend class PendingHandles;
    friend class ReferencedHandles;

    void refHandle(HandleType handleType, uint handle);
    void unrefHandle(HandleType handleType, uint handle);
    void handleRequestLanded(HandleType handleType);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::Connection::ErrorDetails);

#endif
