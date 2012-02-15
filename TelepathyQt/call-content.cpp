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

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/DBus>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReadinessHelper>

namespace Tp
{

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
    Client::DBus::PropertiesInterface *properties;

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
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
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
            SIGNAL(StreamsRemoved(Tp::ObjectPathList)),
            SLOT(onStreamsRemoved(Tp::ObjectPathList)));

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    QLatin1String(TP_QT_IFACE_CALL_CONTENT)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
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
 * \headerfile TelepathyQt/call-channel.h <TelepathyQt/CallContent>
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

void CallContent::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        qWarning().nospace() << "Properties::GetAll(Call.Content) failed with" <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false, reply.error());
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to Properties::GetAll(Call.Content)";

    QVariantMap props = reply.value();

    mPriv->name = qdbus_cast<QString>(props[QLatin1String("Name")]);
    mPriv->type = qdbus_cast<uint>(props[QLatin1String("Type")]);
    mPriv->disposition = qdbus_cast<uint>(props[QLatin1String("Disposition")]);

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

    watcher->deleteLater();
}

void CallContent::onStreamsAdded(const ObjectPathList &streamsPaths)
{
    foreach (const QDBusObjectPath &streamPath, streamsPaths) {
        qDebug() << "Received Call::Content::StreamAdded for stream" << streamPath.path();

        if (mPriv->lookupStream(streamPath)) {
            qDebug() << "Stream already exists, ignoring";
            return;
        }

        mPriv->addStream(streamPath);
    }
}

void CallContent::onStreamsRemoved(const ObjectPathList &streamsPaths)
{
    foreach (const QDBusObjectPath &streamPath, streamsPaths) {
        qDebug() << "Received Call::Content::StreamRemoved for stream" << streamPath.path();

        CallStreamPtr stream = mPriv->lookupStream(streamPath);
        if (!stream) {
            qDebug() << "Stream does not exist, ignoring";
            return;
        }

        bool incomplete = mPriv->incompleteStreams.contains(stream);
        if (incomplete) {
            mPriv->incompleteStreams.removeOne(stream);
        } else {
            mPriv->streams.removeOne(stream);
        }

        if (isReady(FeatureCore) && !incomplete) {
            emit streamRemoved(stream);
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
 * \fn void CallContent::streamRemoved(const Tp::CallStreamPtr &stream);
 *
 * This signal is emitted when a new media stream is removed from this media
 * content.
 *
 * \param stream The media stream that was removed.
 * \sa streams()
 */

} // Tp
