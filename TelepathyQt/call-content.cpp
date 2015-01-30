/*
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#include <TelepathyQt/CallContent>

#include "TelepathyQt/_gen/call-content.moc.hpp"

#include "TelepathyQt/_gen/cli-call-content-body.hpp"
#include "TelepathyQt/_gen/cli-call-content.moc.hpp"

#include <TelepathyQt/debug-internal.h>

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/DBus>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/ReadinessHelper>

namespace Tp
{

/* ====== CallContent ====== */
struct TP_QT_NO_EXPORT CallContent::Private
{
    Private(CallContent *parent, const CallChannelPtr &channel);

    static void introspectMainProperties(Private *self);
    void checkIntrospectionCompleted();

    CallStreamPtr addStream(const QDBusObjectPath &streamPath);
    CallStreamPtr lookupStream(const QDBusObjectPath &streamPath);

    // Public object
    CallContent *parent;

    WeakPtr<CallChannel> channel;

    // Mandatory proxies
    Client::CallContentInterface *contentInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QString name;
    uint type;
    uint disposition;
    CallStreams streams;
    CallStreams incompleteStreams;
};

CallContent::Private::Private(CallContent *parent, const CallChannelPtr &channel)
    : parent(parent),
      channel(channel.data()),
      contentInterface(parent->interface<Client::CallContentInterface>()),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Features(),                                                            // dependsOnFeatures
        QStringList(),                                                             // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMainProperties,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

void CallContent::Private::introspectMainProperties(CallContent::Private *self)
{
    CallContent *parent = self->parent;
    CallChannelPtr channel = parent->channel();

    parent->connect(self->contentInterface,
            SIGNAL(StreamsAdded(Tp::ObjectPathList)),
            SLOT(onStreamsAdded(Tp::ObjectPathList)));
    parent->connect(self->contentInterface,
            SIGNAL(StreamsRemoved(Tp::ObjectPathList,Tp::CallStateReason)),
            SLOT(onStreamsRemoved(Tp::ObjectPathList,Tp::CallStateReason)));

    parent->connect(self->contentInterface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotMainProperties(Tp::PendingOperation*)));
}

