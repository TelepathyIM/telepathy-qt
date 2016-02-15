/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#ifndef _TelepathyQt_connection_h_HEADER_GUARD_
#define _TelepathyQt_connection_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/_gen/cli-connection.h>

#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/Contact>
#include <TelepathyQt/DBus>
#include <TelepathyQt/DBusProxy>
#include <TelepathyQt/OptionalInterfaceFactory>
#include <TelepathyQt/ReadinessHelper>
#include <TelepathyQt/Types>
#include <TelepathyQt/SharedPtr>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#include <QSet>
#include <QString>
#include <QStringList>

namespace Tp
{

class Channel;
class ConnectionCapabilities;
class ConnectionLowlevel;
class ContactManager;
class PendingChannel;
class PendingContactAttributes;
class PendingHandles;
class PendingOperation;
class PendingReady;

class TP_QT_EXPORT Connection : public StatefulDBusProxy,
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
    static const Feature FeatureConnected;

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

            bool hasServerMessage() const
            {
                return allDetails().contains(QLatin1String("server-message"));
            }

            QString serverMessage() const
            {
                return qdbus_cast<QString>(allDetails().value(QLatin1String("server-message")));
            }

            bool hasUserRequested() const
            {
                return allDetails().contains(QLatin1String("user-requested"));
            }

            bool userRequested() const
            {
                return qdbus_cast<bool>(allDetails().value(QLatin1String("user-requested")));
            }

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

    ContactManagerPtr contactManager() const;

#if defined(BUILDING_TP_QT) || defined(TP_QT_ENABLE_LOWLEVEL_API)
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
            const ContactFactoryConstPtr &contactFactory,
            const Feature &coreFeature);

    Client::ConnectionInterface *baseInterface() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onStatusReady(uint status);
    TP_QT_NO_EXPORT void onStatusChanged(uint status, uint reason);
    TP_QT_NO_EXPORT void onConnectionError(const QString &error, const QVariantMap &details);
    TP_QT_NO_EXPORT void gotMainProperties(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotStatus(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotInterfaces(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotSelfHandle(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotCapabilities(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotContactAttributeInterfaces(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotSimpleStatuses(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void gotSelfContact(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onIntrospectRosterFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onIntrospectRosterGroupsFinished(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void doReleaseSweep(uint handleType);

    TP_QT_NO_EXPORT void onSelfHandleChanged(uint);

    TP_QT_NO_EXPORT void gotBalance(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onBalanceChanged(const Tp::CurrencyAmount &);

private:
    class PendingConnect;
    friend class ConnectionLowlevel;
    friend class PendingChannel;
    friend class PendingConnect;
    friend class PendingContactAttributes;
    friend class PendingContacts;
    friend class PendingHandles;
    friend class ReferencedHandles;

    TP_QT_NO_EXPORT void refHandle(HandleType handleType, uint handle);
    TP_QT_NO_EXPORT void unrefHandle(HandleType handleType, uint handle);
    TP_QT_NO_EXPORT void handleRequestLanded(HandleType handleType);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::Connection::ErrorDetails);

#endif
