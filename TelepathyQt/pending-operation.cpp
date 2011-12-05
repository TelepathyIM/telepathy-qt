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

#include <TelepathyQt/PendingOperation>

#define IN_TP_QT_HEADER
#include "simple-pending-operations.h"
#undef IN_TP_QT_HEADER

#include "TelepathyQt/_gen/pending-operation.moc.hpp"
#include "TelepathyQt/_gen/simple-pending-operations.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QTimer>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingOperation::Private
{
    Private(const SharedPtr<RefCounted> &object)
        : object(object),
          finished(false)
    {
    }

    SharedPtr<RefCounted> object;
    QString errorName;
    QString errorMessage;
    bool finished;
};

/**
 * \class PendingOperation
 * \headerfile TelepathyQt/pending-operation.h <TelepathyQt/PendingOperation>
 *
 * \brief The PendingOperation class is a base class for pending asynchronous
 * operations.
 *
 * This class represents an incomplete asynchronous operation, such as a
 * D-Bus method call. When the operation has finished, it emits
 * finished(). The slot or slots connected to the finished() signal may obtain
 * additional information from the pending operation.
 *
 * In simple cases, like a D-Bus method with no 'out' arguments or for which
 * all 'out' arguments are to be ignored (so the possible results are
 * success with no extra information, or failure with an error code), the
 * trivial subclass PendingVoid can be used.
 *
 * For pending operations that produce a result, another subclass of
 * PendingOperation can be used, with additional methods that provide that
 * result to the library user.
 *
 * After finished() is emitted, the PendingOperation is automatically
 * deleted using deleteLater(), so library users must not explicitly
 * delete this object.
 *
 * The design is loosely based on KDE's KJob.
 *
 * See \ref async_model
 */

/**
 * Construct a new PendingOperation object.
 *
 * \param object The object on which this pending operation takes place.
 */
PendingOperation::PendingOperation(const SharedPtr<RefCounted> &object)
    : QObject(),
      mPriv(new Private(object))
{
}

/**
 * Class destructor.
 */
PendingOperation::~PendingOperation()
{
    if (!mPriv->finished) {
        warning() << this <<
            "still pending when it was deleted - finished will "
            "never be emitted";
    }

    delete mPriv;
}

/**
 * Return the object on which this pending operation takes place.
 *
 * \return A pointer to a RefCounted object.
 */
SharedPtr<RefCounted> PendingOperation::object() const
{
    return mPriv->object;
}

void PendingOperation::emitFinished()
{
    Q_ASSERT(mPriv->finished);
    emit finished(this);
    deleteLater();
}

/**
 * Record that this pending operation has finished successfully, and
 * emit the finished() signal next time the event loop runs.
 */
