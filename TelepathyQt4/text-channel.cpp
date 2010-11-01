/* Text channel client-side proxy
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

#include <TelepathyQt4/TextChannel>

#include <QDateTime>

#include "TelepathyQt4/_gen/text-channel.moc.hpp"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/ReceivedMessage>
#include <TelepathyQt4/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT PendingSendMessage::Private
{
    inline Private(const Message &message);
    QString token;
    Message message;
};

inline PendingSendMessage::Private::Private(const Message &message)
    : token(QLatin1String("")), message(message)
{
}

PendingSendMessage::PendingSendMessage(const Message &message, QObject *parent)
    : PendingOperation(parent), mPriv(new Private(message))
{
}

PendingSendMessage::~PendingSendMessage()
{
    delete mPriv;
}

QString PendingSendMessage::sentMessageToken() const
{
    return mPriv->token;
}

Message PendingSendMessage::message() const
{
    return mPriv->message;
}

void PendingSendMessage::onTextSent(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        setFinishedWithError(reply.error());
    } else {
        setFinished();
    }
    watcher->deleteLater();
}

void PendingSendMessage::onMessageSent(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        setFinishedWithError(reply.error());
    } else {
        mPriv->token = reply.value();
        setFinished();
    }
    watcher->deleteLater();
}

struct TELEPATHY_QT4_NO_EXPORT TextChannel::Private
{
    Private(TextChannel *parent);
    ~Private();

    static void introspectMessageQueue(Private *self);
    static void introspectMessageCapabilities(Private *self);
    static void introspectMessageSentSignal(Private *self);
    static void enableChatStateNotifications(Private *self);

    void updateInitialMessages();
    void updateCapabilities();

    void processMessageQueue();
    void processChatStateQueue();

    void contactLost(uint handle);
    void contactFound(ContactPtr contact);

    // Public object
    TextChannel *parent;

    ReadinessHelper *readinessHelper;

    // FeatureMessageCapabilities and FeatureMessageQueue
    QVariantMap props;
    bool getAllInFlight;
    bool gotProperties;

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes;
    MessagePartSupportFlags messagePartSupport;
    DeliveryReportingSupportFlags deliveryReportingSupport;

    // FeatureMessageQueue
    bool initialMessagesReceived;
    struct MessageEvent
    {
        MessageEvent(const ReceivedMessage &message)
            : isMessage(true), message(message),
                removed(0)
        { }
        MessageEvent(uint removed)
            : isMessage(false), message(), removed(removed)
        { }

        bool isMessage;
        ReceivedMessage message;
        uint removed;
    };
    QList<ReceivedMessage> messages;
    QList<MessageEvent *> incompleteMessages;
    QHash<QDBusPendingCallWatcher *, UIntList> acknowledgeBatches;

    // FeatureChatState
    struct ChatStateEvent
    {
        ChatStateEvent(uint contactHandle, uint state)
            : contactHandle(contactHandle), state(state)
        { }

        ContactPtr contact;
        uint contactHandle;
        uint state;
    };
    QList<ChatStateEvent *> chatStateQueue;
    QHash<ContactPtr, ChannelChatState> chatStates;

    QSet<uint> awaitingContacts;
};

TextChannel::Private::Private(TextChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      getAllInFlight(false),
      gotProperties(false),
      messagePartSupport(0),
      deliveryReportingSupport(0),
      initialMessagesReceived(false)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableMessageQueue(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMessageQueue,
        this);
    introspectables[FeatureMessageQueue] = introspectableMessageQueue;

    ReadinessHelper::Introspectable introspectableMessageCapabilities(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMessageCapabilities,
        this);
    introspectables[FeatureMessageCapabilities] = introspectableMessageCapabilities;

    ReadinessHelper::Introspectable introspectableMessageSentSignal(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMessageSentSignal,
        this);
    introspectables[FeatureMessageSentSignal] = introspectableMessageSentSignal;

    ReadinessHelper::Introspectable introspectableChatState(
        QSet<uint>() << 0,                                                                  // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                                 // dependsOnFeatures (core)
        QStringList() << QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CHAT_STATE),   // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::enableChatStateNotifications,
        this);
    introspectables[FeatureChatState] = introspectableChatState;

    readinessHelper->addIntrospectables(introspectables);
}

TextChannel::Private::~Private()
{
    foreach (MessageEvent *e, incompleteMessages) {
        delete e;
    }

    foreach (ChatStateEvent *e, chatStateQueue) {
        delete e;
    }
}

void TextChannel::Private::introspectMessageQueue(
        TextChannel::Private *self)
{
    TextChannel *parent = self->parent;

    if (parent->hasMessagesInterface()) {
        // FeatureMessageQueue needs signal connections + Get (but we
        // might as well do GetAll and reduce the number of code paths)
        parent->connect(parent->messagesInterface(),
                SIGNAL(MessageReceived(Tp::MessagePartList)),
                SLOT(onMessageReceived(Tp::MessagePartList)));
        parent->connect(parent->messagesInterface(),
                SIGNAL(PendingMessagesRemoved(Tp::UIntList)),
                SLOT(onPendingMessagesRemoved(Tp::UIntList)));

        if (!self->gotProperties && !self->getAllInFlight) {
            self->getAllInFlight = true;
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    parent->propertiesInterface()->GetAll(
                        QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES)),
                        parent);
            parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotProperties(QDBusPendingCallWatcher*)));
        } else if (self->gotProperties) {
            self->updateInitialMessages();
        }
    } else {
        // FeatureMessageQueue needs signal connections + ListPendingMessages
        parent->connect(parent->textInterface(),
                SIGNAL(Received(uint,uint,uint,uint,uint,QString)),
                SLOT(onTextReceived(uint,uint,uint,uint,uint,const QString)));

        // we present SendError signals as if they were incoming
        // messages, to be consistent with Messages
        parent->connect(parent->textInterface(),
                SIGNAL(SendError(uint,uint,uint,QString)),
                SLOT(onTextSendError(uint,uint,uint,QString)));

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                parent->textInterface()->ListPendingMessages(false), parent);
        parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotPendingMessages(QDBusPendingCallWatcher*)));
    }
}

void TextChannel::Private::introspectMessageCapabilities(
        TextChannel::Private *self)
{
    TextChannel *parent = self->parent;

    if (parent->hasMessagesInterface()) {
        if (!self->gotProperties && !self->getAllInFlight) {
            self->getAllInFlight = true;
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    parent->propertiesInterface()->GetAll(
                        QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES)),
                        parent);
            parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotProperties(QDBusPendingCallWatcher*)));
        } else if (self->gotProperties) {
            self->updateCapabilities();
        }
    } else {
        self->supportedContentTypes =
            (QStringList(QLatin1String("text/plain")));
        parent->readinessHelper()->setIntrospectCompleted(
                FeatureMessageCapabilities, true);
    }
}

void TextChannel::Private::introspectMessageSentSignal(
        TextChannel::Private *self)
{
    TextChannel *parent = self->parent;

    if (parent->hasMessagesInterface()) {
        parent->connect(parent->messagesInterface(),
                SIGNAL(MessageSent(Tp::MessagePartList,uint,QString)),
                SLOT(onMessageSent(Tp::MessagePartList,uint,QString)));
    } else {
        parent->connect(parent->textInterface(),
                SIGNAL(Sent(uint,uint,QString)),
                SLOT(onTextSent(uint,uint,QString)));
    }

    self->readinessHelper->setIntrospectCompleted(FeatureMessageSentSignal, true);
}

void TextChannel::Private::enableChatStateNotifications(
        TextChannel::Private *self)
{
    TextChannel *parent = self->parent;
    Client::ChannelInterfaceChatStateInterface *chatStateInterface =
        parent->chatStateInterface();

    parent->connect(chatStateInterface,
            SIGNAL(ChatStateChanged(uint,uint)),
            SLOT(onChatStateChanged(uint,uint)));

    // FIXME fd.o#24882 - Download contacts' initial chat states

    self->readinessHelper->setIntrospectCompleted(FeatureChatState, true);
}

void TextChannel::Private::updateInitialMessages()
{
    if (!readinessHelper->requestedFeatures().contains(FeatureMessageQueue) ||
        readinessHelper->isReady(Features() << FeatureMessageQueue)) {
        return;
    }

    Q_ASSERT(!initialMessagesReceived);
    initialMessagesReceived = true;

    MessagePartListList messages = qdbus_cast<MessagePartListList>(
            props[QLatin1String("PendingMessages")]);
    if (messages.isEmpty()) {
        debug() << "Message queue empty: FeatureMessageQueue is now ready";
        readinessHelper->setIntrospectCompleted(FeatureMessageQueue, true);
    } else {
        foreach (const MessagePartList &message, messages) {
            parent->onMessageReceived(message);
        }
    }
}

void TextChannel::Private::updateCapabilities()
{
    if (!readinessHelper->requestedFeatures().contains(FeatureMessageCapabilities) ||
        readinessHelper->isReady(Features() << FeatureMessageCapabilities)) {
        return;
    }

    supportedContentTypes = qdbus_cast<QStringList>(
            props[QLatin1String("SupportedContentTypes")]);
    if (supportedContentTypes.isEmpty()) {
        supportedContentTypes << QLatin1String("text/plain");
    }
    messagePartSupport = MessagePartSupportFlags(qdbus_cast<uint>(
            props[QLatin1String("MessagePartSupportFlags")]));
    deliveryReportingSupport = DeliveryReportingSupportFlags(
            qdbus_cast<uint>(props[QLatin1String("DeliveryReportingSupport")]));
    readinessHelper->setIntrospectCompleted(FeatureMessageCapabilities, true);
}

void TextChannel::Private::processMessageQueue()
{
    // Proceed as far as we can with the processing of incoming messages
    // and message-removal events; message IDs aren't necessarily globally
    // unique, so we need to process them in the correct order relative
    // to incoming messages
    while (!incompleteMessages.isEmpty()) {
        const MessageEvent *e = incompleteMessages.first();
        debug() << "MessageEvent:" << e;

        if (e->isMessage) {
            if (e->message.senderHandle() != 0 &&
                    !e->message.sender()) {
                // the message doesn't have a sender Contact, but needs one.
                // We'll have to stop processing here, and come back to it
                // when we have more Contact objects
                break;
            }

            // if we reach here, the message is ready
            debug() << "Message is usable, copying to main queue";
            messages << e->message;
            emit parent->messageReceived(e->message);
        } else {
            // forget about the message(s) with ID e->removed (there should be
            // at most one under normal circumstances)
            int i = 0;
            while (i < messages.size()) {
                if (messages.at(i).pendingId() == e->removed) {
                    emit parent->pendingMessageRemoved(messages.at(i));
                    messages.removeAt(i);
                } else {
                    i++;
                }
            }
        }

        debug() << "Dropping first event";
        delete incompleteMessages.takeFirst();
    }

    if (incompleteMessages.isEmpty()) {
        if (readinessHelper->requestedFeatures().contains(FeatureMessageQueue) &&
            !readinessHelper->isReady(Features() << FeatureMessageQueue)) {
            debug() << "incompleteMessages empty for the first time: "
                "FeatureMessageQueue is now ready";
            readinessHelper->setIntrospectCompleted(FeatureMessageQueue, true);
        }
        return;
    }

    // What Contact objects do we need in order to proceed, ignoring those
    // for which we've already sent a request?
    QSet<uint> contactsRequired;
    foreach (const MessageEvent *e, incompleteMessages) {
        if (e->isMessage) {
            uint handle = e->message.senderHandle();
            if (handle != 0 && !e->message.sender()
                    && !awaitingContacts.contains(handle)) {
                contactsRequired << handle;
            }
        }
    }

    if (contactsRequired.isEmpty()) {
        return;
    }

    parent->connect(parent->connection()->contactManager()->contactsForHandles(
                contactsRequired.toList()),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactsFinished(Tp::PendingOperation*)));

    awaitingContacts |= contactsRequired;
}

void TextChannel::Private::processChatStateQueue()
{
    while (!chatStateQueue.isEmpty()) {
        const ChatStateEvent *e = chatStateQueue.first();
        debug() << "ChatStateEvent:" << e;

        if (e->contact.isNull()) {
            // the chat state Contact object wasn't retrieved yet, but needs
            // one. We'll have to stop processing here, and come back to it
            // when we have more Contact objects
            break;
        }

        chatStates.insert(e->contact, (ChannelChatState) e->state);

        // if we reach here, the Contact object is ready
        emit parent->chatStateChanged(e->contact, (ChannelChatState) e->state);

        debug() << "Dropping first event";
        delete chatStateQueue.takeFirst();
    }

    // What Contact objects do we need in order to proceed, ignoring those
    // for which we've already sent a request?
    QSet<uint> contactsRequired;
    foreach (const ChatStateEvent *e, chatStateQueue) {
        if (!e->contact &&
            !awaitingContacts.contains(e->contactHandle)) {
            contactsRequired << e->contactHandle;
        }
    }

    if (contactsRequired.isEmpty()) {
        return;
    }

    parent->connect(parent->connection()->contactManager()->contactsForHandles(
                contactsRequired.toList()),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactsFinished(Tp::PendingOperation*)));

    awaitingContacts |= contactsRequired;
}

void TextChannel::Private::contactLost(uint handle)
{
    // we're not going to get a Contact object for this handle, so mark the
    // messages from that handle as "unknown sender"
    foreach (MessageEvent *e, incompleteMessages) {
        if (e->isMessage && e->message.senderHandle() == handle
                && !e->message.sender()) {
            e->message.clearSenderHandle();
        }
    }

    // there is no point in sending chat state notifications for unknown
    // contacts, removing chat state events from queue that refer to this handle
    foreach (ChatStateEvent *e, chatStateQueue) {
        if (e->contactHandle == handle) {
            chatStateQueue.removeOne(e);
            delete e;
        }
    }
}

void TextChannel::Private::contactFound(ContactPtr contact)
{
    uint handle = contact->handle().at(0);

    foreach (MessageEvent *e, incompleteMessages) {
        if (e->isMessage && e->message.senderHandle() == handle
                && !e->message.sender()) {
            e->message.setSender(contact);
        }
    }

    foreach (ChatStateEvent *e, chatStateQueue) {
        if (e->contactHandle == handle) {
            e->contact = contact;
        }
    }
}

/**
 * \class TextChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/text-channel.h <TelepathyQt4/TextChannel>
 *
 * \brief The TextChannel class provides an object representing a
 * Telepathy channel of type Text.
 */

