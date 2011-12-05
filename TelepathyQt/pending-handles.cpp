/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt/PendingHandles>

#include "TelepathyQt/_gen/pending-handles.moc.hpp"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ReferencedHandles>

#include "TelepathyQt/debug-internal.h"

namespace Tp
{

struct TP_QT_NO_EXPORT PendingHandles::Private
{
    HandleType handleType;
    bool isRequest;
    QStringList namesRequested;
    UIntList handlesToReference;
    ReferencedHandles handles;
    ReferencedHandles alreadyHeld;
    UIntList invalidHandles;
    QStringList validNames;
    QHash<QString, QPair<QString, QString> > invalidNames;

    // one to one requests (ids)
    QHash<QDBusPendingCallWatcher *, uint> handlesForWatchers;
    QHash<QDBusPendingCallWatcher *, QString> idsForWatchers;
    QHash<QString, uint> handlesForIds;
    int requestsFinished;
};

/**
 * \class PendingHandles
 * \ingroup clientconn
 * \headerfile TelepathyQt/pending-handles.h <TelepathyQt/PendingHandles>
 *
 * \brief The PendingHandles class represents the parameters of and the reply to
 * an asynchronous handle request/hold.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is to use Connection::requestHandles() or Connection::referenceHandles().
 *
 * See \ref async_model
 */

PendingHandles::PendingHandles(const ConnectionPtr &connection, HandleType handleType,
        const QStringList &names)
    : PendingOperation(connection),
      mPriv(new Private)
{
    debug() << "PendingHandles(request)";

    mPriv->handleType = handleType;
    mPriv->isRequest = true;
    mPriv->namesRequested = names;
    mPriv->requestsFinished = 0;

    // try to request all handles at once
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                connection->baseInterface()->RequestHandles(mPriv->handleType, names),
                this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onRequestHandlesFinished(QDBusPendingCallWatcher*)));
}

PendingHandles::PendingHandles(const ConnectionPtr &connection, HandleType handleType,
        const UIntList &handles, const UIntList &alreadyHeld,
        const UIntList &notYetHeld)
    : PendingOperation(connection),
      mPriv(new Private)
{
    debug() << "PendingHandles(reference)";

    mPriv->handleType = handleType;
    mPriv->isRequest = false;
    mPriv->handlesToReference = handles;
    mPriv->alreadyHeld = ReferencedHandles(connection, mPriv->handleType, alreadyHeld);
    mPriv->requestsFinished = 0;

    if (notYetHeld.isEmpty()) {
        debug() << " All handles already held, finishing up instantly";
        mPriv->handles = mPriv->alreadyHeld;
        setFinished();
    } else {
        debug() << " Calling HoldHandles";

        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    connection->baseInterface()->HoldHandles(mPriv->handleType, notYetHeld),
                    this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onHoldHandlesFinished(QDBusPendingCallWatcher*)));
    }
}

PendingHandles::PendingHandles(const QString &errorName, const QString &errorMessage)
    : PendingOperation(ConnectionPtr()), mPriv(new Private)
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingHandles::~PendingHandles()
{
    delete mPriv;
}

/**
 * Return the connection through which the operation was made.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr PendingHandles::connection() const
{
    return ConnectionPtr(qobject_cast<Connection*>((Connection*) object().data()));
}

/**
 * Return the handle type specified in the operation.
 *
 * \return The target handle type as #HandleType.
 */
HandleType PendingHandles::handleType() const
{
    return mPriv->handleType;
}

/**
 * Return whether the operation was a handle request (as opposed to a
 * reference of existing handles).
 *
 * \return \c true if the operation was a request (== !isReference()), \c false otherwise.
 * \sa isReference()
 */
bool PendingHandles::isRequest() const
{
    return mPriv->isRequest;
}

/**
 * Return whether the operation was a handle reference (as opposed to a
 * request for new handles).
 *
 * \return \c true if the operation was a reference (== !isRequest()), \c false otherwise.
 * \sa isRequest()
 */
bool PendingHandles::isReference() const
{
    return !mPriv->isRequest;
}

/**
 * If the operation was a request (as returned by isRequest()), returns the
 * names of the entities for which handles were requested for. Otherwise,
 * returns an empty list.
 *
 * \return Reference to a list of the names of the entities.
 */
const QStringList &PendingHandles::namesRequested() const
{
    return mPriv->namesRequested;
}

QStringList PendingHandles::validNames() const
{
    if (!isFinished()) {
        warning() << "PendingHandles::validNames called before finished";
        return QStringList();
    } else if (!isValid()) {
        warning() << "PendingHandles::validNames called when not valid";
        return QStringList();
    }

    return mPriv->validNames;
}

