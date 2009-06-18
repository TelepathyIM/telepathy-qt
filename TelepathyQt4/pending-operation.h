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
 * deleted using deleteLater(), so library users must not explicitly
 * delete this object.
 *
 * The design is loosely based on KDE's KJob.
 */
class PendingOperation : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingOperation)

public:
    virtual ~PendingOperation();

    /**
     * Returns whether or not the request has finished processing. #finished()
     * is emitted when this changes from <code>false</code> to
     * <code>true</code>.
     *
     * Equivalent to <code>(isValid() || isError())</code>.
     *
     * \sa finished()
     *
     * \return <code>true</code> if the request has finished
     */
    bool isFinished() const;

    /**
     * Returns whether or not the request completed successfully. If the
     * request has not yet finished processing (isFinished() returns
     * <code>false</code>), this cannot yet be known, and <code>false</code>
     * will be returned.
     *
     * Equivalent to <code>(isFinished() && !isError())</code>.
     *
     * \return <code>true</code> iff the request has finished processing AND
     *         has completed successfully.
     */
    bool isValid() const;

    /**
     * Returns whether or not the request resulted in an error. If the
     * request has not yet finished processing (isFinished() returns
     * <code>false</code>), this cannot yet be known, and <code>false</code>
     * will be returned.
     *
     * Equivalent to <code>(isFinished() && !isValid())</code>.
     *
     * \return <code>true</code> iff the request has finished processing AND
     *         has resulted in an error.
     */
    bool isError() const;

    /**
     * If #isError() would return true, returns the D-Bus error with which
     * the operation failed. If the operation succeeded or has not yet
     * finished, returns an empty string.
     *
     * \return a D-Bus error name or an empty string
     */
    QString errorName() const;

    /**
     * If isError() would return true, returns a debugging message associated
     * with the error, which may be an empty string. Otherwise, return an
     * empty string.
     *
     * \return a debugging message or an empty string
     */
    QString errorMessage() const;

Q_SIGNALS:
    /**
     * Emitted when the pending operation finishes, i.e. when #isFinished()
     * changes from <code>false</code> to <code>true</code>.
     *
     * \param operation This operation object, from which further information
     *    may be obtained
     */
    void finished(Tp::PendingOperation *operation);

protected:
    /**
     * Protected constructor. Only subclasses of this class may be constructed
     *
     * \param parent The object on which this pending operation takes place
     */
    PendingOperation(QObject *parent);

protected Q_SLOTS:
    /**
     * Record that this pending operation has finished successfully, and
     * emit the #finished() signal next time the event loop runs.
     */
    void setFinished();

    /**
     * Record that this pending operation has finished with an error, and
     * emit the #finished() signal next time the event loop runs.
     *
     * \param name A D-Bus error name, which must be non-empty
     * \param message A debugging message
     */
    void setFinishedWithError(const QString &name, const QString &message);

    /**
     * Record that this pending operation has finished with an error, and
     * emit the #finished() signal next time the event loop runs.
     *
     * \param error A QtDBus error
     */
    void setFinishedWithError(const QDBusError &error);

private Q_SLOTS:
    void emitFinished();

private:
    struct Private;
    Private *mPriv;
};

} // Tp

#endif