/**
 * Feature used in order to access the message queue info.
 *
 * Se message queue methods' documentation for more details.
 */
const Feature TextChannel::FeatureMessageQueue = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to access message capabilities info.
 *
 * Se message capabilities methods' documentation for more details.
 */
const Feature TextChannel::FeatureMessageCapabilities = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 1);

/**
 * Feature used in order to receive notification when a message is sent.
 */
const Feature TextChannel::FeatureMessageSentSignal = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 2);

/**
 * Feature used in order to keep track of chat state changes.
 *
 * The chatStateChanged() signal will be emitted when a remote contact chat
 * state changes.
 */
const Feature TextChannel::FeatureChatState = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 3);

/**
 * \fn void TextChannel::messageSent(const Tp::Message &message,
 *     Tp::MessageSendingFlags flags,
 *     const QString &sentMessageToken)
 *
 * Emitted when a message is sent, if the FeatureMessageSentSignal Feature
 * has been enabled.
 *
 * This signal is emitted regardless of whether the message is sent by this
 * client, or another client using the same Channel via D-Bus.
 *
 * \param message A message. This may differ slightly from what the client
 *                requested to send, for instance if it has been altered due
 *                to limitations of the instant messaging protocol used.
 * \param flags MessageSendingFlags that were in effect when the message was
 *              sent. Clients can use these in conjunction with
 *              deliveryReportingSupport to determine whether delivery
 *              reporting can be expected.
 * \param sentMessageToken Either an empty QString, or an opaque token used
 *                         to match the message to any delivery reports.
 */

