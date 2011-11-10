/**
 * This file is part of TelepathyQt
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

#ifndef _TelepathyQt_simple_observer_h_HEADER_GUARD_
#define _TelepathyQt_simple_observer_h_HEADER_GUARD_

#include <TelepathyQt/AbstractClientObserver>
#include <TelepathyQt/ChannelClassFeatures>
#include <TelepathyQt/Types>

#include <QObject>

namespace Tp
{

class PendingOperation;

class TP_QT_EXPORT SimpleObserver : public QObject, public RefCounted
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
    TP_QT_NO_EXPORT void onAccountConnectionChanged(const Tp::ConnectionPtr &connection);
    TP_QT_NO_EXPORT void onAccountConnectionConnected();
    TP_QT_NO_EXPORT void onContactConstructed(Tp::PendingOperation *op);

    TP_QT_NO_EXPORT void onNewChannels(const Tp::AccountPtr &channelsAccount,
            const QList<Tp::ChannelPtr> &channels);
    TP_QT_NO_EXPORT void onChannelInvalidated(const Tp::AccountPtr &channelAccount,
            const Tp::ChannelPtr &channel, const QString &errorName, const QString &errorMessage);

private:
    friend class SimpleCallObserver;
    friend class SimpleTextObserver;

    TP_QT_NO_EXPORT static SimpleObserverPtr create(const AccountPtr &account,
            const ChannelClassSpecList &channelFilter,
            const QString &contactIdentifier,
            bool requiresNormalization,
            const QList<ChannelClassFeatures> &extraChannelFeatures);

    TP_QT_NO_EXPORT SimpleObserver(const AccountPtr &account,
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
