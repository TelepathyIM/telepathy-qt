/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt4_tube_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>

namespace Tp {

class TubeChannelPrivate;
class TELEPATHY_QT4_EXPORT TubeChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(TubeChannel)
    Q_DECLARE_PRIVATE(TubeChannel)

// private Q_SLOTS:
    Q_PRIVATE_SLOT(d_func(), void onTubeChannelStateChanged(uint))
    Q_PRIVATE_SLOT(d_func(), void gotTubeProperties(QDBusPendingCallWatcher *))

public:
    static const Feature FeatureTube;

    static TubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~TubeChannel();

    TubeChannelState tubeState() const;

    QVariantMap parameters() const;

protected:
    TubeChannel(const ConnectionPtr &connection, const QString &objectPath,
                const QVariantMap &immutableProperties,
                const Feature &coreFeature = TubeChannel::FeatureCore);
    // For private class inheriters
    TubeChannel(const ConnectionPtr &connection, const QString &objectPath,
                const QVariantMap &immutableProperties,
                const Feature &coreFeature,
                TubeChannelPrivate &dd);

    virtual void connectNotify(const char* signal);

Q_SIGNALS:
    void tubeStateChanged(Tp::TubeChannelState newstate);

protected:
    TubeChannelPrivate * const d_ptr;
};

}

#endif