/**
 * \fn void TextChannel::messageReceived(const Tp::ReceivedMessage &message)
 *
 * Emitted when a message is added to messageQueue(), if the
 * FeatureMessageQueue Feature has been enabled.
 *
 * This occurs slightly later than the message being received over D-Bus;
 * see messageQueue() for details.
 */

/**
 * \fn void TextChannel::pendingMessageRemoved(
 *      const Tp::ReceivedMessage &message)
 *
 * Emitted when a message is removed from messageQueue(), if the
 * FeatureMessageQueue Feature has been enabled. See messageQueue() for the
 * circumstances in which this happens.
 */

/**
 * \fn void TextChannel::chatStateChanged(const Tp::ContactPtr &contact,
 *      ChannelChatState state)
 *
 * Emitted when the state of a member of the channel has changed, if the
 * FeatureChatState feature has been enabled.
 * This includes local state.
 */

/**
 * Create a new IncomingFileTransfer channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A TextChannelPtr object pointing to the newly created
 *         TextChannel object.
 */
TextChannelPtr TextChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return TextChannelPtr(new TextChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new TextChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 */
TextChannel::TextChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : Channel(connection, objectPath, immutableProperties),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
TextChannel::~TextChannel()
{
    delete mPriv;
}

/**
 * Return whether this channel supports the Telepathy Messages interface.
 * if it does not, requestChatState() will fail and all contacts' chat states
 * will appear to be %Tp::ChannelChatStateInactive.
 *
 * The result of calling this method is undefined until basic Channel
 * functionality has been enabled by calling becomeReady and waiting for the
 * pending operation to complete.
 *
 * \return true if the Messages interface is supported
 */
bool TextChannel::hasMessagesInterface() const
{
    return interfaces().contains(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES));
}

