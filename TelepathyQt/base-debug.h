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

#ifndef _TelepathyQt_base_debug_h_HEADER_GUARD_
#define _TelepathyQt_base_debug_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Callbacks>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>

namespace Tp
{

class TP_QT_EXPORT BaseDebug : public DBusService
{
    Q_OBJECT
public:
    explicit BaseDebug(const QDBusConnection &dbusConnection = QDBusConnection::sessionBus());

    bool isEnabled() const;
    int getMessagesLimit() const;

    typedef Callback1<DebugMessageList, DBusError*> GetMessagesCallback;
    void setGetMessagesCallback(const GetMessagesCallback &cb);

    DebugMessageList getMessages(DBusError *error) const;

public Q_SLOTS:
    void setEnabled(bool enabled);
    void setGetMessagesLimit(int limit);
    void clear();

    void newDebugMessage(const QString &domain, DebugLevel level, const QString &message);
    void newDebugMessage(double time, const QString &domain, DebugLevel level, const QString &message);

    QVariantMap immutableProperties() const;

    bool registerObject(DBusError *error = NULL);

protected:
    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // namespace Tp

#endif // _TelepathyQt_base_debug_h_HEADER_GUARD_