QHash<QString, QPair<QString, QString> > PendingHandles::invalidNames() const
{
    if (!isFinished()) {
        warning() << "PendingHandles::invalidNames called before finished";
        return QHash<QString, QPair<QString, QString> >();
    }

    return mPriv->invalidNames;
}

/**
 * If the operation was a reference (as returned by isReference()), returns
 * the handles which were to be referenced. Otherwise, returns an empty
 * list.
 *
 * \return Reference to a list of the handles specified to be referenced.
 */
const UIntList &PendingHandles::handlesToReference() const
{
    return mPriv->handlesToReference;
}

/**
 * Return the now-referenced handles resulting from the operation. If the
 * operation has not (yet) finished successfully (isFinished() returns
 * <code>false</code>), the return value is undefined.
 *
 * For requests of new handles, <code>handles()[i]</code> will be the handle
 * corresponding to the entity name <code>namesToRequest()[i]</code>. For
 * references of existing handles, <code>handles()[i] ==
 * handlesToReference()[i]</code> will be true for any <code>i</code>.
 *
 * \return ReferencedHandles object containing the handles.
 */
ReferencedHandles PendingHandles::handles() const
{
    if (!isFinished()) {
        warning() << "PendingHandles::handles() called before finished";
        return ReferencedHandles();
    } else if (!isValid()) {
        warning() << "PendingHandles::handles() called when not valid";
        return ReferencedHandles();
    }

    return mPriv->handles;
}

UIntList PendingHandles::invalidHandles() const
{
    if (!isFinished()) {
        warning() << "PendingHandles::invalidHandles called before finished";
    }

    return mPriv->invalidHandles;
}

void PendingHandles::onRequestHandlesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UIntList> reply = *watcher;

    if (reply.isError()) {
        QDBusError error = reply.error();
        if (error.name() != TP_QT_ERROR_INVALID_HANDLE &&
            error.name() != TP_QT_ERROR_INVALID_ARGUMENT &&
            error.name() != TP_QT_ERROR_NOT_AVAILABLE) {
            // do not fallback
            foreach (const QString &name, mPriv->namesRequested) {
                mPriv->invalidNames.insert(name,
                        QPair<QString, QString>(error.name(),
                            error.message()));
            }
            setFinishedWithError(error);
            connection()->handleRequestLanded(mPriv->handleType);
            watcher->deleteLater();
            return;
        }

        if (mPriv->namesRequested.size() == 1) {
            debug().nospace() << " Failure: error " <<
                reply.error().name() << ": " <<
                reply.error().message();

            mPriv->invalidNames.insert(mPriv->namesRequested.first(),
                    QPair<QString, QString>(error.name(),
                        error.message()));
            setFinished();
            connection()->handleRequestLanded(mPriv->handleType);
            watcher->deleteLater();
            return;
        }

        // try to request one handles at a time
        foreach (const QString &name, mPriv->namesRequested) {
            QDBusPendingCallWatcher *watcher =
                new QDBusPendingCallWatcher(
                        connection()->baseInterface()->RequestHandles(
                            mPriv->handleType,
                            QStringList() << name),
                        this);
            connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(onRequestHandlesFallbackFinished(QDBusPendingCallWatcher*)));
            mPriv->idsForWatchers.insert(watcher, name);
        }
    } else {
        debug() << "Received reply to RequestHandles";
        mPriv->handles = ReferencedHandles(connection(),
                mPriv->handleType, reply.value());
        mPriv->validNames.append(mPriv->namesRequested);
        setFinished();
        connection()->handleRequestLanded(mPriv->handleType);
    }
}

void PendingHandles::onHoldHandlesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<void> reply = *watcher;

    debug() << "Received reply to HoldHandles";

    if (reply.isError()) {
        debug().nospace() << " Failure: error " <<
            reply.error().name() << ": " <<
            reply.error().message();

        QDBusError error = reply.error();
        if (error.name() != TP_QT_ERROR_INVALID_HANDLE &&
            error.name() != TP_QT_ERROR_INVALID_ARGUMENT &&
            error.name() != TP_QT_ERROR_NOT_AVAILABLE) {
            // do not fallback
            mPriv->invalidHandles = mPriv->handlesToReference;
            setFinishedWithError(error);
            watcher->deleteLater();
            return;
        }

        if (mPriv->handlesToReference.size() == 1) {
            debug().nospace() << " Failure: error " <<
                reply.error().name() << ": " <<
                reply.error().message();

            mPriv->invalidHandles = mPriv->handlesToReference;
            setFinished();
            watcher->deleteLater();
            return;
        }

        // try to request one handles at a time
        foreach (uint handle, mPriv->handlesToReference) {
            QDBusPendingCallWatcher *watcher =
                new QDBusPendingCallWatcher(
                        connection()->baseInterface()->HoldHandles(
                            mPriv->handleType,
                            UIntList() << handle),
                        this);
            connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(onHoldHandlesFallbackFinished(QDBusPendingCallWatcher*)));
            mPriv->handlesForWatchers.insert(watcher, handle);
        }
    } else {
        mPriv->handles = ReferencedHandles(connection(),
                mPriv->handleType, mPriv->handlesToReference);
        setFinished();
    }

    watcher->deleteLater();
}

void PendingHandles::onRequestHandlesFallbackFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UIntList> reply = *watcher;

    Q_ASSERT(mPriv->idsForWatchers.contains(watcher));
    QString id = mPriv->idsForWatchers.value(watcher);

    debug() << "Received reply to RequestHandles(" << id << ")";

    if (reply.isError()) {
        debug().nospace() << " Failure: error " << reply.error().name() << ": "
            << reply.error().message();

        // if the error is disconnected for example, fail immediately
        QDBusError error = reply.error();
        if (error.name() != TP_QT_ERROR_INVALID_HANDLE &&
            error.name() != TP_QT_ERROR_INVALID_ARGUMENT &&
            error.name() != TP_QT_ERROR_NOT_AVAILABLE) {
            foreach (const QString &name, mPriv->namesRequested) {
                mPriv->invalidNames.insert(name,
                        QPair<QString, QString>(error.name(),
                            error.message()));
            }
            setFinishedWithError(error);
            connection()->handleRequestLanded(mPriv->handleType);
            watcher->deleteLater();
            return;
        }

        mPriv->invalidNames.insert(id,
                QPair<QString, QString>(reply.error().name(),
                    reply.error().message()));
    } else {
        Q_ASSERT(reply.value().size() == 1);
        uint handle = reply.value().first();
        mPriv->handlesForIds.insert(id, handle);
    }

    if (++mPriv->requestsFinished == mPriv->namesRequested.size()) {
        if (mPriv->handlesForIds.size() == 0) {
            // all requests failed
            setFinished();
        } else {
            // all requests either failed or finished successfully

            // we need to return the handles in the same order as requested
            UIntList handles;
            foreach (const QString &name, mPriv->namesRequested) {
                if (!mPriv->invalidNames.contains(name)) {
                    Q_ASSERT(mPriv->handlesForIds.contains(name));
                    handles.append(mPriv->handlesForIds.value(name));
                    mPriv->validNames.append(name);
                }
            }
            mPriv->handles = ReferencedHandles(connection(),
                    mPriv->handleType, handles);

            setFinished();
        }

        debug() << " namesRequested:" << mPriv->namesRequested;
        debug() << " invalidNames  :" << mPriv->invalidNames;
        debug() << " validNames    :" << mPriv->validNames;

        connection()->handleRequestLanded(mPriv->handleType);
    }

    watcher->deleteLater();
}

void PendingHandles::onHoldHandlesFallbackFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<void> reply = *watcher;

    Q_ASSERT(mPriv->handlesForWatchers.contains(watcher));
    uint handle = mPriv->handlesForWatchers.value(watcher);

    debug() << "Received reply to HoldHandles(" << handle << ")";

    if (reply.isError()) {
        debug().nospace() << " Failure: error " << reply.error().name() << ": "
            << reply.error().message();

        // if the error is disconnected for example, fail immediately
        QDBusError error = reply.error();
        if (error.name() != TP_QT_ERROR_INVALID_HANDLE &&
            error.name() != TP_QT_ERROR_INVALID_ARGUMENT &&
            error.name() != TP_QT_ERROR_NOT_AVAILABLE) {
            mPriv->invalidHandles = mPriv->handlesToReference;
            setFinishedWithError(error);
            watcher->deleteLater();
            return;
        }

        mPriv->invalidHandles.append(handle);
    }

    if (++mPriv->requestsFinished == mPriv->namesRequested.size()) {
        // we need to return the handles in the same order as requested
        UIntList handles;
        foreach (uint handle, mPriv->handlesToReference) {
            if (!mPriv->invalidHandles.contains(handle)) {
                handles.append(handle);
            }
        }

        if (handles.size() != 0) {
            mPriv->handles = ReferencedHandles(connection(),
                    mPriv->handleType, handles);
        }

        setFinished();
    }

    watcher->deleteLater();
}

} // Tp
