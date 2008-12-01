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

#include <QObject>

class QDBusError;
class QDBusPendingCall;
class QDBusPendingCallWatcher;

namespace Telepathy
{
namespace Client
{

// TODO: remove this when we have the DBusProxy class
typedef QObject DBusProxy;

/**
 * Abstract base class for pending asynchronous operations.
 *
 * This class represents an incomplete asynchronous operation, such as a
 * D-Bus method call. When the operation has finished, it emits
 * #finished. The slot or slots connected to the #finished() signal may obtain
 * additional information from the %PendingOperation.
 *
 * In simple cases, like a D-Bus method with no 'out' arguments or for which
 * all 'out' arguments are to be ignored (so the possible results are
 * success with no extra information, or failure with an error code), the
 * trivial subclass %PendingVoidMethodCall can be used.
 *
 * For pending operations that produce a result, another subclass of
 * %PendingOperation can be used, with additional methods that provide that
 * result to the library user.
 *
 * After #finished() is emitted, the %PendingOperation is automatically
 * deleted using #deleteLater(), so library users must not explicitly
 * delete this object.
 *
 * The design is loosely based on KDE's KJob.
 */
class PendingOperation : public QObject
{
    Q_OBJECT

protected:
    PendingOperation(DBusProxy* proxy);
    void setFinished();
    void setError(const QString& name, const QString& message);
    void setError(const QDBusError& error);

public:
    virtual ~PendingOperation();

    DBusProxy* proxy() const;

    /**
     * Returns true if the pending operation has finished successfully.
     *
     * This should only be called from a slot connected to #finished(),
     * where it is equivalent to to (!isError()).
     *
     * \return true if the pending operation has finished and was successful
     */
    // FIXME: do we want this or isError() or both? KJob only has error()
    bool isSuccessful() const;

    /**
     * Returns true if the pending operation has finished unsuccessfully.
     *
     * This should only be called from a slot connected to #finished(),
     * where it is equivalent to to (!isSuccessful()).
     *
     * \return true if the pending operation has finished but was unsuccessful
     */
    bool isError() const;

    /**
     * If #isError() would return true, returns the D-Bus error with which
     * the operation failed. If the operation succeeded, returns an empty
     * string.
     *
     * This should only be called from a slot connected to #finished().
     *
     * \return a D-Bus error name or an empty string
     */
    // FIXME: in KJob it is an error to call this if !isError()
    QString errorName() const;

    /**
     * If isError() would return true, returns a debugging message associated
     * with the error, which may be an empty string. Otherwise, return an
     * empty string.
     *
     * This should only be called from a slot connected to #finished().
     *
     * \return a debugging message or an empty string
     */
    // FIXME: in KJob it is an error to call this if !isError()
    QString errorMessage() const;

Q_SIGNALS:
    /**
     * Emitted when the pending operation finishes, i.e. when #isFinished()
     * changes from false to true.
     *
     * \param operation This operation object, from which further information
     *    may be obtained
     * \param successful true if the operation was successful (the same as
     *    operation->isSuccessful())
     */
    // FIXME: would bool error be better? KJob only has error()
    void finished(PendingOperation* operation, bool successful);

private slots:
    void selfDestroyed(QObject* self);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};


class PendingVoidMethodCall : public PendingOperation
{
    Q_OBJECT

public:
    PendingVoidMethodCall(DBusProxy* proxy, QDBusPendingCall call);

private Q_SLOTS:
    void watcherFinished(QDBusPendingCallWatcher*);

private:
    // just ABI padding at the moment
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
