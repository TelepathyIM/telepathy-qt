/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt_tube_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>

namespace Tp
{

class TP_QT_EXPORT TubeChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(TubeChannel)

public:
    static const Feature FeatureCore;

    static TubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~TubeChannel();

    TubeChannelState state() const;

    QVariantMap parameters() const;

Q_SIGNALS:
    void stateChanged(Tp::TubeChannelState state);

protected:
    TubeChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = TubeChannel::FeatureCore);

    void setParameters(const QVariantMap &parameters);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onTubeChannelStateChanged(uint newstate);
    TP_QT_NO_EXPORT void gotTubeProperties(Tp::PendingOperation *op);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

#endif
