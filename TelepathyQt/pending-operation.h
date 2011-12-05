/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt_pending_operation_h_HEADER_GUARD_
#define _TelepathyQt_pending_operation_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>
#include <TelepathyQt/RefCounted>
#include <TelepathyQt/SharedPtr>

#include <QObject>

class QDBusError;
class QDBusPendingCall;
class QDBusPendingCallWatcher;

namespace Tp
{

class ReadinessHelper;

class TP_QT_EXPORT PendingOperation : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingOperation)

public:
    virtual ~PendingOperation();

    bool isFinished() const;

    bool isValid() const;

    bool isError() const;
    QString errorName() const;
    QString errorMessage() const;

Q_SIGNALS:
    void finished(Tp::PendingOperation *operation);

protected:
    PendingOperation(const SharedPtr<RefCounted> &object);
    SharedPtr<RefCounted> object() const;

protected Q_SLOTS:
    void setFinished();
    void setFinishedWithError(const QString &name, const QString &message);
    void setFinishedWithError(const QDBusError &error);

private Q_SLOTS:
    TP_QT_NO_EXPORT void emitFinished();

private:
    friend class ContactManager;
    friend class ReadinessHelper;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