void PendingOperation::setFinished()
{
    if (mPriv->finished) {
        if (!mPriv->errorName.isEmpty()) {
            warning() << this << "trying to finish with success, but already"
                " failed with" << mPriv->errorName << ":" << mPriv->errorMessage;
        } else {
            warning() << this << "trying to finish with success, but already"
                " succeeded";
        }
        return;
    }

    mPriv->finished = true;
    Q_ASSERT(isValid());
    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

/**
 * Record that this pending operation has finished with an error, and
 * emit the finished() signal next time the event loop runs.
 *
 * \param name The D-Bus error name, which must be non-empty.
 * \param message The debugging message.
 */
void PendingOperation::setFinishedWithError(const QString &name,
        const QString &message)
{
    if (mPriv->finished) {
        if (mPriv->errorName.isEmpty()) {
            warning() << this << "trying to fail with" << name <<
                "but already failed with" << errorName() << ":" <<
                errorMessage();
        } else {
            warning() << this << "trying to fail with" << name <<
                "but already succeeded";
        }
        return;
    }

    if (name.isEmpty()) {
        warning() << this << "should be given a non-empty error name";
        mPriv->errorName = QLatin1String("org.freedesktop.Telepathy.Qt.ErrorHandlingError");
    } else {
        mPriv->errorName = name;
    }

    mPriv->errorMessage = message;
    mPriv->finished = true;
    Q_ASSERT(isError());
    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

/**
 * Record that this pending operation has finished with an error, and
 * emit the finished() signal next time the event loop runs.
 *
 * \param error The error.
 * \sa finished()
 */
void PendingOperation::setFinishedWithError(const QDBusError &error)
{
    setFinishedWithError(error.name(), error.message());
}

/**
 * Return whether or not the request completed successfully. If the
 * request has not yet finished processing (isFinished() returns
 * \c false), this cannot yet be known, and \c false
 * will be returned.
 *
 * Equivalent to <code>(isFinished() && !isError())</code>.
 *
 * \return \c true if the request has finished processing and
 *         has completed successfully, \c false otherwise.
 */
bool PendingOperation::isValid() const
{
    return (mPriv->finished && mPriv->errorName.isEmpty());
}

/**
 * Return whether or not the request has finished processing.
 *
 * The signal finished() is emitted when this changes from \c false
 * to true.
 *
 * Equivalent to <code>(isValid() || isError())</code>.
 *
 * \return \c true if the request has finished, \c false otherwise.
 * \sa finished()
 */
bool PendingOperation::isFinished() const
{
    return mPriv->finished;
}

/**
 * Return whether or not the request resulted in an error.
 *
 * If the request has not yet finished processing (isFinished() returns
 * \c false), this cannot yet be known, and \c false
 * will be returned.
 *
 * Equivalent to <code>(isFinished() && !isValid())</code>.
 *
 * \return \c true if the request has finished processing and
 *         has resulted in an error, \c false otherwise.
 */
bool PendingOperation::isError() const
{
    return (mPriv->finished && !mPriv->errorName.isEmpty());
}

/**
 * If isError() returns \c true, returns the D-Bus error with which
 * the operation failed. If the operation succeeded or has not yet
 * finished, returns an empty string.
 *
 * \return A D-Bus error name, or an empty string.
 */
QString PendingOperation::errorName() const
{
    return mPriv->errorName;
}

/**
 * If isError() would return \c true, returns a debugging message associated
 * with the error, which may be an empty string. Otherwise, return an
 * empty string.
 *
 * \return A debugging message, or an empty string.
 */
QString PendingOperation::errorMessage() const
{
    return mPriv->errorMessage;
}

/**
 * \fn void PendingOperation::finished(Tp::PendingOperation* operation)
 *
 * Emitted when the pending operation finishes, i.e. when isFinished()
 * changes from \c false to \c true.
 *
 * \param operation This operation object, from which further information
 *                  may be obtained.
 */

/**
 * \class PendingSuccess
 * \ingroup utils
 * \headerfile TelepathyQt/simple-pending-operations.h <TelepathyQt/PendingSuccess>
 *
 * \brief The PendingSuccess class represents PendingOperation that is always
 * successful.
 */

/**
 * \class PendingFailure
 * \ingroup utils
 * \headerfile TelepathyQt/simple-pending-operations.h <TelepathyQt/PendingFailure>
 *
 * \brief The PendingFailure class represents a PendingOperation that always
 * fails with the error passed to the constructor.
 */

/**
 * \class PendingVoid
 * \ingroup utils
 * \headerfile TelepathyQt/simple-pending-operations.h <TelepathyQt/PendingVoid>
 *
 * \brief The PendingVoid class is a generic subclass of PendingOperation
 * representing a pending D-Bus method call that does not return anything
 * (or returns a result that is not interesting).
 */

/**
 * Construct a new PendingVoid object.
 *
 * \param object The object on which this pending operation takes place.
 * \param call A pending call as returned by the auto-generated low level
 *             Telepathy API; if the method returns anything, the return
 *             value(s) will be ignored.
 */
PendingVoid::PendingVoid(QDBusPendingCall call, const SharedPtr<RefCounted> &object)
    : PendingOperation(object)
{
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

void PendingVoid::watcherFinished(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        setFinishedWithError(watcher->error());
    } else {
        setFinished();
    }

    watcher->deleteLater();
}

struct TP_QT_NO_EXPORT PendingComposite::Private
{
    Private(bool failOnFirstError, uint nOperations)
        : failOnFirstError(failOnFirstError),
          error(false),
          nOperations(nOperations),
          nOperationsFinished(0)
    {
    }

    bool failOnFirstError;
    bool error;
    QString errorName;
    QString errorMessage;
    uint nOperations;
    uint nOperationsFinished;
};

/**
 * \class PendingComposite
 * \ingroup utils
 * \headerfile TelepathyQt/simple-pending-operations.h <TelepathyQt/PendingComposite>
 *
 * \brief The PendingComposite class is a PendingOperation that can be used
 * to track multiple pending operations at once.
 */

PendingComposite::PendingComposite(const QList<PendingOperation*> &operations,
         const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private(true, operations.size()))
{
    foreach (PendingOperation *operation, operations) {
        connect(operation,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onOperationFinished(Tp::PendingOperation*)));
    }
}

PendingComposite::PendingComposite(const QList<PendingOperation*> &operations,
         bool failOnFirstError, const SharedPtr<RefCounted> &object)
    : PendingOperation(object),
      mPriv(new Private(failOnFirstError, operations.size()))
{
    foreach (PendingOperation *operation, operations) {
        connect(operation,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onOperationFinished(Tp::PendingOperation*)));
    }
}

PendingComposite::~PendingComposite()
{
    delete mPriv;
}

void PendingComposite::onOperationFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        if (mPriv->failOnFirstError) {
            setFinishedWithError(op->errorName(), op->errorMessage());
            return;
        } else if (!mPriv->error) {
            /* only save the first error that will be used on setFinishedWithError when all
             * pending operations finish */
            mPriv->error = true;
            mPriv->errorName = op->errorName();
            mPriv->errorMessage = op->errorMessage();
        }
    }

    if (++mPriv->nOperationsFinished == mPriv->nOperations) {
        if (!mPriv->error) {
            setFinished();
        } else {
            setFinishedWithError(mPriv->errorName, mPriv->errorMessage);
        }
    }
}

} // Tp
