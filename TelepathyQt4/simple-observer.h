/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt4_simple_observer_h_HEADER_GUARD_
#define _TelepathyQt4_simple_observer_h_HEADER_GUARD_

#include <TelepathyQt4/AbstractClientObserver>
#include <TelepathyQt4/ChannelClassFeatures>
#include <TelepathyQt4/Types>

#include <QObject>

namespace Tp
{

class PendingOperation;

class TELEPATHY_QT4_NO_EXPORT SimpleObserver : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(SimpleObserver)

public:
    static SimpleObserverPtr create(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QList<ChannelClassFeatures> &extraChannelFeatures =
                QList<ChannelClassFeatures>());
    static SimpleObserverPtr create(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const ContactPtr &contact,
            const QList<ChannelClassFeatures> &extraChannelFeatures =
                QList<ChannelClassFeatures>());
    static SimpleObserverPtr create(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            const QList<ChannelClassFeatures> &extraChannelFeatures =
                QList<ChannelClassFeatures>());

    virtual ~SimpleObserver();

    AccountPtr account() const;
    ChannelClassSpecList channelFilter() const;
    QString contactIdentifier() const;
    QList<ChannelClassFeatures> extraChannelFeatures() const;

    QList<ChannelPtr> channels() const;

Q_SIGNALS:
    void newChannels(const QList<Tp::ChannelPtr> &channels);
    void channelInvalidated(const Tp::ChannelPtr &channel, const QString &errorName,
            const QString &errorMessage);

private Q_SLOTS:
    TELEPATHY_QT4_NO_EXPORT void onAccountConnectionChanged(const Tp::ConnectionPtr &connection);
    TELEPATHY_QT4_NO_EXPORT void onAccountConnectionConnected();
    TELEPATHY_QT4_NO_EXPORT void onContactConstructed(Tp::PendingOperation *op);

    TELEPATHY_QT4_NO_EXPORT void onNewChannels(const Tp::AccountPtr &channelsAccount,
            const QList<Tp::ChannelPtr> &channels);
    TELEPATHY_QT4_NO_EXPORT void onChannelInvalidated(const Tp::AccountPtr &channelAccount,
            const Tp::ChannelPtr &channel, const QString &errorName, const QString &errorMessage);

private:
    friend class SimpleCallObserver;
    friend class SimpleTextObserver;

    TELEPATHY_QT4_NO_EXPORT static SimpleObserverPtr create(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            bool requiresNormalization,
            const QList<ChannelClassFeatures> &extraChannelFeatures);

    TELEPATHY_QT4_NO_EXPORT SimpleObserver(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            bool requiresNormalization,
            const QList<ChannelClassFeatures> &extraChannelFeatures);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
