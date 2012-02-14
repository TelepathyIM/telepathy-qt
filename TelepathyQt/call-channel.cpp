/*
 * This file is part of TelepathyQt4Yell
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt4Yell/CallChannel>
#include "TelepathyQt4Yell/call-channel-internal.h"

#include "TelepathyQt4Yell/_gen/call-channel.moc.hpp"
#include "TelepathyQt4Yell/_gen/call-channel-internal.moc.hpp"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingVoid>

#include <QHash>

namespace Tpy
{

/* ====== CallStream ====== */
CallStream::Private::Private(CallStream *parent, const CallContentPtr &content)
    : parent(parent),
      content(content.data()),
      streamInterface(parent->interface<Client::CallStreamInterface>()),
      properties(parent->interface<Tp::Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      localSendingState(SendingStateNone),
      buildingRemoteMembers(false),
      currentRemoteMembersChangedInfo(0)
{
    Tp::ReadinessHelper::Introspectables introspectables;

    Tp::ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                        // makesSenseForStatuses
        Tp::Features(),                                                           // dependsOnFeatures
        QStringList(),                                                            // dependsOnInterfaces
        (Tp::ReadinessHelper::IntrospectFunc) &Private::introspectMainProperties,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
    readinessHelper->becomeReady(FeatureCore);
}

void CallStream::Private::introspectMainProperties(CallStream::Private *self)
{
    CallStream *parent = self->parent;

    parent->connect(self->streamInterface,
            SIGNAL(LocalSendingStateChanged(uint)),
            SLOT(onLocalSendingStateChanged(uint)));
    parent->connect(self->streamInterface,
            SIGNAL(RemoteMembersChanged(Tpy::ContactSendingStateMap,Tpy::UIntList)),
            SLOT(onRemoteMembersChanged(Tpy::ContactSendingStateMap,Tpy::UIntList)));

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TP_QT_YELL_IFACE_CALL_STREAM),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void CallStream::Private::processRemoteMembersChanged()
{
    if (buildingRemoteMembers) {
        return;
    }

    if (remoteMembersChangedQueue.isEmpty()) {
        if (!parent->isReady(FeatureCore)) {
            readinessHelper->setIntrospectCompleted(FeatureCore, true);
        }
        return;
    }

    currentRemoteMembersChangedInfo = remoteMembersChangedQueue.dequeue();

    QSet<uint> pendingRemoteMembers;
    for (ContactSendingStateMap::const_iterator i = currentRemoteMembersChangedInfo->updates.constBegin();
            i != currentRemoteMembersChangedInfo->updates.constEnd(); ++i) {
        pendingRemoteMembers.insert(i.key());
    }

    if (!pendingRemoteMembers.isEmpty()) {
        buildingRemoteMembers = true;

        Tp::ContactManagerPtr contactManager =
            parent->content()->channel()->connection()->contactManager();
        Tp::PendingContacts *contacts = contactManager->contactsForHandles(
                pendingRemoteMembers.toList());
        parent->connect(contacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotRemoteMembersContacts(Tp::PendingOperation*)));
    } else {
        processRemoteMembersChanged();
    }
}

/**
 * \class CallStream
 * \ingroup clientchannel
 * \headerfile TelepathyQt4Yell/call-channel.h <TelepathyQt4Yell/CallStream>
 *
 * \brief The CallStream class provides an object representing a Telepathy
 * Call.Stream.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is via CallContent.
 *
 * See \ref async_model
 */

/**
 * Feature representing the core that needs to become ready to make the
 * CallStream object usable.
 *
 * Note that this feature must be enabled in order to use most CallStream
 * methods. See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Tp::Feature CallStream::FeatureCore = Tp::Feature(QLatin1String(CallStream::staticMetaObject.className()), 0);

/**
 * Construct a new CallStream object.
 *
 * \param content The content owning this media stream.
 * \param objectPath The object path of this media stream.
 */
CallStream::CallStream(const CallContentPtr &content, const QDBusObjectPath &objectPath)
    : Tp::StatefulDBusProxy(content->dbusConnection(), content->busName(),
            objectPath.path(), FeatureCore),
      Tp::OptionalInterfaceFactory<CallStream>(this),
      mPriv(new Private(this, content))
{
}

/**
 * Class destructor.
 */
CallStream::~CallStream()
{
    delete mPriv;
}

/**
 * Return the channel owning this media stream.
 *
 * \return The channel owning this media stream.
 */
CallContentPtr CallStream::content() const
{
    return CallContentPtr(mPriv->content);
}

/**
 * Return the contacts whose the media stream is with.
 *
 * \deprecated Use contact() instead.
 *
 * \return The contacts whose the media stream is with.
 * \sa membersRemoved()
 */
