/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2020 Alexander Akulich <akulichalexander@gmail.com>
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

#ifndef _TelepathyQt_pending_chat_read_operation_h_HEADER_GUARD_
#define _TelepathyQt_pending_chat_read_operation_h_HEADER_GUARD_

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

QT_FORWARD_DECLARE_CLASS(QDBusPendingCallWatcher)

namespace Tp
{

class TP_QT_EXPORT PendingChatReadOperation : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingChatReadOperation)

public:
    ~PendingChatReadOperation() override;

    ConnectionPtr connection() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onMarkReadFinished(QDBusPendingCallWatcher *watcher);

private:
    friend class ChatManager;
    friend class ConnectionLowlevel;

    TP_QT_NO_EXPORT PendingChatReadOperation(const ConnectionPtr &connection,
            const QVariantMap &request,
            const QString &messageToken);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif // _TelepathyQt_pending_chat_read_operation_h_HEADER_GUARD_