/**
 * Return whether this channel supports the Telepathy ChatState interface.
 * If it does not, some advanced functionality will be unavailable.
 *
 * The result of calling this method is undefined until basic Channel
 * functionality has been enabled by calling becomeReady and waiting for the
 * pending operation to complete.
 *
 * \return true if the ChatState interface is supported
 */
bool TextChannel::hasChatStateInterface() const
{
    return interfaces().contains(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CHAT_STATE));
}

/**
 * Return whether contacts can be invited into this channel using
 * inviteContacts (which is equivalent to groupAddContacts). Whether this
 * is the case depends on the underlying protocol, the type of channel,
 * and the user's privileges (in some chatrooms, only a privileged user
 * can invite other contacts).
 *
 * This is an alias for groupCanAddContacts, to indicate its meaning more
 * clearly for Text channels.
 *
 * The result of calling this method is undefined until basic Group
 * functionality has been enabled by calling becomeReady and waiting for the
 * pending operation to complete.
 *
 * \return The same thing as groupCanAddContacts
 */
bool TextChannel::canInviteContacts() const
{
    return groupCanAddContacts();
}

/* <!--x--> in the block below is used to escape the star-slash sequence */
/**
 * Return a list of supported MIME content types for messages on this channel.
 * For a simple text channel this will be a list containing one item,
 * "text/plain".
 *
 * This list may contain the special value "*<!--x-->/<!--x-->*", which
 * indicates that any content type is supported.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete
 *
 * \return A list of MIME content types
 */
