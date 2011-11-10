/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt_ready_object_h_HEADER_GUARD_
#define _TelepathyQt_ready_object_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Feature>

#include <QObject>

namespace Tp
{

class DBusProxy;
class PendingReady;
class ReadinessHelper;
class RefCounted;

class TP_QT_EXPORT ReadyObject
{
    Q_DISABLE_COPY(ReadyObject)

public:
    ReadyObject(RefCounted *object, const Feature &featureCore);
    ReadyObject(DBusProxy *proxy, const Feature &featureCore);
    virtual ~ReadyObject();

    virtual bool isReady(const Features &features = Features()) const;
    virtual PendingReady *becomeReady(const Features &requestedFeatures = Features());

    virtual Features requestedFeatures() const;
    virtual Features actualFeatures() const;
    virtual Features missingFeatures() const;

protected:
    ReadinessHelper *readinessHelper() const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
