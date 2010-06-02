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

#ifndef _TelepathyQt4_tube_channel_internal_h_HEADER_GUARD_
#define _TelepathyQt4_tube_channel_internal_h_HEADER_GUARD_

#include <TelepathyQt4/TubeChannel>

namespace Tp {

class TELEPATHY_QT4_NO_EXPORT TubeChannelPrivate
{
    Q_DECLARE_PUBLIC(TubeChannel)

protected:
    TubeChannel * const q_ptr;

public:
    TubeChannelPrivate(TubeChannel *parent);
    virtual ~TubeChannelPrivate();

    virtual void init();

    void extractTubeProperties(const QVariantMap &props);

    static void introspectTube(TubeChannelPrivate *self);

    void reintrospectParameters();

    ReadinessHelper *readinessHelper;

    // Properties
    TubeChannelState state;
    TubeType type;
    QVariantMap parameters;

    // Private slots
    void __k__onTubeChannelStateChanged(uint newstate);
    void __k__gotTubeProperties(QDBusPendingCallWatcher *watcher);
    void __k__gotTubeParameters(QDBusPendingCallWatcher *watcher);

};

}

#endif