QStringList TextChannel::supportedContentTypes() const
{
    return mPriv->supportedContentTypes;
}

/**
 * Return a set of flags indicating support for multi-part messages on this
 * channel. This is zero on simple text channels, or greater than zero if
 * there is partial or full support for multi-part messages.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete.
 *
 * \return A set of MessagePartSupportFlags
 */
MessagePartSupportFlags TextChannel::messagePartSupport() const
{
    return mPriv->messagePartSupport;
}

/**
 * Return a set of flags indicating support for delivery reporting on this
 * channel. This is zero if there are no particular guarantees, or greater
 * than zero if delivery reports can be expected under certain circumstances.
 *
 * The result of calling this method is undefined until the
 * FeatureMessageCapabilities Feature has been enabled, by calling becomeReady
 * and waiting for the pending operation to complete.
 *
 * \return A set of DeliveryReportingSupportFlags
 */
DeliveryReportingSupportFlags TextChannel::deliveryReportingSupport() const
{
    return mPriv->deliveryReportingSupport;
}

/**
 * Return a list of messages received in this channel. This list is empty
 * unless the FeatureMessageQueue Feature has been enabled.
 *
 * Messages are added to this list when they are received from the instant
 * messaging service; the messageReceived signal is emitted.
 *
 * There is a small delay between the message being received over D-Bus and
 * becoming available to users of this C++ API, since a small amount of
 * additional information needs to be fetched. However, the relative ordering
 * of all the messages in a channel is preserved.
 *
 * Messages are removed from this list when they are acknowledged with the
 * acknowledge() or forget() methods. On channels where hasMessagesInterface()
 * returns true, they will also be removed when acknowledged by a different
 * client. In either case, the pendingMessageRemoved signal is emitted.
 *
 * \return The unacknowledged messages in this channel, excluding any that
 *         have been forgotten with forget().
 */
QList<ReceivedMessage> TextChannel::messageQueue() const
{
    return mPriv->messages;
}

/**
 * Return the current chat state for \a contact.
 *
 * If hasChatStateInterface() returns false, this method will always return
 * ChannelChatStateInactive.
 *
 * This method requires TextChannel::FeatureChatState to be enabled.
 *
 * \return A contact current chat state.
 */