Tp::Contacts CallStream::members() const
{
    return mPriv->remoteMembersContacts.values().toSet();
}

/**
 * Return the media stream local sending state.
 *
 * \return The media stream local sending state.
 * \sa localSendingStateChanged()
 */
SendingState CallStream::localSendingState() const
{
    return (SendingState) mPriv->localSendingState;
}

/**
 * Return the media stream remote sending state for a given \a contact.
 *
 * \deprecated Use remoteSendingState() instead.
 *
 * \return The media stream remote sending state for a contact.
 * \sa remoteSendingStateChanged()
 */
SendingState CallStream::remoteSendingState(const Tp::ContactPtr &contact) const
{
    if (!contact) {
        return SendingStateNone;
    }

    for (ContactSendingStateMap::const_iterator i = mPriv->remoteMembers.constBegin();
            i != mPriv->remoteMembers.constEnd(); ++i) {
        uint handle = i.key();
        SendingState sendingState = (SendingState) i.value();

        if (handle == contact->handle()[0]) {
            return sendingState;
        }
    }

    return SendingStateNone;
}

/**
 * Request that media starts or stops being sent on this media stream.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa localSendingStateChanged()
 */
Tp::PendingOperation *CallStream::requestSending(bool send)
{
    return new Tp::PendingVoid(mPriv->streamInterface->SetSending(send), CallStreamPtr(this));
}

/**
 * Request that a remote \a contact stops or starts sending on this media stream.
 *
 * \deprecated Use requestReceiving(bool receive) instead.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa remoteSendingStateChanged()
 */
Tp::PendingOperation *CallStream::requestReceiving(const Tp::ContactPtr &contact, bool receive)
{
    if (!contact) {
        return new Tp::PendingFailure(TP_QT_YELL_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid contact"), CallStreamPtr(this));
    }

    return new Tp::PendingVoid(mPriv->streamInterface->RequestReceiving(contact->handle()[0], receive),
            CallStreamPtr(this));
}

void CallStream::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        qWarning().nospace() << "Properties::GetAll(Call.Stream) failed with " <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to Properties::GetAll(Call.Stream)";

    QVariantMap props = reply.value();

    mPriv->localSendingState = qdbus_cast<uint>(props[QLatin1String("LocalSendingState")]);

    ContactSendingStateMap remoteMembers =
        qdbus_cast<ContactSendingStateMap>(props[QLatin1String("RemoteMembers")]);

    mPriv->remoteMembersChangedQueue.enqueue(new Private::RemoteMembersChangedInfo(
                remoteMembers, UIntList()));
    mPriv->processRemoteMembersChanged();

    watcher->deleteLater();
}

void CallStream::gotRemoteMembersContacts(Tp::PendingOperation *op)
{
    Tp::PendingContacts *pc = qobject_cast<Tp::PendingContacts *>(op);

    mPriv->buildingRemoteMembers = false;

    if (!pc->isValid()) {
        qWarning().nospace() << "Getting contacts failed with " <<
            pc->errorName() << ":" << pc->errorMessage() << ", ignoring";
        mPriv->processRemoteMembersChanged();
        return;
    }

    QMap<uint, Tp::ContactPtr> removed;

    for (ContactSendingStateMap::const_iterator i =
                mPriv->currentRemoteMembersChangedInfo->updates.constBegin();
            i != mPriv->currentRemoteMembersChangedInfo->updates.constEnd(); ++i) {
        mPriv->remoteMembers.insert(i.key(), i.value());
    }

    foreach (const Tp::ContactPtr &contact, pc->contacts()) {
        mPriv->remoteMembersContacts.insert(contact->handle()[0], contact);
    }

    foreach (uint handle, mPriv->currentRemoteMembersChangedInfo->removed) {
        mPriv->remoteMembers.remove(handle);
        if (isReady(FeatureCore) && mPriv->remoteMembersContacts.contains(handle)) {
            removed.insert(handle, mPriv->remoteMembersContacts[handle]);

            // make sure we don't have updates for removed contacts
            mPriv->currentRemoteMembersChangedInfo->updates.remove(handle);
        }
        mPriv->remoteMembersContacts.remove(handle);
    }

    foreach (uint handle, pc->invalidHandles()) {
        mPriv->remoteMembers.remove(handle);
        if (isReady(FeatureCore) && mPriv->remoteMembersContacts.contains(handle)) {
            removed.insert(handle, mPriv->remoteMembersContacts[handle]);

            // make sure we don't have updates for invalid handles
            mPriv->currentRemoteMembersChangedInfo->updates.remove(handle);
        }
        mPriv->remoteMembersContacts.remove(handle);
    }

    if (isReady(FeatureCore)) {
        CallChannelPtr channel(content()->channel());
        QHash<Tp::ContactPtr, SendingState> remoteSendingStates;
        for (ContactSendingStateMap::const_iterator i =
                    mPriv->currentRemoteMembersChangedInfo->updates.constBegin();
                i != mPriv->currentRemoteMembersChangedInfo->updates.constEnd(); ++i) {
            uint handle = i.key();
            SendingState sendingState = (SendingState) i.value();

            Q_ASSERT(mPriv->remoteMembersContacts.contains(handle));
            remoteSendingStates.insert(mPriv->remoteMembersContacts[handle], sendingState);

            mPriv->remoteMembers.insert(i.key(), i.value());
        }

        if (!remoteSendingStates.isEmpty()) {
            emit remoteSendingStateChanged(remoteSendingStates);
        }

        if (!removed.isEmpty()) {
            emit remoteMembersRemoved(removed.values().toSet());
        }
    }

    mPriv->processRemoteMembersChanged();
}

