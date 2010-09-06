/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_connection_factory_h_HEADER_GUARD_
#define _TelepathyQt4_connection_factory_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>
#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

#include <TelepathyQt4/Feature>
#include <TelepathyQt4/FixedFeatureFactory>

// For Q_DISABLE_COPY
#include <QtGlobal>

#include <QString>

class QDBusConnection;

namespace Tp
{

class PendingReady;

class TELEPATHY_QT4_EXPORT ConnectionFactory : public FixedFeatureFactory
{
public:
    static ConnectionFactoryPtr create(const QDBusConnection &bus,
            const Features &features = Features());

    virtual ~ConnectionFactory();

    PendingReady *proxy(const QString &busName, const QString &objectPath,
            const ChannelFactoryConstPtr &chanFactory) const;

protected:
    ConnectionFactory(const QDBusConnection &bus, const Features &features);

    virtual ConnectionPtr construct(const QString &busName, const QString &objectPath,
            const ChannelFactoryConstPtr &chanFactory) const;
    virtual QString finalBusNameFrom(const QString &uniqueOrWellKnown) const;
    // Nothing we'd like to prepare()
    // Fixed features

private:
    struct Private;
    Private *mPriv; // Currently unused, just for future-proofing
};

} // Tp

#endif
