/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/PendingReady>

#include "TelepathyQt4/_gen/pending-ready.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ReadyObject>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingReady::Private
{
    Private(const Features &requestedFeatures, QObject *object, const SharedPtr<RefCounted> &proxy) :
        requestedFeatures(requestedFeatures),
        object(object),
        proxy(proxy)
    {
    }

    Features requestedFeatures;
    QObject *object;
    SharedPtr<RefCounted> proxy;
};

/**
 * \class PendingReady
 * \headerfile TelepathyQt4/pending-ready.h <TelepathyQt4/PendingReady>
 *
 * \brief Class containing the features requested and the reply to a request
 * for an object to become ready. Instances of this class cannot be
 * constructed directly; the only way to get one is via Object::becomeReady().
 */

/**
 * Construct a PendingReady object, which will first wait for an operation to finish.
 *
 * When the operation given as finishFirst finishes, the resulting PendingReady object will first
 * wait for that operation to finish successfully, and then, if requestedFeatures is not empty,
 * calls ReadyObject::becomeReady(requestedFeatures), and finishes when that finished.
 *
 * If either nested operation fails, this PendingReady object will fail too with the same error name
 * and message as they reported.
 *
 * \param requestedFeatures Features to be made ready on the object.
 * \param finishFirst The object to finish first.
 * \param object The object that will become ready.
 */
PendingReady::PendingReady(PendingOperation *finishFirst, const Features &requestedFeatures,
        const SharedPtr<RefCounted> &proxy, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(requestedFeatures, 0, proxy))
{
    if (!finishFirst || finishFirst->isFinished()) {
        onFirstFinished(finishFirst);
    } else {
        connect(finishFirst,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onFirstFinished(Tp::PendingOperation*)));
    }
}

/**
 * Construct a PendingReady object.
 *
 * \param object The object that will become ready.
 */
PendingReady::PendingReady(const Features &requestedFeatures,
        QObject *object, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private(requestedFeatures, object, SharedPtr<RefCounted>()))
{
}

/**
 * Class destructor.
 */
PendingReady::~PendingReady()
{
    delete mPriv;
}

/**
 * Return the object through which the request was made.
 *
 * \return The object through which the request was made.
 */
QObject *PendingReady::object() const
{
    return mPriv->object;
}

SharedPtr<RefCounted> PendingReady::proxy() const
{
    return mPriv->proxy;
}

/**
 * Return the Features that were requested to become ready on the
 * object.
 *
 * \return Features The requested features
 */
Features PendingReady::requestedFeatures() const
{
    return mPriv->requestedFeatures;
}

void PendingReady::onFirstFinished(Tp::PendingOperation *first)
{
    if (first) {
        Q_ASSERT(first->isFinished());

        if (first->isError()) {
            warning() << "Prepare operation for" << object() << "failed with" <<
                first->errorName() << ":" << first->errorMessage();
            setFinishedWithError(first->errorName(), first->errorMessage());
            return;
        }
    }

    if (requestedFeatures().isEmpty()) {
        setFinished();
        return;
    }

    // API/ABI break TODO: Make some baseclass exist which is both a QObject and a ReadyObject...
    connect(dynamic_cast<ReadyObject *>(proxy().data())->becomeReady(requestedFeatures()),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onNestedFinished(Tp::PendingOperation*)));
}

void PendingReady::onNestedFinished(Tp::PendingOperation *nested)
{
    Q_ASSERT(nested->isFinished());

    if (nested->isValid()) {
        setFinished();
    } else {
        warning() << "Nested PendingReady for" << object() << "failed with"
            << nested->errorName() << ":" << nested->errorMessage();
        setFinishedWithError(nested->errorName(), nested->errorMessage());
    }
}

} // Tp