void CallStream::onLocalSendingStateChanged(uint state)
{
    mPriv->localSendingState = state;
    emit localSendingStateChanged((SendingState) state);
}

void CallStream::onRemoteMembersChanged(const ContactSendingStateMap &updates, const UIntList &removed)
{
    if (updates.isEmpty() && removed.isEmpty()) {
        qDebug() << "Received Call::Content::RemoteMembersChanged with 0 changes and "
            "updates, skipping it";
        return;
    }

    qDebug() << "Received Call::Content::RemoteMembersChanged with" << updates.size() <<
        "and " << removed.size() << "removed";
    mPriv->remoteMembersChangedQueue.enqueue(new Private::RemoteMembersChangedInfo(updates, removed));
    mPriv->processRemoteMembersChanged();
}

/**
 * \fn void CallStream::localSendingStateChanged(Tpy::SendingState localSendingState);
 *
 * This signal is emitted when the local sending state of this media stream
 * changes.
 *
 * \param localSendingState The new local sending state of this media stream.
 * \sa localSendingState()
 */

/**
 * \fn void CallStream::remoteSendingStateChanged(const QHash<Tp::ContactPtr, Tpy::SendingState> &remoteSendingStates);
 *
 * This signal is emitted when any remote sending state of this media stream
 * changes.
 *
 * \param remoteSendingStates The new remote sending states of this media stream.
 * \sa remoteSendingState()
 */

/**
 * \fn void CallStream::remoteMembersRemoved(const Tp::Contacts &members);
 *
 * This signal is emitted when one or more members of this stream are removed.
 *
 * \param members The members that were removed from this media stream.
 * \sa members()
 */

/* ====== PendingCallContent ====== */
PendingCallContent::PendingCallContent(const CallChannelPtr &channel,
        const QString &name, Tp::MediaStreamType type)
    : Tp::PendingOperation(channel),
      mPriv(new Private(this, channel))
{
    Client::ChannelTypeCallInterface *callInterface =
        channel->interface<Client::ChannelTypeCallInterface>();
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                callInterface->AddContent(name, type), this);
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
        qWarning().nospace() << "Call::AddContent failed with " <<
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
            SIGNAL(contentRemoved(Tpy::CallContentPtr)),
            SLOT(onContentRemoved(Tpy::CallContentPtr)));

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
        setFinishedWithError(TP_QT_YELL_ERROR_CANCELLED,
                QLatin1String("Content removed before ready"));
    }
}

/* ====== CallContent ====== */
CallContent::Private::Private(CallContent *parent, const CallChannelPtr &channel)
    : parent(parent),
      channel(channel.data()),
      contentInterface(parent->interface<Client::CallContentInterface>()),
      properties(parent->interface<Tp::Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper())
{
    Tp::ReadinessHelper::Introspectables introspectables;

    Tp::ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Tp::Features(),                                                            // dependsOnFeatures
        QStringList(),                                                             // dependsOnInterfaces
        (Tp::ReadinessHelper::IntrospectFunc) &Private::introspectMainProperties,
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
            SIGNAL(StreamsAdded(Tpy::ObjectPathList)),
            SLOT(onStreamsAdded(Tpy::ObjectPathList)));
    parent->connect(self->contentInterface,
            SIGNAL(StreamsRemoved(Tpy::ObjectPathList)),
            SLOT(onStreamsRemoved(Tpy::ObjectPathList)));

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    QLatin1String(TP_QT_YELL_IFACE_CALL_CONTENT)),
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
 * \headerfile TelepathyQt4Yell/call-channel.h <TelepathyQt4Yell/CallContent>
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
const Tp::Feature CallContent::FeatureCore = Tp::Feature(QLatin1String(CallContent::staticMetaObject.className()), 0);

