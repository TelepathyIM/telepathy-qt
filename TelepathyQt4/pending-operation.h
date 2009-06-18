/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_pending_operation_h_HEADER_GUARD_
#define _TelepathyQt4_cli_pending_operation_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>

class QDBusError;
class QDBusPendingCall;
class QDBusPendingCallWatcher;

namespace Tp
{

class PendingOperation : public QObject
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
    PendingOperation(QObject *parent);

protected Q_SLOTS:
    void setFinished();
    void setFinishedWithError(const QString &name, const QString &message);
    void setFinishedWithError(const QDBusError &error);

private Q_SLOTS:
    void emitFinished();

private:
    struct Private;
    Private *mPriv;
};

} // Tp

#endif
