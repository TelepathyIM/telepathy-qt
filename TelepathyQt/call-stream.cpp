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

#include <TelepathyQt/debug-internal.h>

#include <TelepathyQt/CallChannel>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/DBus>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingVoid>
#include <TelepathyQt/PendingVariantMap>
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

    ReadinessHelper *readinessHelper;

    // Introspection
    uint localSendingState;
    ContactSendingStateMap remoteMembers;
    QHash<uint, ContactPtr> remoteMembersContacts;
    bool canRequestReceiving;
    QQueue< QSharedPointer<RemoteMembersChangedInfo> > remoteMembersChangedQueue;
    QSharedPointer<RemoteMembersChangedInfo> currentRemoteMembersChangedInfo;
};

struct TP_QT_NO_EXPORT CallStream::Private::RemoteMembersChangedInfo
{
    RemoteMembersChangedInfo(const ContactSendingStateMap &updates,
            const HandleIdentifierMap &identifiers,
            const UIntList &removed,
            const CallStateReason &reason)
        : updates(updates),
          identifiers(identifiers),
          removed(removed),
          reason(reason)
    {
    }

    static QSharedPointer<RemoteMembersChangedInfo> create(
            const ContactSendingStateMap &updates,
            const HandleIdentifierMap &identifiers,
            const UIntList &removed,
            const CallStateReason &reason)
    {
        RemoteMembersChangedInfo *info = new RemoteMembersChangedInfo(
                updates, identifiers, removed, reason);
        return QSharedPointer<RemoteMembersChangedInfo>(info);
    }

    ContactSendingStateMap updates;
    HandleIdentifierMap identifiers;
    UIntList removed;
    CallStateReason reason;
};

CallStream::Private::Private(CallStream *parent, const CallContentPtr &content)
    : parent(parent),
      content(content.data()),
      streamInterface(parent->interface<Client::CallStreamInterface>()),
      readinessHelper(parent->readinessHelper()),
      localSendingState(SendingStateNone),
      canRequestReceiving(true)
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
            SIGNAL(LocalSendingStateChanged(uint,Tp::CallStateReason)),
            SLOT(onLocalSendingStateChanged(uint,Tp::CallStateReason)));
    parent->connect(self->streamInterface,
            SIGNAL(RemoteMembersChanged(Tp::ContactSendingStateMap,Tp::HandleIdentifierMap,Tp::UIntList,Tp::CallStateReason)),
            SLOT(onRemoteMembersChanged(Tp::ContactSendingStateMap,Tp::HandleIdentifierMap,Tp::UIntList,Tp::CallStateReason)));

    parent->connect(self->streamInterface->requestAllProperties(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotMainProperties(Tp::PendingOperation*)));
}