/**
 * Construct a new CallContent object.
 *
 * \param channel The channel owning this media content.
 * \param name The object path of this media content.
 */
CallContent::CallContent(const CallChannelPtr &channel, const QDBusObjectPath &objectPath)
    : Tp::StatefulDBusProxy(channel->dbusConnection(), channel->busName(),
            objectPath.path(), FeatureCore),
      Tp::OptionalInterfaceFactory<CallContent>(this),
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
Tp::MediaStreamType CallContent::type() const
{
    return (Tp::MediaStreamType) mPriv->type;
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

void CallContent::onStreamReady(Tp::PendingOperation *op)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
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
 * \fn void CallContent::streamAdded(const Tpy::CallStreamPtr &stream);
 *
 * This signal is emitted when a new media stream is added to this media
 * content.
 *
 * \param stream The media stream that was added.
 * \sa streams()
 */

/**
 * \fn void CallContent::streamRemoved(const Tpy::CallStreamPtr &stream);
 *
 * This signal is emitted when a new media stream is removed from this media
 * content.
 *
 * \param stream The media stream that was removed.
 * \sa streams()
 */

/* ====== CallChannel ====== */
CallChannel::Private::Private(CallChannel *parent)
    : parent(parent),
      callInterface(parent->interface<Client::ChannelTypeCallInterface>()),
      properties(parent->interface<Tp::Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      state(CallStateUnknown),
      flags((uint) -1),
      hardwareStreaming(false),
      initialTransportType(StreamTransportTypeUnknown),
      initialAudio(false),
      initialVideo(false),
      mutableContents(false),
      localHoldState(Tp::LocalHoldStateUnheld),
      localHoldStateReason(Tp::LocalHoldStateReasonNone)
{
    Tp::ReadinessHelper::Introspectables introspectables;

    Tp::ReadinessHelper::Introspectable introspectableContents(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Tp::Features() << Channel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList(),                                                             // dependsOnInterfaces
        (Tp::ReadinessHelper::IntrospectFunc) &Private::introspectContents,
        this);
    introspectables[FeatureContents] = introspectableContents;

    Tp::ReadinessHelper::Introspectable introspectableLocalHoldState(
        QSet<uint>() << 0,                                                         // makesSenseForStatuses
        Tp::Features() << Channel::FeatureCore,                                    // dependsOnFeatures (core)
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_HOLD,                      // dependsOnInterfaces
        (Tp::ReadinessHelper::IntrospectFunc) &Private::introspectLocalHoldState,
        this);
    introspectables[FeatureLocalHoldState] = introspectableLocalHoldState;

    readinessHelper->addIntrospectables(introspectables);
}

CallChannel::Private::~Private()
{
}

void CallChannel::Private::introspectContents(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;

    parent->connect(self->callInterface,
            SIGNAL(ContentAdded(QDBusObjectPath)),
            SLOT(onContentAdded(QDBusObjectPath)));
    parent->connect(self->callInterface,
            SIGNAL(ContentRemoved(QDBusObjectPath)),
            SLOT(onContentRemoved(QDBusObjectPath)));
    parent->connect(self->callInterface,
            SIGNAL(CallStateChanged(uint,uint,Tpy::CallStateReason,QVariantMap)),
            SLOT(onCallStateChanged(uint,uint,Tpy::CallStateReason,QVariantMap)));
    // TODO handle CallMembersChanged

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(
                    QLatin1String(TP_QT_YELL_IFACE_CHANNEL_TYPE_CALL)),
                parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher*)));
}

void CallChannel::Private::introspectLocalHoldState(CallChannel::Private *self)
{
    CallChannel *parent = self->parent;
    Tp::Client::ChannelInterfaceHoldInterface *holdInterface =
        parent->interface<Tp::Client::ChannelInterfaceHoldInterface>();

    parent->connect(holdInterface,
            SIGNAL(HoldStateChanged(uint,uint)),
            SLOT(onLocalHoldStateChanged(uint,uint)));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            holdInterface->GetHoldState(), parent);
    parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(gotLocalHoldState(QDBusPendingCallWatcher*)));
}

CallChannel::Private::PendingRemoveContent::PendingRemoveContent(const CallChannelPtr &channel,
        const CallContentPtr &content, ContentRemovalReason reason,
        const QString &detailedReason, const QString &message)
    : PendingOperation(channel),
      channel(channel),
      content(content)
{
    connect(channel.data(),
            SIGNAL(contentRemoved(Tpy::CallContentPtr)),
            SLOT(onContentRemoved(Tpy::CallContentPtr)));

    Client::CallContentInterface *contentInterface =
        content->interface<Client::CallContentInterface>();
    connect(new QDBusPendingCallWatcher(
                contentInterface->Remove(reason, detailedReason, message),
                this),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onCallFinished(QDBusPendingCallWatcher*)));
}

