/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
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
#include <TelepathyQt/DebugReceiver>

#include "TelepathyQt/_gen/debug-receiver.moc.hpp"
#include "TelepathyQt/_gen/cli-debug-receiver-body.hpp"
#include "TelepathyQt/_gen/cli-debug-receiver.moc.hpp"

namespace Tp
{

struct TP_QT_NO_EXPORT DebugReceiver::Private
{
};

/**
 * Feature representing the core that needs to become ready to make the DebugReceiver
 * object usable.
 *
 * Note that this feature must be enabled in order to use most DebugReceiver methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature DebugReceiver::FeatureCore = Feature(QLatin1String(DebugReceiver::staticMetaObject.className()), 0, true);

DebugReceiverPtr DebugReceiver::create(const QString &busName, const QDBusConnection &bus)
{
    return DebugReceiverPtr(new DebugReceiver(bus, busName,
                QLatin1String("/org/freedesktop/Telepathy/debug"),
                DebugReceiver::FeatureCore));
}

DebugReceiver::DebugReceiver(const QDBusConnection &bus,
        const QString &busName,
        const QString &objectPath,
        const Feature &featureCore)
    : StatefulDBusProxy(bus, busName, objectPath, featureCore),
      mPriv(new Private)
{
}

DebugReceiver::~DebugReceiver()
{
    delete mPriv;
}


}
