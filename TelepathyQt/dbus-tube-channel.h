/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_dbus_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt_dbus_tube_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/TubeChannel>

namespace Tp
{

class TP_QT_EXPORT DBusTubeChannel : public TubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(DBusTubeChannel)

public:
    static const Feature FeatureCore;
    static const Feature FeatureBusNameMonitoring;

    static DBusTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~DBusTubeChannel();

    QString serviceName() const;

    bool supportsRestrictingToCurrentUser() const;

    QHash<QString, Tp::ContactPtr> contactsForBusNames() const;

    QString address() const;

protected:
    DBusTubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

Q_SIGNALS:
    void busNameAdded(const QString &busName, const Tp::ContactPtr &contact);
    void busNameRemoved(const QString &busName, const Tp::ContactPtr &contact);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onRequestAllPropertiesFinished(Tp::PendingOperation*);
    TP_QT_NO_EXPORT void onRequestPropertyDBusNamesFinished(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void onDBusNamesChanged(const Tp::DBusTubeParticipants &added, const Tp::UIntList &removed);
    TP_QT_NO_EXPORT void onContactsRetrieved(const QUuid &uuid, const QList<Tp::ContactPtr> &contacts);
    TP_QT_NO_EXPORT void onQueueCompleted();

private:
    TP_QT_NO_EXPORT void setAddress(const QString &address);

    struct Private;
    friend struct PendingDBusTubeConnection;
    friend struct Private;
    Private *mPriv;

};

} // Tp

#endif