void CallChannel::Private::PendingRemoveContent::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    if (isFinished()) {
        watcher->deleteLater();
        return;
    }

    if (watcher->isError()) {
        setFinishedWithError(watcher->error());
    } else {
        if (!channel->contents().contains(content)) {
            setFinished();
        }
    }

    watcher->deleteLater();
}

void CallChannel::Private::PendingRemoveContent::onContentRemoved(const CallContentPtr &contentRemoved)
{
    if (!isFinished() && contentRemoved == content) {
        setFinished();
    }
}

/**
 * \class CallChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4Yell/call-channel.h <TelepathyQt4Yell/CallChannel>
 *
 * \brief The CallChannel class provides an object representing a
 * Telepathy channel of type Call.
 */

/**
 * Feature used in order to access content specific methods.
 *
 * See media content specific methods' documentation for more details.
 */
const Tp::Feature CallChannel::FeatureContents = Tp::Feature(QLatin1String(CallChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to access local hold state info.
 *
 * See local hold state specific methods' documentation for more details.
 */
const Tp::Feature CallChannel::FeatureLocalHoldState = Tp::Feature(QLatin1String(CallChannel::staticMetaObject.className()), 1);

/**
 * Create a new CallChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A CallChannelPtr object pointing to the newly created
 *         CallChannel object.
 */
CallChannelPtr CallChannel::create(const Tp::ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return CallChannelPtr(new CallChannel(connection, objectPath, immutableProperties));
}

/**
 * Construct a new CallChannel associated with the given object on the same
 * service as the given connection.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \param coreFeature The core feature of the channel type. The corresponding introspectable
 *                     should depend on Channel::FeatureCore.
 */
CallChannel::CallChannel(const Tp::ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Tp::Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
CallChannel::~CallChannel()
{
    delete mPriv;
}

/**
 * Return the current state of this call.
 *
 * \return The current state of this call.
 * \sa stateChanged()
 */
CallState CallChannel::state() const
{
    return (CallState) mPriv->state;
}

/**
 * Return the flags representing the status of this call as a whole, providing more specific
 * information than state().
 *
 * \return The flags representing the status of this call.
 * \sa stateChanged()
 */
CallFlags CallChannel::flags() const
{
    return (CallFlags) mPriv->flags;
}

/**
 * Return the reason for the last change to the state() and/or flags().
 *
 * \return The reason for the last change to the state() and/or flags().
 * \sa stateChanged()
 */
CallStateReason CallChannel::stateReason() const
{
    return mPriv->stateReason;
}

/**
 * Return optional extensible details for the state(), flags() and/or stateReason().
 *
 * \return The optional extensible details for the state(), flags() and/or stateReason().
 * \sa stateChanged()
 */
QVariantMap CallChannel::stateDetails() const
{
    return mPriv->stateDetails;
}

/**
 * Check whether media streaming by the handler is required for this channel.
 *
 * If \c false, all of the media streaming is done by some mechanism outside the scope
 * of Telepathy, otherwise the handler is responsible for doing the actual media streaming.
 *
 * \return \c true if required, \c false otherwise.
 */
bool CallChannel::handlerStreamingRequired() const
{
    return !mPriv->hardwareStreaming;
}

/**
 * Return the initial transport type used for this call if set on a requested channel.
 *
 * Where not applicable, this property is defined to be #StreamTransportTypeUnknown, in
 * particular, on CMs with hardware streaming.
 *
 * \return The initial transport type used for this call.
 */
StreamTransportType CallChannel::initialTransportType() const
{
    return (StreamTransportType) mPriv->initialTransportType;
}

/**
 * Return whether an audio content has already been requested.
 *
 * \return \c true if an audio content has already been requested, \c false otherwise.
 */
bool CallChannel::hasInitialAudio() const
{
    return mPriv->initialAudio;
}

/**
 * Return whether a video content has already been requested.
 *
 * \return \c true if an video content has already been requested, \c false otherwise.
 */
bool CallChannel::hasInitialVideo() const
{
    return mPriv->initialVideo;
}

/**
 * Return the name of the initial audio content if hasInitialAudio() returns \c true.
 *
 * \return The name of the initial audio content.
 */
QString CallChannel::initialAudioName() const
{
    return mPriv->initialAudioName;
}

/**
 * Return the name of the initial video content if hasInitialVideo() returns \c true.
 *
 * \return The name of the initial video content.
 */
QString CallChannel::initialVideoName() const
{
    return mPriv->initialVideoName;
}

/**
 * Return whether a stream of a different content type can be added after the Channel has
 * been requested.
 *
 * \return \c true if a stream of different content type can be added after the Channel has been
 *         requested, \c false otherwise.
 */
bool CallChannel::hasMutableContents() const
{
    return mPriv->mutableContents;
}

/**
 * Accept an incoming call.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
Tp::PendingOperation *CallChannel::accept()
{
    return new Tp::PendingVoid(mPriv->callInterface->Accept(), CallChannelPtr(this));
}

/**
 * Request that the call is ended.
 *
 * \deprecated Use hangupCall() instead.
 *
 * \param reason A generic hangup reason.
 * \param detailedReason A more specific reason for the call hangup, if one is
 *                       available, or an empty string otherwise.
 * \param message A human-readable message to be sent to the remote contact(s).
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 */
Tp::PendingOperation *CallChannel::hangup(CallStateChangeReason reason,
        const QString &detailedReason, const QString &message)
{
    return new Tp::PendingVoid(mPriv->callInterface->Hangup(reason, detailedReason, message),
            CallChannelPtr(this));
}

/**
 * Return a list of media contents in this channel.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \return The contents in this channel.
 * \sa contentAdded(), contentRemoved(), contentsForType(), requestContent()
 */
CallContents CallChannel::contents() const
{
    return mPriv->contents;
}

/**
 * Return a list of media contents in this channel for the given type \a type.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \param type The interested type.
 * \return A list of media contents in this channel for the given type \a type.
 * \sa contentAdded(), contentRemoved(), contents(), requestContent()
 */
CallContents CallChannel::contentsForType(Tp::MediaStreamType type) const
{
    CallContents contents;
    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->type() == type) {
            contents.append(content);
        }
    }
    return contents;
}

/**
 * Request that media content be established to exchange the given type \a type
 * of media.
 *
 * This methods requires CallChannel::FeatureContents to be enabled.
 *
 * \return A PendingCallContent which will emit PendingCallContent::finished
 *         when the call has finished.
 * \sa contentAdded(), contents(), contentsForType()
 */
PendingCallContent *CallChannel::requestContent(const QString &name, Tp::MediaStreamType type)
{
    return new PendingCallContent(CallChannelPtr(this), name, type);
}

/**
 * Remove the content.
 *
 * \param content The content to remove.
 * \param reason A removal reason.
 * \param detailedReason A more specific reason for the removal, if one is available, or an empty
 *                       string.
 * \param message A human-readable message for the reason for the removal, such as "Fatal
 *                streaming failure" or "no codec intersection". This property can be left empty if
 *                no reason is to be given.
 */
Tp::PendingOperation *CallChannel::removeContent(const CallContentPtr &content,
        ContentRemovalReason reason, const QString &detailedReason, const QString &message)
{
    return new Private::PendingRemoveContent(CallChannelPtr(this),
            content, reason, detailedReason, message);
}

/**
 * Return whether the local user has placed this channel on hold.
 *
 * This method requires CallChannel::FeatureHoldState to be enabled.
 *
 * \return The channel local hold state.
 * \sa requestHold(), localHoldStateChanged()
 */
Tp::LocalHoldState CallChannel::localHoldState() const
{
    if (!isReady(FeatureLocalHoldState)) {
        qWarning() << "CallChannel::localHoldState() used with FeatureLocalHoldState not ready";
    } else if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        qWarning() << "CallChannel::localHoldStateReason() used with no hold interface";
    }

    return (Tp::LocalHoldState) mPriv->localHoldState;
}

