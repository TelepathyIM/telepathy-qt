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

namespace Telepathy
{
namespace Client
{

struct PendingHandles::Private
{
    Connection* connection;
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

PendingHandles::PendingHandles(Connection* connection, uint handleType, const QStringList& names)
    : PendingOperation(connection), mPriv(new Private)
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
    : PendingOperation(connection), mPriv(new Private)
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

PendingHandles::~PendingHandles()
{
    delete mPriv;
}

Connection* PendingHandles::connection() const
{
    return mPriv->connection;
}

uint PendingHandles::handleType() const
{
    return mPriv->handleType;
}

bool PendingHandles::isRequest() const
{
    return mPriv->isRequest;
}

bool PendingHandles::isReference() const
{
    return !isRequest();
}

const QStringList& PendingHandles::namesRequested() const
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

const UIntList& PendingHandles::handlesToReference() const
{
    return mPriv->handlesToReference;
}

ReferencedHandles PendingHandles::handles() const
{
    return mPriv->handles;
}

void PendingHandles::onCallFinished(QDBusPendingCallWatcher* watcher)
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
            mPriv->handles = ReferencedHandles(connection(),
                    mPriv->handleType, reply.value());
            mPriv->validNames.append(mPriv->namesRequested);
            setFinished();
            connection()->handleRequestLanded(mPriv->handleType);
        }
    } else {
        QDBusPendingReply<void> reply = *watcher;

        debug() << "Received reply to HoldHandles";

        if (reply.isError()) {
            debug().nospace() << " Failure: error " << reply.error().name() << ": " << reply.error().message();
            setFinishedWithError(reply.error());
        } else {
            mPriv->handles = ReferencedHandles(connection(),
                    mPriv->handleType, handlesToReference());
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
            mPriv->handles = ReferencedHandles(connection(),
                    mPriv->handleType, handles);

            setFinished();
        }

        debug() << " namesRequested:" << mPriv->namesRequested;
        debug() << " invalidNames  :" << mPriv->invalidNames;
        debug() << " validNames    :" << mPriv->validNames;

        connection()->handleRequestLanded(handleType());
    }

    watcher->deleteLater();
}

} // Telepathy::Client
} // Telepathy