ChannelChatState TextChannel::chatState(const ContactPtr &contact) const
{
    if (!isReady(FeatureChatState)) {
        warning() << "TextChannel::chatState() used with "
            "FeatureChatState not ready";
        return ChannelChatStateInactive;
    }

    if (mPriv->chatStates.contains(contact)) {
        return mPriv->chatStates.value(contact);
    }
    return ChannelChatStateInactive;
}

void TextChannel::onAcknowledgePendingMessagesReply(
        QDBusPendingCallWatcher *watcher)
{
    UIntList ids = mPriv->acknowledgeBatches.value(watcher);
    QDBusPendingReply<> reply = *watcher;

    if (reply.isError()) {
        // One of the IDs was bad, and we can't know which one. Recover by
        // doing as much as possible, and hope for the best...
        debug() << "Recovering from AcknowledgePendingMessages failure for: "
            << ids;
        foreach (uint id, ids) {
            textInterface()->AcknowledgePendingMessages(UIntList() << id);
        }
    }

    mPriv->acknowledgeBatches.remove(watcher);
    watcher->deleteLater();
}

/**
 * Acknowledge that received messages have been displayed to the user.
 *
 * This method should only be called by the main handler of a Channel, usually
 * meaning the user interface process that displays the Channel to the user
 * (when a ChannelDispatcher is used, the Handler must acknowledge messages,
 * and other Approvers or Observers must not acknowledge messages).
 *
 * Processes other than the main handler of a Channel can free memory used
 * in Telepathy-Qt4 by calling forget() instead.
 *
 * The messages must have come from this channel, therefore this method does
 * not make sense if FeatureMessageQueue has not been enabled.
 *
 * \param messages A list of received messages that have now been displayed.
 */
void TextChannel::acknowledge(const QList<ReceivedMessage> &messages)
{
    UIntList ids;

    foreach (const ReceivedMessage &m, messages) {
        if (m.isFromChannel(TextChannelPtr(this))) {
            ids << m.pendingId();
        } else {
            warning() << "message did not come from this channel, ignoring";
        }
    }

    if (ids.isEmpty()) {
        return;
    }

    // we're going to acknowledge these messages (or as many as possible, if
    // we lose a race with another acknowledging process), so let's remove
    // them from the list immediately
    forget(messages);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            textInterface()->AcknowledgePendingMessages(ids));
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onAcknowledgePendingMessagesReply(QDBusPendingCallWatcher*)));
    mPriv->acknowledgeBatches[watcher] = ids;
}

/**
 * Remove messages from messageQueue without acknowledging them.
 *
 * This method frees memory inside the Telepathy-Qt4 TextChannel proxy, but
 * does not free the corresponding memory in the Connection Manager process.
 * It should be used by clients that are not the main handler for a Channel;
 * the main handler for a Channel should use acknowledge instead.
 *
 * The messages must have come from this channel, therefore this method does
 * not make sense if FeatureMessageQueue has not been enabled.
 *
 * \param messages A list of received messages that have now been processed.
 */
void TextChannel::forget(const QList<ReceivedMessage> &messages)
{
    foreach (const ReceivedMessage &m, messages) {
        if (!m.isFromChannel(TextChannelPtr(this))) {
            warning() << "message did not come from this channel, ignoring";
        } else if (mPriv->messages.removeOne(m)) {
            emit pendingMessageRemoved(m);
        }
    }
}

PendingSendMessage *TextChannel::send(const QString &text,
        ChannelTextMessageType type, MessageSendingFlags flags)
{
    Message m(type, text);
    PendingSendMessage *op = new PendingSendMessage(m, this);

    if (hasMessagesInterface()) {
        connect(new QDBusPendingCallWatcher(
                    messagesInterface()->SendMessage(m.parts(),
                        (uint) flags)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onMessageSent(QDBusPendingCallWatcher*)));
    } else {
        connect(new QDBusPendingCallWatcher(textInterface()->Send(type, text)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onTextSent(QDBusPendingCallWatcher*)));
    }
    return op;
}

PendingSendMessage *TextChannel::send(const MessagePartList &parts,
        MessageSendingFlags flags)
{
    Message m(parts);
    PendingSendMessage *op = new PendingSendMessage(m, this);

    if (hasMessagesInterface()) {
        connect(new QDBusPendingCallWatcher(
                    messagesInterface()->SendMessage(m.parts(),
                        (uint) flags)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onMessageSent(QDBusPendingCallWatcher*)));
    } else {
        connect(new QDBusPendingCallWatcher(textInterface()->Send(
                        m.messageType(), m.text())),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onTextSent(QDBusPendingCallWatcher*)));
    }
    return op;
}