/**
 * Return the reason why localHoldState() changed to its current value.
 *
 * This method requires CallChannel::FeatureLocalHoldState to be enabled.
 *
 * \return The channel local hold state reason.
 * \sa requestHold(), localHoldStateChanged()
 */
Tp::LocalHoldStateReason CallChannel::localHoldStateReason() const
{
    if (!isReady(FeatureLocalHoldState)) {
        qWarning() << "CallChannel::localHoldStateReason() used with FeatureLocalHoldState not ready";
    } else if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        qWarning() << "CallChannel::localHoldStateReason() used with no hold interface";
    }

    return (Tp::LocalHoldStateReason) mPriv->localHoldStateReason;
}

/**
 * Request that the channel be put on hold (be instructed not to send any media
 * streams to you) or be taken off hold.
 *
 * If the connection manager can immediately tell that the requested state
 * change could not possibly succeed, the resulting PendingOperation will fail
 * with error code #TP_QT_YELL_ERROR_NOT_AVAILABLE.
 * If the requested state is the same as the current state, the resulting
 * PendingOperation will finish successfully.
 *
 * Otherwise, the channel's local hold state will change to
 * Tp::LocalHoldStatePendingHold or Tp::LocalHoldStatePendingUnhold (as
 * appropriate), then the resulting PendingOperation will finish successfully.
 *
 * The eventual success or failure of the request is indicated by a subsequent
 * localHoldStateChanged() signal, changing the local hold state to
 * Tp::LocalHoldStateHeld or Tp::LocalHoldStateUnheld.
 *
 * If the channel has multiple streams, and the connection manager succeeds in
 * changing the hold state of one stream but fails to change the hold state of
 * another, it will attempt to revert all streams to their previous hold
 * states.
 *
 * If the channel does not support the #TP_QT_IFACE_CHANNEL_INTERFACE_HOLD
 * interface, the PendingOperation will fail with error code
 * #TP_QT_YELL_ERROR_NOT_IMPLEMENTED.
 *
 * \param hold A boolean indicating whether or not the channel should be on hold
 * \return A %PendingOperation, which will emit PendingOperation::finished
 *         when the request finishes.
 * \sa localHoldState(), localHoldStateReason(), localHoldStateChanged()
 */