void CallStream::Private::processRemoteMembersChanged()
{
    if (currentRemoteMembersChangedInfo) { // currently building contacts
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

    foreach(uint i, currentRemoteMembersChangedInfo->removed) {
        pendingRemoteMembers.insert(i);
    }

    if (!pendingRemoteMembers.isEmpty()) {
        ConnectionPtr connection = parent->content()->channel()->connection();
        connection->lowlevel()->injectContactIds(currentRemoteMembersChangedInfo->identifiers);

        ContactManagerPtr contactManager = connection->contactManager();
        PendingContacts *contacts = contactManager->contactsForHandles(
                pendingRemoteMembers.toList());
        parent->connect(contacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotRemoteMembersContacts(Tp::PendingOperation*)));
    } else {
        currentRemoteMembersChangedInfo.clear();
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
 * \param content The content owning this call stream.
 * \param objectPath The object path of this call stream.
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
 * Return the content owning this call stream.
 *
 * \return The content owning this call stream.
 */
CallContentPtr CallStream::content() const
{
    return CallContentPtr(mPriv->content);
}

/**
 * Returns whether the user can request that a remote contact starts
 * sending on this stream. Not all protocols allow the user to ask
 * the other side to start sending media.
 *
 * \return true if the user can request that a remote contact starts
 * sending on this stream, or false otherwise.
 * \sa requestReceiving()
 */
bool CallStream::canRequestReceiving() const
{
    return mPriv->canRequestReceiving;
}

/**
 * Return the contacts whose the call stream is with.
 *
 * \return The contacts whose the call stream is with.
 * \sa remoteMembersRemoved()
 */
Contacts CallStream::remoteMembers() const
{
    return mPriv->remoteMembersContacts.values().toSet();
}

/**
 * Return the call stream local sending state.
 *
 * \return The call stream local sending state.
 * \sa localSendingStateChanged()
 */
SendingState CallStream::localSendingState() const
{
    return (SendingState) mPriv->localSendingState;
}

/**
 * Return the call stream remote sending state for a given \a contact.
 *
 * \return The call stream remote sending state for a contact.
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
 * Request that media starts or stops being sent on this call stream.
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
 * Request that a remote \a contact stops or starts sending on this call stream.
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
    } else if (!mPriv->canRequestReceiving && receive) {
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("Requesting the other side to start sending media "
                              "is not allowed by this protocol"),
                CallStreamPtr(this));
    }

    return new PendingVoid(mPriv->streamInterface->RequestReceiving(contact->handle()[0], receive),
            CallStreamPtr(this));
}

void CallStream::gotMainProperties(PendingOperation *op)
{
    if (op->isError()) {
        warning().nospace() << "CallStreamInterface::requestAllProperties() failed with " <<
            op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
            op->errorName(), op->errorMessage());
        return;
    }

    debug() << "Got reply to CallStreamInterface::requestAllProperties()";

    PendingVariantMap *pvm = qobject_cast<PendingVariantMap*>(op);
    Q_ASSERT(pvm);

    QVariantMap props = pvm->result();

    mPriv->canRequestReceiving = qdbus_cast<bool>(props[QLatin1String("CanRequestReceiving")]);
    mPriv->localSendingState = qdbus_cast<uint>(props[QLatin1String("LocalSendingState")]);

    ContactSendingStateMap remoteMembers =
        qdbus_cast<ContactSendingStateMap>(props[QLatin1String("RemoteMembers")]);
    HandleIdentifierMap remoteMemberIdentifiers =
        qdbus_cast<HandleIdentifierMap>(props[QLatin1String("RemoteMemberIdentifiers")]);

    mPriv->remoteMembersChangedQueue.enqueue(Private::RemoteMembersChangedInfo::create(
                remoteMembers, remoteMemberIdentifiers, UIntList(), CallStateReason()));
    mPriv->processRemoteMembersChanged();
}

void CallStream::gotRemoteMembersContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);

    if (!pc->isValid()) {
        warning().nospace() << "Getting contacts failed with " <<
            pc->errorName() << ":" << pc->errorMessage() << ", ignoring";
        mPriv->currentRemoteMembersChangedInfo.clear();
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
            emit remoteSendingStateChanged(remoteSendingStates,
                    mPriv->currentRemoteMembersChangedInfo->reason);
        }

        if (!removed.isEmpty()) {
            emit remoteMembersRemoved(removed.values().toSet(),
                    mPriv->currentRemoteMembersChangedInfo->reason);
        }
    }

    mPriv->currentRemoteMembersChangedInfo.clear();
    mPriv->processRemoteMembersChanged();
}

void CallStream::onLocalSendingStateChanged(uint state, const CallStateReason &reason)
{
    mPriv->localSendingState = state;
    emit localSendingStateChanged((SendingState) state, reason);
}

void CallStream::onRemoteMembersChanged(const ContactSendingStateMap &updates,
        const HandleIdentifierMap &identifiers,
        const UIntList &removed,
        const CallStateReason &reason)
{
    if (updates.isEmpty() && removed.isEmpty()) {
        debug() << "Received Call::Stream::RemoteMembersChanged with 0 removals and "
            "updates, skipping it";
        return;
    }

    debug() << "Received Call::Stream::RemoteMembersChanged with" << updates.size() <<
        "updated and" << removed.size() << "removed";
    mPriv->remoteMembersChangedQueue.enqueue(
            Private::RemoteMembersChangedInfo::create(updates, identifiers, removed, reason));
    mPriv->processRemoteMembersChanged();
}

/**
 * \fn void CallStream::localSendingStateChanged(Tp::SendingState localSendingState, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when the local sending state of this call stream
 * changes.
 *
 * \param localSendingState The new local sending state of this call stream.
 * \param reason The reason that caused this change
 * \sa localSendingState()
 */

/**
 * \fn void CallStream::remoteSendingStateChanged(const QHash<Tp::ContactPtr, Tp::SendingState> &remoteSendingStates, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when any remote sending state of this call stream
 * changes.
 *
 * \param remoteSendingStates The new remote sending states of this call stream.
 * \param reason The reason that caused these changes
 * \sa remoteSendingState()
 */

/**
 * \fn void CallStream::remoteMembersRemoved(const Tp::Contacts &members, const Tp::CallStateReason &reason);
 *
 * This signal is emitted when one or more members of this stream are removed.
 *
 * \param members The members that were removed from this call stream.
 * \param reason The reason for that caused these removals
 * \sa remoteMembers()
 */

} // Tp