/**
 * Set the local chat state and notify other members of the channel that it has
 * changed.
 *
 * Note that only the primary handler of the channel should set its chat
 * state.
 *
 * \param state The new state.
 * \sa chatStateChanged()
 */
PendingOperation *TextChannel::requestChatState(ChannelChatState state)
{
    if (!interfaces().contains(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_CHAT_STATE))) {
        warning() << "TextChannel::requestChatState() used with no chat "
            "state interface";
        return new PendingFailure(QLatin1String(TELEPATHY_ERROR_NOT_IMPLEMENTED),
                QLatin1String("TextChannel does not support chat state interface"),
                this);
    }
    return new PendingVoid(chatStateInterface()->SetChatState(
                (uint) state), this);
}

void TextChannel::onMessageSent(const MessagePartList &parts,
        uint flags,
        const QString &sentMessageToken)
{
    emit messageSent(Message(parts), MessageSendingFlag(flags),
            sentMessageToken);
}

void TextChannel::onContactsFinished(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    UIntList failed;

    Q_ASSERT(pc->isForHandles());

    foreach (uint handle, pc->handles()) {
        mPriv->awaitingContacts -= handle;
    }

    if (pc->isError()) {
        warning().nospace() << "Gathering contacts failed: "
            << pc->errorName() << ": " << pc->errorMessage();
        foreach (uint handle, pc->handles()) {
            mPriv->contactLost(handle);
        }
    } else {
        foreach (const ContactPtr &contact, pc->contacts()) {
            mPriv->contactFound(contact);
        }
        foreach (uint handle, pc->invalidHandles()) {
            mPriv->contactLost(handle);
        }
    }

    // all contacts for messages and chat state events we were asking about
    // should now be ready
    mPriv->processMessageQueue();
    mPriv->processChatStateQueue();
}

void TextChannel::onMessageReceived(const MessagePartList &parts)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    mPriv->incompleteMessages << new Private::MessageEvent(
            ReceivedMessage(parts, TextChannelPtr(this)));
    mPriv->processMessageQueue();
}

void TextChannel::onPendingMessagesRemoved(const UIntList &ids)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }
    foreach (uint id, ids) {
        mPriv->incompleteMessages << new Private::MessageEvent(id);
    }
    mPriv->processMessageQueue();
}

void TextChannel::onTextSent(uint timestamp, uint type, const QString &text)
{
    emit messageSent(Message(timestamp, type, text), 0,
            QLatin1String(""));
}

void TextChannel::onTextReceived(uint id, uint timestamp, uint sender,
        uint type, uint flags, const QString &text)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    MessagePart header;

    if (timestamp == 0) {
        timestamp = QDateTime::currentDateTime().toTime_t();
    }
    header.insert(QLatin1String("message-received"),
            QDBusVariant(static_cast<qlonglong>(timestamp)));

    header.insert(QLatin1String("pending-message-id"), QDBusVariant(id));
    header.insert(QLatin1String("message-sender"), QDBusVariant(sender));
    header.insert(QLatin1String("message-type"), QDBusVariant(type));

    if (flags & ChannelTextMessageFlagScrollback) {
        header.insert(QLatin1String("scrollback"), QDBusVariant(true));
    }
    if (flags & ChannelTextMessageFlagRescued) {
        header.insert(QLatin1String("rescued"), QDBusVariant(true));
    }

    MessagePart body;

    body.insert(QLatin1String("content-type"),
            QDBusVariant(QLatin1String("text/plain")));
    body.insert(QLatin1String("content"), QDBusVariant(text));

    if (flags & ChannelTextMessageFlagTruncated) {
        header.insert(QLatin1String("truncated"), QDBusVariant(true));
    }

    MessagePartList parts;
    parts << header;
    parts << body;

    ReceivedMessage m(parts, TextChannelPtr(this));

    if (flags & ChannelTextMessageFlagNonTextContent) {
        // set the "you are not expected to understand this" flag
        m.setForceNonText();
    }

    mPriv->incompleteMessages << new Private::MessageEvent(m);
    mPriv->processMessageQueue();
}

