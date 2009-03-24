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

#include <TelepathyQt4/Client/PendingHandles>

#include "TelepathyQt4/Client/_gen/pending-handles.moc.hpp"

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

namespace Telepathy
{
namespace Client
{

struct PendingHandles::Private
{
    Connection *connection;
    uint handleType;
    bool isRequest;
    QStringList namesRequested;
    UIntList handlesToReference;
    ReferencedHandles handles;
    ReferencedHandles alreadyHeld;

    QStringList validNames;
    QHash<QString, QPair<QString, QString> > invalidNames;

    // one to one requests (ids)
    QHash<QDBusPendingCallWatcher *, QString> idsForWatchers;
    QHash<QString, uint> handlesForIds;
    int requests;
};

/**
 * \class PendingHandles
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/Client/pending-handles.h>  <TelepathyQt4/Client/PendingHandles>
 *
 * Class containing the parameters of and the reply to an asynchronous handle
 * request/hold. Instances of this class cannot be constructed directly; the
 * only ways to get one are to use Connection::requestHandles() or
 * Connection::referenceHandles().
 */

PendingHandles::PendingHandles(Connection *connection, uint handleType,
        const QStringList &names)
    : PendingOperation(connection),
      mPriv(new Private)
{
    debug() << "PendingHandles(request)";

    mPriv->connection = connection;
    mPriv->handleType = handleType;
    mPriv->isRequest = true;
    mPriv->namesRequested = names;
    mPriv->requests = 0;

    // try to request all handles at once
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                connection->baseInterface()->RequestHandles(handleType, names),
                this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(onCallFinished(QDBusPendingCallWatcher *)));
}

PendingHandles::PendingHandles(Connection *connection, uint handleType,
        const UIntList &handles, const UIntList &alreadyHeld,
        const UIntList &notYetHeld)
    : PendingOperation(connection),
      mPriv(new Private)
{
    debug() << "PendingHandles(reference)";

    mPriv->connection = connection;
    mPriv->handleType = handleType;
    mPriv->isRequest = false;
    mPriv->handlesToReference = handles;
    mPriv->alreadyHeld = ReferencedHandles(connection, handleType, alreadyHeld);
    mPriv->requests = 0;

    if (notYetHeld.isEmpty()) {
        debug() << " All handles already held, finishing up instantly";
        mPriv->handles = mPriv->alreadyHeld;
        setFinished();
    } else {
        debug() << " Calling HoldHandles";

        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    connection->baseInterface()->HoldHandles(handleType, notYetHeld),
                    this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onCallFinished(QDBusPendingCallWatcher *)));
    }
}

/**
 * Class destructor.
 */
PendingHandles::~PendingHandles()
{
    delete mPriv;
}

/**
 * Returns the Connection object through which the operation was made.
 *
 * \return Pointer to the Connection.
 */
Connection *PendingHandles::connection() const
{
    return mPriv->connection;
}

/**
 * Returns the handle type specified in the operation.
 *
 * \return The handle type, as specified in #HandleType.
 */
uint PendingHandles::handleType() const
{
    return mPriv->handleType;
}

/**
 * Returns whether the operation was a handle request (as opposed to a
 * reference of existing handles).
 *
 * \sa isReference()
 *
 * \return Whether the operation was a request (== !isReference()).
 */
bool PendingHandles::isRequest() const
{
    return mPriv->isRequest;
}

/**
 * Returns whether the operation was a handle reference (as opposed to a
 * request for new handles).
 *
 * \sa isRequest()
 *
 * \return Whether the operation was a reference (== !isRequest()).
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
    } else if (!isValid()) {
        warning() << "PendingHandles::invalidNames called when not valid";
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
 * Returns the now-referenced handles resulting from the operation. If the
 * operation has not (yet) finished successfully (isFinished() returns
 * <code>false</code>), the return value is undefined.
 *
 * For requests of new handles, <code>handles()[i]</code> will be the handle
 * corresponding to the entity name <code>namesToRequest()[i]</code>. For
 * references of existing handles, <code>handles()[i] ==
 * handlesToReference()[i]</code> will be true for any <code>i</code>.
 *
 * \return ReferencedHandles instance containing the handles.
 */
ReferencedHandles PendingHandles::handles() const
{
    return mPriv->handles;
}

void PendingHandles::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    // Thanks QDBus for this the need for this error-handling code duplication
    if (mPriv->isRequest) {
        QDBusPendingReply<UIntList> reply = *watcher;

        if (reply.isError()) {
            if (mPriv->namesRequested.size() == 1) {
                debug().nospace() << " Failure: error " <<
                    reply.error().name() << ": " <<
                    reply.error().message();

                mPriv->invalidNames.insert(mPriv->namesRequested.first(),
                        QPair<QString, QString>(reply.error().name(),
                            reply.error().message()));
                setFinishedWithError(reply.error());
                connection()->handleRequestLanded(mPriv->handleType);
                return;
            }

            // try to request one handles at a time
            foreach (const QString &name, mPriv->namesRequested) {
                QDBusPendingCallWatcher *watcher =
                    new QDBusPendingCallWatcher(
                            mPriv->connection->baseInterface()->RequestHandles(
                                mPriv->handleType,
                                QStringList() << name),
                            this);
                connect(watcher,
                        SIGNAL(finished(QDBusPendingCallWatcher *)),
                        SLOT(onRequestHandlesFinished(QDBusPendingCallWatcher *)));
                mPriv->idsForWatchers.insert(watcher, name);
            }
        } else {
            debug() << "Received reply to RequestHandles";
            mPriv->handles = ReferencedHandles(mPriv->connection,
                    mPriv->handleType, reply.value());
            mPriv->validNames.append(mPriv->namesRequested);
            setFinished();
            mPriv->connection->handleRequestLanded(mPriv->handleType);
        }
    } else {
        QDBusPendingReply<void> reply = *watcher;

        debug() << "Received reply to HoldHandles";

        if (reply.isError()) {
            debug().nospace() << " Failure: error " <<
                reply.error().name() << ": " <<
                reply.error().message();
            setFinishedWithError(reply.error());
        } else {
            mPriv->handles = ReferencedHandles(mPriv->connection,
                    mPriv->handleType, mPriv->handlesToReference);
            setFinished();
        }
    }

    watcher->deleteLater();
}

void PendingHandles::onRequestHandlesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UIntList> reply = *watcher;

    Q_ASSERT(mPriv->idsForWatchers.contains(watcher));
    QString id = mPriv->idsForWatchers.value(watcher);

    debug() << "Received reply to RequestHandles(" << id << ")";

    if (reply.isError()) {
        debug().nospace() << " Failure: error " << reply.error().name() << ": "
            << reply.error().message();
        mPriv->invalidNames.insert(id,
                QPair<QString, QString>(reply.error().name(),
                    reply.error().message()));
    } else {
        Q_ASSERT(reply.value().size() == 1);
        uint handle = reply.value().first();
        mPriv->handlesForIds.insert(id, handle);
    }

    if (++mPriv->requests == mPriv->namesRequested.size()) {
        if (mPriv->handlesForIds.size() == 0) {
            // all requests failed
            setFinishedWithError(reply.error());
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
            mPriv->handles = ReferencedHandles(mPriv->connection,
                    mPriv->handleType, handles);

            setFinished();
        }

        debug() << " namesRequested:" << mPriv->namesRequested;
        debug() << " invalidNames  :" << mPriv->invalidNames;
        debug() << " validNames    :" << mPriv->validNames;

        mPriv->connection->handleRequestLanded(mPriv->handleType);
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