void CallContent::Private::checkIntrospectionCompleted()
{
    if (!parent->isReady(FeatureCore) && incompleteStreams.size() == 0) {
        readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

CallStreamPtr CallContent::Private::addStream(const QDBusObjectPath &streamPath)
{
    CallStreamPtr stream = CallStreamPtr(
            new CallStream(CallContentPtr(parent), streamPath));
    incompleteStreams.append(stream);
    parent->connect(stream->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamReady(Tp::PendingOperation*)));
    return stream;
}

CallStreamPtr CallContent::Private::lookupStream(const QDBusObjectPath &streamPath)
{
    foreach (const CallStreamPtr &stream, streams) {
        if (stream->objectPath() == streamPath.path()) {
            return stream;
        }
    }
    foreach (const CallStreamPtr &stream, incompleteStreams) {
        if (stream->objectPath() == streamPath.path()) {
            return stream;
        }
    }
    return CallStreamPtr();
}

/**
 * \class CallContent
 * \ingroup clientchannel
 * \headerfile TelepathyQt/call-content.h <TelepathyQt/CallContent>
 *
 * \brief The CallContent class provides an object representing a Telepathy
 * Call.Content.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via CallChannel.
 *
 * See \ref async_model
 */

/**
 * Feature representing the core that needs to become ready to make the
 * CallContent object usable.
 *
 * Note that this feature must be enabled in order to use most CallContent
 * methods. See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature CallContent::FeatureCore = Feature(QLatin1String(CallContent::staticMetaObject.className()), 0);

/**
 * Construct a new CallContent object.
 *
 * \param channel The channel owning this media content.
 * \param name The object path of this media content.
 */
CallContent::CallContent(const CallChannelPtr &channel, const QDBusObjectPath &objectPath)
    : StatefulDBusProxy(channel->dbusConnection(), channel->busName(),
            objectPath.path(), FeatureCore),
      OptionalInterfaceFactory<CallContent>(this),
      mPriv(new Private(this, channel))
{
}

/**
 * Class destructor.
 */
CallContent::~CallContent()
{
    delete mPriv;
}

/**
 * Return the channel owning this media content.
 *
 * \return The channel owning this media content.
 */
CallChannelPtr CallContent::channel() const
{
    return CallChannelPtr(mPriv->channel);
}

/**
 * Return the name of this media content.
 *
 * \return The name of this media content.
 */
QString CallContent::name() const
{
    return mPriv->name;
}

/**
 * Return the type of this media content.
 *
 * \return The type of this media content.
 */
MediaStreamType CallContent::type() const
{
    return (MediaStreamType) mPriv->type;
}

/**
 * Return the disposition of this media content.
 *
 * \return The disposition of this media content.
 */
CallContentDisposition CallContent::disposition() const
{
    return (CallContentDisposition) mPriv->disposition;
}

/**
 * Return the media streams of this media content.
 *
 * \return A list of media streams of this media content.
 * \sa streamAdded(), streamRemoved()
 */
CallStreams CallContent::streams() const
{
    return mPriv->streams;
}

/**
 * Removes this media content from the call.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
PendingOperation *CallContent::remove()
{
    return new PendingVoid(mPriv->contentInterface->Remove(), CallContentPtr(this));
}

/**
 * Return whether sending DTMF events is supported on this content.
 * DTMF is only supported on audio contents that implement the
 * #TP_QT_IFACE_CALL_CONTENT_INTERFACE_DTMF interface.
 *
 * \returns \c true if DTMF is supported, or \c false otherwise.
 */
bool CallContent::supportsDTMF() const
{
    return hasInterface(TP_QT_IFACE_CALL_CONTENT_INTERFACE_DTMF);
}

/**
 * Start sending a DTMF tone on this media stream.
 *
 * Where possible, the tone will continue until stopDTMFTone() is called.
 * On certain protocols, it may only be possible to send events with a predetermined
 * length. In this case, the implementation may emit a fixed-length tone,
 * and the stopDTMFTone() method call should return #TP_QT_ERROR_NOT_AVAILABLE.
 *
 * If this content does not support the #TP_QT_IFACE_CALL_CONTENT_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * #TP_QT_ERROR_NOT_IMPLEMENTED.
 *
 * \param event A numeric event code from the #DTMFEvent enum.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa stopDTMFTone(), supportsDTMF()
 */
PendingOperation *CallContent::startDTMFTone(DTMFEvent event)
{
    if (!supportsDTMF()) {
        warning() << "CallContent::startDTMFTone() used with no dtmf interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("This CallContent does not support the dtmf interface"),
                CallContentPtr(this));
    }

    Client::CallContentInterfaceDTMFInterface *dtmfInterface =
        interface<Client::CallContentInterfaceDTMFInterface>();
    return new PendingVoid(dtmfInterface->StartTone(event), CallContentPtr(this));
}

/**
 * Stop sending any DTMF tone which has been started using the startDTMFTone()
 * method.
 *
 * If there is no current tone, the resulting PendingOperation will
 * finish successfully.
 *
 * If this content does not support the #TP_QT_IFACE_CALL_CONTENT_INTERFACE_DTMF
 * interface, the resulting PendingOperation will fail with error code
 * #TP_QT_ERROR_NOT_IMPLEMENTED.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa startDTMFTone(), supportsDTMF()
 */
PendingOperation *CallContent::stopDTMFTone()
{
    if (!supportsDTMF()) {
        warning() << "CallContent::stopDTMFTone() used with no dtmf interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("This CallContent does not support the dtmf interface"),
                CallContentPtr(this));
    }

    Client::CallContentInterfaceDTMFInterface *dtmfInterface =
        interface<Client::CallContentInterfaceDTMFInterface>();
    return new PendingVoid(dtmfInterface->StopTone(), CallContentPtr(this));
}