Tp::PendingOperation *CallChannel::requestHold(bool hold)
{
    if (!hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_HOLD)) {
        qWarning() << "CallChannel::requestHold() used with no hold interface";
        return new Tp::PendingFailure(TP_QT_YELL_ERROR_NOT_IMPLEMENTED,
                QLatin1String("CallChannel does not support hold interface"),
                CallChannelPtr(this));
    }

    Tp::Client::ChannelInterfaceHoldInterface *holdInterface =
        interface<Tp::Client::ChannelInterfaceHoldInterface>();
    return new Tp::PendingVoid(holdInterface->RequestHold(hold), CallChannelPtr(this));
}

void CallChannel::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        qWarning().nospace() << "Properties::GetAll(Call) failed with " <<
            reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents,
                false, reply.error());
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to Properties::GetAll(Call)";

    QVariantMap props = reply.value();

    // TODO bind CallMembers
    mPriv->state = qdbus_cast<uint>(props[QLatin1String("CallState")]);
    mPriv->flags = qdbus_cast<uint>(props[QLatin1String("CallFlags")]);
    // TODO Add high-level classes for CallStateReason/Details
    mPriv->stateReason = qdbus_cast<CallStateReason>(props[QLatin1String("CallStateReason")]);
    mPriv->stateDetails = qdbus_cast<QVariantMap>(props[QLatin1String("CallStateDetails")]);
    mPriv->hardwareStreaming = qdbus_cast<bool>(props[QLatin1String("HardwareStreaming")]);
    mPriv->initialTransportType = qdbus_cast<uint>(props[QLatin1String("InitialTransport")]);
    mPriv->initialAudio = qdbus_cast<bool>(props[QLatin1String("Audio")]);
    mPriv->initialVideo = qdbus_cast<bool>(props[QLatin1String("Video")]);
    mPriv->initialAudioName = qdbus_cast<QString>(props[QLatin1String("AudioName")]);
    mPriv->initialVideoName = qdbus_cast<QString>(props[QLatin1String("VideoName")]);
    mPriv->mutableContents = qdbus_cast<bool>(props[QLatin1String("MutableContents")]);

    ObjectPathList contentsPaths = qdbus_cast<ObjectPathList>(props[QLatin1String("Contents")]);
    if (contentsPaths.size() > 0) {
        foreach (const QDBusObjectPath &contentPath, contentsPaths) {
            CallContentPtr content = lookupContent(contentPath);
            if (!content) {
                addContent(contentPath);
            }
        }
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }

    watcher->deleteLater();
}

void CallChannel::onContentAdded(const QDBusObjectPath &contentPath)
{
    qDebug() << "Received Call::ContentAdded for content" << contentPath.path();

    if (lookupContent(contentPath)) {
        qDebug() << "Content already exists, ignoring";
        return;
    }

    addContent(contentPath);
}

