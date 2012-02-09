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

#ifndef _TelepathyQt_abstract_interface_h_HEADER_GUARD_
#define _TelepathyQt_abstract_interface_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QDBusAbstractInterface>

namespace Tp
{

class DBusProxy;
class PendingVariant;
class PendingOperation;
class PendingVariantMap;

class TP_QT_EXPORT AbstractInterface : public QDBusAbstractInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractInterface)

public:
    virtual ~AbstractInterface();

    bool isValid() const;
    QString invalidationReason() const;
    QString invalidationMessage() const;

    void setMonitorProperties(bool monitorProperties);
    bool isMonitoringProperties() const;

Q_SIGNALS:
    void propertiesChanged(const QVariantMap &changedProperties,
            const QStringList &invalidatedProperties);

protected Q_SLOTS:
    virtual void invalidate(Tp::DBusProxy *proxy,
            const QString &error, const QString &message);

protected:
    AbstractInterface(DBusProxy *proxy, const QLatin1String &interface);
    AbstractInterface(const QString &busName, const QString &path,
            const QLatin1String &interface, const QDBusConnection &connection,
            QObject *parent);

    PendingVariant *internalRequestProperty(const QString &name) const;
    PendingOperation *internalSetProperty(const QString &name, const QVariant &newValue);
    PendingVariantMap *internalRequestAllProperties() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onPropertiesChanged(const QString &interface,
            const QVariantMap &changedProperties,
            const QStringList &invalidatedProperties);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
