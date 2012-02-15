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

#include <TelepathyQt/CallStream>

#include "TelepathyQt/_gen/call-stream.moc.hpp"

#include "TelepathyQt/_gen/cli-call-stream-body.hpp"
#include "TelepathyQt/_gen/cli-call-stream.moc.hpp"

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/DBus>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/ReadinessHelper>

namespace Tp
{

struct TP_QT_NO_EXPORT CallStream::Private
{
    Private(CallStream *parent, const CallContentPtr &content);

    static void introspectMainProperties(Private *self);

    void processRemoteMembersChanged();

    struct RemoteMembersChangedInfo;

    // Public object
    CallStream *parent;

    WeakPtr<CallContent> content;

    // Mandatory proxies
    Client::CallStreamInterface *streamInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint localSendingState;
    ContactSendingStateMap remoteMembers;
    QHash<uint, ContactPtr> remoteMembersContacts;
    bool buildingRemoteMembers;
    QQueue<RemoteMembersChangedInfo *> remoteMembersChangedQueue;
    RemoteMembersChangedInfo *currentRemoteMembersChangedInfo;
};

struct TP_QT_NO_EXPORT CallStream::Private::RemoteMembersChangedInfo
{
    RemoteMembersChangedInfo(const ContactSendingStateMap &updates, const UIntList &removed)
        : updates(updates),
          removed(removed)
    {
    }

    ContactSendingStateMap updates;
    UIntList removed;
};

CallStream::Private::Private(CallStream *parent, const CallContentPtr &content)
    : parent(parent),
      content(content.data()),
      streamInterface(parent->interface<Client::CallStreamInterface>()),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      localSendingState(SendingStateNone),
      buildingRemoteMembers(false),
      currentRemoteMembersChangedInfo(0)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                        // makesSenseForStatuses
        Features(),                                                           // dependsOnFeatures
        QStringList(),                                                            // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMainProperties,
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
            SIGNAL(RemoteMembersChanged(Tp::ContactSendingStateMap,Tp::UIntList)),
            SLOT(onRemoteMembersChanged(Tp::ContactSendingStateMap,Tp::UIntList)));

    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(
                self->properties->GetAll(TP_QT_IFACE_CALL_STREAM),
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

        ContactManagerPtr contactManager =
            parent->content()->channel()->connection()->contactManager();
        PendingContacts *contacts = contactManager->contactsForHandles(
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
 * \headerfile TelepathyQt/call-stream.h <TelepathyQt/CallStream>
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
const Feature CallStream::FeatureCore = Feature(QLatin1String(CallStream::staticMetaObject.className()), 0);

/**
 * Construct a new CallStream object.
 *
 * \param content The content owning this media stream.
 * \param objectPath The object path of this media stream.
 */
CallStream::CallStream(const CallContentPtr &content, const QDBusObjectPath &objectPath)
    : StatefulDBusProxy(content->dbusConnection(), content->busName(),
            objectPath.path(), FeatureCore),
      OptionalInterfaceFactory<CallStream>(this),
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
Contacts CallStream::members() const
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
SendingState CallStream::remoteSendingState(const ContactPtr &contact) const
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
PendingOperation *CallStream::requestSending(bool send)
{
    return new PendingVoid(mPriv->streamInterface->SetSending(send), CallStreamPtr(this));
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
PendingOperation *CallStream::requestReceiving(const ContactPtr &contact, bool receive)
{
    if (!contact) {
        return new PendingFailure(TP_QT_ERROR_INVALID_ARGUMENT,
                QLatin1String("Invalid contact"), CallStreamPtr(this));
    }

    return new PendingVoid(mPriv->streamInterface->RequestReceiving(contact->handle()[0], receive),
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

void CallStream::gotRemoteMembersContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);

    mPriv->buildingRemoteMembers = false;

    if (!pc->isValid()) {
        qWarning().nospace() << "Getting contacts failed with " <<
            pc->errorName() << ":" << pc->errorMessage() << ", ignoring";
        mPriv->processRemoteMembersChanged();
        return;
    }

    QMap<uint, ContactPtr> removed;

    for (ContactSendingStateMap::const_iterator i =
                mPriv->currentRemoteMembersChangedInfo->updates.constBegin();
            i != mPriv->currentRemoteMembersChangedInfo->updates.constEnd(); ++i) {
        mPriv->remoteMembers.insert(i.key(), i.value());
    }

    foreach (const ContactPtr &contact, pc->contacts()) {
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
        QHash<ContactPtr, SendingState> remoteSendingStates;
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
 * \fn void CallStream::localSendingStateChanged(Tp::SendingState localSendingState);
 *
 * This signal is emitted when the local sending state of this media stream
 * changes.
 *
 * \param localSendingState The new local sending state of this media stream.
 * \sa localSendingState()
 */

/**
 * \fn void CallStream::remoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::SendingState> &remoteSendingStates);
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

} // Tp