void CallChannel::onContentRemoved(const QDBusObjectPath &contentPath)
{
    qDebug() << "Received Call::ContentRemoved for content" << contentPath.path();

    CallContentPtr content = lookupContent(contentPath);
    if (!content) {
        qDebug() << "Content does not exist, ignoring";
        return;
    }

    bool incomplete = mPriv->incompleteContents.contains(content);
    if (incomplete) {
        mPriv->incompleteContents.removeOne(content);
    } else {
        mPriv->contents.removeOne(content);
    }

    if (isReady(FeatureContents) && !incomplete) {
        emit contentRemoved(content);
    }

    // the content was added/removed before become ready
    if (!isReady(FeatureContents) &&
        mPriv->contents.size() == 0 &&
        mPriv->incompleteContents.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void CallChannel::onContentReady(Tp::PendingOperation *op)
{
    Tp::PendingReady *pr = qobject_cast<Tp::PendingReady*>(op);
    CallContentPtr content = CallContentPtr::qObjectCast(pr->proxy());

    if (op->isError()) {
        mPriv->incompleteContents.removeOne(content);
        if (!isReady(FeatureContents) && mPriv->incompleteContents.size() == 0) {
            // let's not fail because a content could not become ready
            mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
        }
        return;
    }

    // the content was removed before become ready
    if (!mPriv->incompleteContents.contains(content)) {
        if (!isReady(FeatureContents) &&
            mPriv->incompleteContents.size() == 0) {
            mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
        }
        return;
    }

    mPriv->incompleteContents.removeOne(content);
    mPriv->contents.append(content);

    if (isReady(FeatureContents)) {
        emit contentAdded(content);
    }

    if (!isReady(FeatureContents) && mPriv->incompleteContents.size() == 0) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureContents, true);
    }
}

void CallChannel::onCallStateChanged(uint state, uint flags,
        const Tpy::CallStateReason &stateReason, const QVariantMap &stateDetails)
{
    if (mPriv->state == state && mPriv->flags == flags && mPriv->stateReason == stateReason &&
        mPriv->stateDetails == stateDetails) {
        // nothing changed
        return;
    }

    mPriv->state = state;
    mPriv->flags = flags;
    mPriv->stateReason = stateReason;
    mPriv->stateDetails = stateDetails;
    emit stateChanged((CallState) state);
}

void CallChannel::gotLocalHoldState(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<uint, uint> reply = *watcher;
    if (reply.isError()) {
        qWarning().nospace() << "Call::Hold::GetHoldState() failed with " <<
            reply.error().name() << ": " << reply.error().message();
        qDebug() << "Ignoring error getting hold state and assuming we're not on hold";
        onLocalHoldStateChanged(mPriv->localHoldState, mPriv->localHoldStateReason);
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to Call::Hold::GetHoldState()";
    onLocalHoldStateChanged(reply.argumentAt<0>(), reply.argumentAt<1>());
    watcher->deleteLater();
}

void CallChannel::onLocalHoldStateChanged(uint localHoldState, uint localHoldStateReason)
{
    bool changed = false;
    if (mPriv->localHoldState != localHoldState ||
        mPriv->localHoldStateReason != localHoldStateReason) {
        changed = true;
    }

    mPriv->localHoldState = localHoldState;
    mPriv->localHoldStateReason = localHoldStateReason;

    if (!isReady(FeatureLocalHoldState)) {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureLocalHoldState, true);
    } else {
        if (changed) {
            emit localHoldStateChanged((Tp::LocalHoldState) mPriv->localHoldState,
                    (Tp::LocalHoldStateReason) mPriv->localHoldStateReason);
        }
    }
}

CallContentPtr CallChannel::addContent(const QDBusObjectPath &contentPath)
{
    CallContentPtr content = CallContentPtr(
            new CallContent(CallChannelPtr(this), contentPath));
    mPriv->incompleteContents.append(content);
    connect(content->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContentReady(Tp::PendingOperation*)));
    return content;
}

CallContentPtr CallChannel::lookupContent(const QDBusObjectPath &contentPath) const
{
    foreach (const CallContentPtr &content, mPriv->contents) {
        if (content->objectPath() == contentPath.path()) {
            return content;
        }
    }
    foreach (const CallContentPtr &content, mPriv->incompleteContents) {
        if (content->objectPath() == contentPath.path()) {
            return content;
        }
    }
    return CallContentPtr();
}

/**
 * \fn void CallChannel::contentAdded(const Tpy::CallContentPtr &content);
 *
 * This signal is emitted when a media content is added to this channel.
 *
 * \param content The media content that was added.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::contentRemoved(const Tpy::CallContentPtr &content);
 *
 * This signal is emitted when a media content is removed from this channel.
 *
 * \param content The media content that was removed.
 * \sa contents(), contentsForType()
 */

/**
 * \fn void CallChannel::stateChanged(Tpy::CallState &state);
 *
 * This signal is emitted when the value of state(), flags(), stateReason() or stateDetails()
 * changes.
 *
 * \param state The new state.
 */

/**
 * \fn void CallChannel::localHoldStateChanged(Tp::LocalHoldState state, Tp::LocalHoldStateReason reason);
 *
 * This signal is emitted when the local hold state of this channel changes.
 *
 * \param state The new local hold state of this channel.
 * \param reason The reason why the change occurred.
 * \sa localHoldState(), localHoldStateReason()
 */

} // Tp