void CallContent::gotMainProperties(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallContentInterface::requestAllProperties() failed with" <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallContentInterface::requestAllProperties()";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
    Q_ASSERT(pvm);

    QVariantMap props = pvm->result();

    mPriv->name = qdbus_cast<QString>(props[QLatin1String("Name")]);
    mPriv->type = qdbus_cast<uint>(props[QLatin1String("Type")]);
    mPriv->disposition = qdbus_cast<uint>(props[QLatin1String("Disposition")]);
    setInterfaces(qdbus_cast<QStringList>(props[QLatin1String("Interfaces")]));

    ObjectPathList streamsPaths = qdbus_cast<ObjectPathList>(props[QLatin1String("Streams")]);
    if (streamsPaths.size() != 0) {
        foreach (const QDBusObjectPath &streamPath, streamsPaths) {
            CallStreamPtr stream = mPriv->lookupStream(streamPath);
            if (!stream) {
                mPriv->addStream(streamPath);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    }
}

void CallContent::onStreamsAdded(const ObjectPathList &streamsPaths)
{
    foreach (const QDBusObjectPath &streamPath, streamsPaths) {
        debug() << "Received Call::Content::StreamAdded for stream" << streamPath.path();

        if (mPriv->lookupStream(streamPath)) {
            debug() << "Stream already exists, ignoring";
            return;
        }

        mPriv->addStream(streamPath);
    }
}

void CallContent::onStreamsRemoved(const ObjectPathList &streamsPaths,
        const CallStateReason &reason)
{
    foreach (const QDBusObjectPath &streamPath, streamsPaths) {
        debug() << "Received Call::Content::StreamRemoved for stream" << streamPath.path();

        CallStreamPtr stream = mPriv->lookupStream(streamPath);
        if (!stream) {
            debug() << "Stream does not exist, ignoring";
            return;
        }

        bool incomplete = mPriv->incompleteStreams.contains(stream);
        if (incomplete) {
            mPriv->incompleteStreams.removeOne(stream);
        } else {
            mPriv->streams.removeOne(stream);
        }

        if (isReady(FeatureCore) && !incomplete) {
            emit streamRemoved(stream, reason);
        }

        mPriv->checkIntrospectionCompleted();
    }
}

void CallContent::onStreamReady(PendingOperation *op)
{
    PendingReady *pr = qobject_cast<PendingReady*>(op);
    CallStreamPtr stream = CallStreamPtr::qObjectCast(pr->proxy());

    if (op->isError() || !mPriv->incompleteStreams.contains(stream)) {
        mPriv->incompleteStreams.removeOne(stream);
        mPriv->checkIntrospectionCompleted();
        return;
    }

    mPriv->incompleteStreams.removeOne(stream);
    mPriv->streams.append(stream);

    if (isReady(FeatureCore)) {
        emit streamAdded(stream);
    }

    mPriv->checkIntrospectionCompleted();
}

/**
 * \fn void CallContent::streamAdded(const Tp::CallStreamPtr &stream);
 *
 * This signal is emitted when a new media stream is added to this media
 * content.
 *
 * \param stream The media stream that was added.
 * \sa streams()
 */

/**
 * \fn void CallContent::streamRemoved(const Tp::CallStreamPtr &stream, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when a new media stream is removed from this media
 * content.
 *
 * \param stream The media stream that was removed.
 * \param reason The reason for this removal.
 * \sa streams()
 */


/* ====== PendingCallContent ====== */
struct TP_QT_NO_EXPORT PendingCallContent::Private
{
    Private(PendingCallContent *parent, const CallChannelPtr &channel)
        : parent(parent),
          channel(channel)
    {
    }

    PendingCallContent *parent;
    CallChannelPtr channel;
    CallContentPtr content;
};

PendingCallContent::PendingCallContent(const CallChannelPtr &channel,
        const QString &name, MediaStreamType type, MediaStreamDirection direction)
    : PendingOperation(channel),
      mPriv(new Private(this, channel))
{
    Client::ChannelTypeCallInterface *callInterface =
        channel->interface<Client::ChannelTypeCallInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                callInterface->AddContent(name, type, direction), this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotContent(QDBusPendingCallWatcher*)));
}

PendingCallContent::~PendingCallContent()
{
    delete mPriv;
}

CallContentPtr PendingCallContent::content() const
{
    if (!isFinished() || !isValid()) {
        return CallContentPtr();
    }

    return mPriv->content;
}

void PendingCallContent::gotContent(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Call::AddContent failed with " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    QDBusObjectPath contentPath = reply.value();
    CallChannelPtr channel(mPriv->channel);
    CallContentPtr content = channel->lookupContent(contentPath);
    if (!content) {
        content = channel->addContent(contentPath);
    }

    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    connect(channel.data(),
            SIGNAL(contentRemoved(Tp::CallContentPtr,Tp::CallStateReason)),
            SLOT(onContentRemoved(Tp::CallContentPtr)));

    mPriv->content = content;

    watcher->deleteLater();
}

void PendingCallContent::onContentReady(PendingOperation *op)
{
    if (op->isError()) {
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    setFinished();
}

void PendingCallContent::onContentRemoved(const CallContentPtr &content)
{
    if (isFinished()) {
        return;
    }

    if (mPriv->content == content) {
        // the content was removed before becoming ready
        setFinishedWithError(TP_QT_ERROR_CANCELLED,
                QLatin1String("Content removed before ready"));
    }
}

} // Tp
