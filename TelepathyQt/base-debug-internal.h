/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
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

#include "TelepathyQt/_gen/svc-debug.h"

#include <TelepathyQt/MethodInvocationContext>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class TP_QT_NO_EXPORT BaseDebug::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    Adaptee(const QDBusConnection &dbusConnection, BaseDebug *interface);

    bool isEnabled();

Q_SIGNALS:
    void newDebugMessage(double time, const QString& domain, uint level, const QString& message);

public Q_SLOTS:
    void setEnabled(bool enabled);

private Q_SLOTS:
    void getMessages(
            const Tp::Service::DebugAdaptor::GetMessagesContextPtr &context);

public:
    BaseDebug *mInterface;
};

}
