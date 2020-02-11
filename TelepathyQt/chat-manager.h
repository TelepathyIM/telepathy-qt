/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2020 Alexandr Akulich <akulichalexander@gmail.com>
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

#ifndef _TelepathyQt_chat_manager_h_HEADER_GUARD_
#define _TelepathyQt_chat_manager_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Feature>
#include <TelepathyQt/Object>
#include <TelepathyQt/Types>

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Tp
{

class TP_QT_EXPORT ChatManager : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(ChatManager)

public:
    ~ChatManager() override;

    ConnectionPtr connection() const;

    Features supportedFeatures() const;

private:
    friend class Connection;

    TP_QT_NO_EXPORT ChatManager(Connection *parent);

    TP_QT_NO_EXPORT static QString featureToInterface(const Feature &feature);
    TP_QT_NO_EXPORT void ensureTracking(const Feature &feature);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
