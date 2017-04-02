/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2017 Alexandr Akulich <akulichalexander@gmail.com>
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

#ifndef _TelepathyQt_room_h_HEADER_GUARD_
#define _TelepathyQt_room_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Feature>
#include <TelepathyQt/Object>
#include <TelepathyQt/Types>

#include <QSet>
#include <QVariantMap>

namespace Tp
{

class TP_QT_EXPORT Room : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(Room)

public:
    static const Feature FeatureRoomConfig;

    ~Room();

    Features actualFeatures() const;

    bool moderated() const;
    QString title() const;

Q_SIGNALS:
    void titleChanged(const QString &title);

protected:
    explicit Room(const ConnectionPtr &connection, uint handle);

private:
    TP_QT_NO_EXPORT void receiveRoomConfig(const QVariantMap &props);
    TP_QT_NO_EXPORT void updateRoomConfigProperties(const QVariantMap &changedProperties);

    struct Private;
    friend class Connection;
    friend class Channel;
    friend class Private;
    Private *mPriv;
};

} // Tp

#endif