void TextChannel::onTextSendError(uint error, uint timestamp, uint type,
        const QString &text)
{
    if (!mPriv->initialMessagesReceived) {
        return;
    }

    MessagePart header;

    header.insert(QLatin1String("message-received"),
            QDBusVariant(static_cast<qlonglong>(
                    QDateTime::currentDateTime().toTime_t())));
    header.insert(QLatin1String("message-type"),
            QDBusVariant(static_cast<uint>(
                    ChannelTextMessageTypeDeliveryReport)));

    // we can't tell whether it's a temporary or permanent failure here,
    // so guess based on the delivery-error
    uint deliveryStatus;
    switch (error) {
        case ChannelTextSendErrorOffline:
        case ChannelTextSendErrorPermissionDenied:
            deliveryStatus = DeliveryStatusTemporarilyFailed;
            break;

        case ChannelTextSendErrorInvalidContact:
        case ChannelTextSendErrorTooLong:
        case ChannelTextSendErrorNotImplemented:
            deliveryStatus = DeliveryStatusPermanentlyFailed;
            break;

        case ChannelTextSendErrorUnknown:
        default:
            deliveryStatus = DeliveryStatusTemporarilyFailed;
            break;
    }

    header.insert(QLatin1String("delivery-status"),
            QDBusVariant(deliveryStatus));
    header.insert(QLatin1String("delivery-error"), QDBusVariant(error));

    MessagePart echoHeader;
    echoHeader.insert(QLatin1String("message-sent"),
            QDBusVariant(timestamp));
    echoHeader.insert(QLatin1String("message-type"),
            QDBusVariant(type));

    MessagePart echoBody;
    echoBody.insert(QLatin1String("content-type"),
            QDBusVariant(QLatin1String("text/plain")));
    echoBody.insert(QLatin1String("content"), QDBusVariant(text));

    MessagePartList echo;
    echo << echoHeader;
    echo << echoBody;
    header.insert(QLatin1String("delivery-echo"),
            QDBusVariant(QVariant::fromValue(echo)));

    MessagePartList parts;
    parts << header;
}

void TextChannel::gotProperties(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT(mPriv->getAllInFlight);
    mPriv->getAllInFlight = false;
    mPriv->gotProperties = true;

    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Messages)"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        ReadinessHelper *readinessHelper = mPriv->readinessHelper;
        if (readinessHelper->requestedFeatures().contains(FeatureMessageQueue) &&
            !readinessHelper->isReady(Features() << FeatureMessageQueue)) {
            readinessHelper->setIntrospectCompleted(FeatureMessageQueue, false, reply.error());
        }

        if (readinessHelper->requestedFeatures().contains(FeatureMessageCapabilities) &&
            !readinessHelper->isReady(Features() << FeatureMessageCapabilities)) {
            readinessHelper->setIntrospectCompleted(FeatureMessageCapabilities, false, reply.error());
        }
        return;
    }

    debug() << "Properties::GetAll(Channel.Interface.Messages) returned";
    mPriv->props = reply.value();

    mPriv->updateInitialMessages();
    mPriv->updateCapabilities();

    watcher->deleteLater();
}

void TextChannel::gotPendingMessages(QDBusPendingCallWatcher *watcher)
{
    Q_ASSERT(!mPriv->initialMessagesReceived);
    mPriv->initialMessagesReceived = true;

    QDBusPendingReply<PendingTextMessageList> reply = *watcher;
    if (reply.isError()) {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Messages)"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();

        // TODO should we fail here?
        mPriv->readinessHelper->setIntrospectCompleted(FeatureMessageQueue, false, reply.error());
        return;
    }

    debug() << "Text::ListPendingMessages returned";
    PendingTextMessageList list = reply.value();

    if (!list.isEmpty()) {
        foreach (const PendingTextMessage &message, list) {
            onTextReceived(message.identifier, message.unixTimestamp,
                    message.sender, message.messageType, message.flags,
                    message.text);
        }
        // processMessageQueue sets FeatureMessageQueue ready when the queue is empty for the first
        // time
    } else {
        mPriv->readinessHelper->setIntrospectCompleted(FeatureMessageQueue, true);
    }

    watcher->deleteLater();
}

void TextChannel::onChatStateChanged(uint contactHandle, uint state)
{
    mPriv->chatStateQueue.append(new Private::ChatStateEvent(
                contactHandle, state));
    mPriv->processChatStateQueue();
}

} // Tp
