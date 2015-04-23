/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt/TextChannel>

#include "TelepathyQt/_gen/text-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Message>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/ReceivedMessage>
#include <TelepathyQt/ReferencedHandles>

#include <QDateTime>

namespace Tp
{

struct TP_QT_NO_EXPORT TextChannel::Private
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

    Client::ChannelTypeTextInterface *textInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // FeatureMessageCapabilities and FeatureMessageQueue
    QVariantMap props;
    bool getAllInFlight;
    bool gotProperties;

    // requires FeatureMessageCapabilities
    QList<ChannelTextMessageType> supportedMessageTypes;
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
      textInterface(parent->interface<Client::ChannelTypeTextInterface>()),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
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
        QStringList() << TP_QT_IFACE_CHANNEL_INTERFACE_CHAT_STATE,   // dependsOnInterfaces
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
        Client::ChannelInterfaceMessagesInterface *messagesInterface =
            parent->interface<Client::ChannelInterfaceMessagesInterface>();

        // FeatureMessageQueue needs signal connections + Get (but we
        // might as well do GetAll and reduce the number of code paths)
        parent->connect(messagesInterface,
                SIGNAL(MessageReceived(Tp::MessagePartList)),
                SLOT(onMessageReceived(Tp::MessagePartList)));
        parent->connect(messagesInterface,
                SIGNAL(PendingMessagesRemoved(Tp::UIntList)),
                SLOT(onPendingMessagesRemoved(Tp::UIntList)));

        if (!self->gotProperties && !self->getAllInFlight) {
            self->getAllInFlight = true;
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    self->properties->GetAll(
                        TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES),
                        parent);
            parent->connect(watcher,
                    SIGNAL(finished(QDBusPendingCallWatcher*)),
                    SLOT(gotProperties(QDBusPendingCallWatcher*)));
        } else if (self->gotProperties) {
            self->updateInitialMessages();
        }
    } else {
        // FeatureMessageQueue needs signal connections + ListPendingMessages
        parent->connect(self->textInterface,
                SIGNAL(Received(uint,uint,uint,uint,uint,QString)),
                SLOT(onTextReceived(uint,uint,uint,uint,uint,const QString)));

        // we present SendError signals as if they were incoming
        // messages, to be consistent with Messages
        parent->connect(self->textInterface,
                SIGNAL(SendError(uint,uint,uint,QString)),
                SLOT(onTextSendError(uint,uint,uint,QString)));

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                self->textInterface->ListPendingMessages(false), parent);
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
                    self->properties->GetAll(
                        TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES),
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
        Client::ChannelInterfaceMessagesInterface *messagesInterface =
            parent->interface<Client::ChannelInterfaceMessagesInterface>();

        parent->connect(messagesInterface,
                SIGNAL(MessageSent(Tp::MessagePartList,uint,QString)),
                SLOT(onMessageSent(Tp::MessagePartList,uint,QString)));
    } else {
        parent->connect(self->textInterface,
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
        parent->interface<Client::ChannelInterfaceChatStateInterface>();

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

    UIntList messageTypesAsUIntList = qdbus_cast<UIntList>(props[QLatin1String("MessageTypes")]);

    // Populate the list with the correct variable type
    supportedMessageTypes.clear();

    foreach (uint messageType, messageTypesAsUIntList) {
        supportedMessageTypes.append(static_cast<ChannelTextMessageType>(messageType));
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
        debug() << "MessageEvent:" << reinterpret_cast<const void *>(e);

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
                    ReceivedMessage removedMessage = messages.at(i);
                    messages.removeAt(i);
                    emit parent->pendingMessageRemoved(removedMessage);
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
    HandleIdentifierMap contactsRequired;
    foreach (const MessageEvent *e, incompleteMessages) {
        if (e->isMessage) {
            uint handle = e->message.senderHandle();
            if (handle != 0 && !e->message.sender()
                    && !awaitingContacts.contains(handle)) {
                contactsRequired.insert(handle, e->message.senderId());
            }
        }
    }

    if (contactsRequired.isEmpty()) {
        return;
    }

    ConnectionPtr conn = parent->connection();
    conn->lowlevel()->injectContactIds(contactsRequired);

    parent->connect(conn->contactManager()->contactsForHandles(
                contactsRequired.keys()),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onContactsFinished(Tp::PendingOperation*)));

    awaitingContacts |= contactsRequired.keys().toSet();
}

void TextChannel::Private::processChatStateQueue()
{
    while (!chatStateQueue.isEmpty()) {
        const ChatStateEvent *e = chatStateQueue.first();
        debug() << "ChatStateEvent:" << reinterpret_cast<const void *>(e);

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

    // TODO: pass id hints to ContactManager if we ever gain support to retrieve contact ids
    //       from ChatState.
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
 * \headerfile TelepathyQt/text-channel.h <TelepathyQt/TextChannel>
 *
 * \brief The TextChannel class represents a Telepathy channel of type Text.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * TextChannel object usable.
 *
 * This is currently the same as Channel::FeatureCore, but may change to include more.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature TextChannel::FeatureCore = Feature(QLatin1String(Channel::staticMetaObject.className()), 0, true);

/**
 * Feature used in order to access the message queue info.
 *
 * See message queue methods' documentation for more details.
 *
 * \sa messageQueue(), messageReceived(), pendingMessageRemoved(), forget(), acknowledge()
 */
const Feature TextChannel::FeatureMessageQueue = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to access message capabilities info.
 *
 * See message capabilities methods' documentation for more details.
 *
 * \sa supportedContentTypes(), messagePartSupport(), deliveryReportingSupport()
 */
const Feature TextChannel::FeatureMessageCapabilities = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 1);

/**
 * Feature used in order to receive notification when a message is sent.
 *
 * \sa messageSent()
 */
const Feature TextChannel::FeatureMessageSentSignal = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 2);

/**
 * Feature used in order to keep track of chat state changes.
 *
 * See chat state methods' documentation for more details.
 *
 * \sa chatState(), chatStateChanged()
 */
const Feature TextChannel::FeatureChatState = Feature(QLatin1String(TextChannel::staticMetaObject.className()), 3);

/**
 * \fn void TextChannel::messageSent(const Tp::Message &message,
 *          Tp::MessageSendingFlags flags,
 *          const QString &sentMessageToken)
 *
 * Emitted when a message is sent, if the TextChannel::FeatureMessageSentSignal
 * has been enabled.
 *
 * This signal is emitted regardless of whether the message is sent by this
 * client, or another client using the same channel via D-Bus.
 *
 * \param message A message. This may differ slightly from what the client
 *                requested to send, for instance if it has been altered due
 *                to limitations of the instant messaging protocol used.
 * \param flags #MessageSendingFlags that were in effect when the message was
 *              sent. Clients can use these in conjunction with
 *              deliveryReportingSupport() to determine whether delivery
 *              reporting can be expected.
 * \param sentMessageToken Either an empty QString, or an opaque token used
 *                         to match the message to any delivery reports.
 */

/**
 * \fn void TextChannel::messageReceived(const Tp::ReceivedMessage &message)
 *
 * Emitted when a message is added to messageQueue(), if the
 * TextChannel::FeatureMessageQueue Feature has been enabled.
 *
 * This occurs slightly later than the message being received over D-Bus;
 * see messageQueue() for details.
 *
 * \param message The message received.
 * \sa messageQueue(), acknowledge(), forget()
 */

/**
 * \fn void TextChannel::pendingMessageRemoved(
 *      const Tp::ReceivedMessage &message)
 *
 * Emitted when a message is removed from messageQueue(), if the
 * TextChannel::FeatureMessageQueue Feature has been enabled. See messageQueue() for the
 * circumstances in which this happens.
 *
 * \param message The message removed.
 * \sa messageQueue(), acknowledge(), forget()
 */

/**
 * \fn void TextChannel::chatStateChanged(const Tp::ContactPtr &contact,
 *      ChannelChatState state)
 *
 * Emitted when the state of a member of the channel has changed, if the
 * TextChannel::FeatureChatState feature has been enabled.
 *
 * Local state changes are also emitted here.
 *
 * \param contact The contact whose chat state changed.
 * \param state The new chat state for \a contact.
 * \sa chatState()
 */

/**
 * Create a new TextChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A TextChannelPtr object pointing to the newly created
 *         TextChannel object.
 */
TextChannelPtr TextChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return TextChannelPtr(new TextChannel(connection, objectPath,
                immutableProperties, TextChannel::FeatureCore));
}

/**
 * Construct a new TextChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on TextChannel::FeatureCore.
 */
TextChannel::TextChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
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
 * Return whether this channel supports the Messages interface.
 *
 * If the interface is not supported, some advanced functionality will be unavailable.
 *
 * This method requires TextChannel::FeatureCore to be ready.
 *
 * \return \c true if the Messages interface is supported, \c false otherwise.
 */
bool TextChannel::hasMessagesInterface() const
{
    return interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_MESSAGES);
}

/**
 * Return whether this channel supports the ChatState interface.
 *
 * If the interface is not supported, requestChatState() will fail and all contacts' chat states
 * will appear to be #ChannelChatStateInactive.
 *
 * This method requires TextChannel::FeatureCore to be ready.
 *
 * \return \c true if the ChatState interface is supported, \c false otherwise.
 * \sa requestChatState(), chatStateChanged()
 */
bool TextChannel::hasChatStateInterface() const
{
    return interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_CHAT_STATE);
}

/**
 * Return whether contacts can be invited into this channel using
 * inviteContacts() (which is equivalent to Channel::groupAddContacts()).
 *
 * Whether this is the case depends on the underlying protocol, the type of channel,
 * and the user's privileges (in some chatrooms, only a privileged user
 * can invite other contacts).
 *
 * This is an alias for Channel::groupCanAddContacts(), to indicate its meaning more
 * clearly for Text channels.
 *
 * This method requires Channel::FeatureCore to be ready.
 *
 * \return \c true if contacts can be invited, \c false otherwise.
 * \sa inviteContacts(), Channel::groupCanAddContacts(), Channel::groupAddContacts()
 */
bool TextChannel::canInviteContacts() const
{
    return groupCanAddContacts();
}

/* <!--x--> in the block below is used to escape the star-slash sequence */
/**
 * Return a list of supported MIME content types for messages on this channel.
 *
 * For a simple text channel this will be a list containing one item,
 * "text/plain".
 *
 * This list may contain the special value "*<!--x-->/<!--x-->*", which
 * indicates that any content type is supported.
 *
 * This method requires TextChannel::FeatureMessageCapabilities to be ready.
 *
 * \return The list of MIME content types.
 */
QStringList TextChannel::supportedContentTypes() const
{
    return mPriv->supportedContentTypes;
}

/**
 * Return the message types supported by this channel.
 *
 * This method requires TextChannel::FeatureMessageCapabilities to be ready.
 *
 * \return The list of supported message types
 */
QList<ChannelTextMessageType> TextChannel::supportedMessageTypes() const
{
    if (!isReady(FeatureMessageCapabilities)) {
        warning() << "TextChannel::supportedMessageTypes() used with "
                "FeatureMessageCapabilities not ready";
    }

    return mPriv->supportedMessageTypes;
}

/**
 * Return whether the provided message type is supported.
 *
 * This method requires TextChannel::FeatureMessageCapabilities to be ready.
 *
 * \param messageType The message type to check.
 * \return \c true if supported, \c false otherwise
 */
bool TextChannel::supportsMessageType(ChannelTextMessageType messageType) const
{
    if (!isReady(FeatureMessageCapabilities)) {
        warning() << "TextChannel::supportsMessageType() used with "
                "FeatureMessageCapabilities not ready";
    }

    return mPriv->supportedMessageTypes.contains(messageType);
}

/**
 * Return a set of flags indicating support for multi-part messages on this
 * channel.
 *
 * This is zero on simple text channels, or greater than zero if
 * there is partial or full support for multi-part messages.
 *
 * This method requires TextChannel::FeatureMessageCapabilities to be ready.
 *
 * \return The flags as #MessagePartSupportFlags.
 */
MessagePartSupportFlags TextChannel::messagePartSupport() const
{
    return mPriv->messagePartSupport;
}

/**
 * Return a set of flags indicating support for delivery reporting on this
 * channel.
 *
 * This is zero if there are no particular guarantees, or greater
 * than zero if delivery reports can be expected under certain circumstances.
 *
 * This method requires TextChannel::FeatureMessageCapabilities to be ready.
 *
 * \return The flags as #DeliveryReportingSupportFlags.
 */
DeliveryReportingSupportFlags TextChannel::deliveryReportingSupport() const
{
    return mPriv->deliveryReportingSupport;
}

/**
 * Return a list of messages received in this channel.
 *
 * Messages are added to this list when they are received from the instant
 * messaging service; the messageReceived() signal is emitted.
 *
 * There is a small delay between the message being received over D-Bus and
 * becoming available to users of this C++ API, since a small amount of
 * additional information needs to be fetched. However, the relative ordering
 * of all the messages in a channel is preserved.
 *
 * Messages are removed from this list when they are acknowledged with the
 * acknowledge() or forget() methods. On channels where hasMessagesInterface()
 * returns \c true, they will also be removed when acknowledged by a different
 * client. In either case, the pendingMessageRemoved() signal is emitted.
 *
 * This method requires TextChannel::FeatureMessageQueue to be ready.
 *
 * \return A list of ReceivedMessage objects.
 * \sa messageReceived()
 */
QList<ReceivedMessage> TextChannel::messageQueue() const
{
    return mPriv->messages;
}

/**
 * Return the current chat state for \a contact.
 *
 * If hasChatStateInterface() returns \c false, this method will always return
 * #ChannelChatStateInactive.
 *
 * This method requires TextChannel::FeatureChatState to be ready.
 *
 * \return The contact chat state as #ChannelChatState.
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
            mPriv->textInterface->AcknowledgePendingMessages(UIntList() << id);
        }
    }

    mPriv->acknowledgeBatches.remove(watcher);
    watcher->deleteLater();
}

/**
 * Acknowledge that received messages have been displayed to the user.
 *
 * Note that this method should only be called by the main handler of a channel, usually
 * meaning the user interface process that displays the channel to the user
 * (when a channel dispatcher is used, the handler must acknowledge messages,
 * and other approvers or observers must not acknowledge messages).
 *
 * Processes other than the main handler of a channel can free memory used
 * by the library by calling forget() instead.
 *
 * This method requires TextChannel::FeatureMessageQueue to be ready.
 *
 * \param messages A list of received messages that have now been displayed.
 * \sa forget(), messageQueue(), messageReceived(), pendingMessageRemoved()
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
            mPriv->textInterface->AcknowledgePendingMessages(ids),
            this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onAcknowledgePendingMessagesReply(QDBusPendingCallWatcher*)));
    mPriv->acknowledgeBatches[watcher] = ids;
}

/**
 * Remove messages from the message queue without acknowledging them.
 *
 * Note that this method frees memory used by the library, but
 * does not free the corresponding memory in the CM process.
 * It should be used by clients that are not the main handler for a channel;
 * the main handler for a channel should use acknowledge() instead.
 *
 * This method requires TextChannel::FeatureMessageQueue to be ready.
 *
 * \param messages A list of received messages that have now been processed.
 * \sa acknowledge(), messageQueue(), messageReceived(), pendingMessageRemoved()
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

/**
 * Request that a message be sent on this channel.
 *
 * When the message has been submitted for delivery,
 * this method will return and the messageSent() signal will be emitted.
 *
 * If the message cannot be submitted for delivery, the returned pending operation will fail and no
 * signal is emitted.
 *
 * This method requires TextChannel::FeatureCore to be ready.
 *
 * \param text The message body.
 * \param type The message type.
 * \param flags Flags affecting how the message is sent.
 *              Note that the channel may ignore some or all flags, depending on
 *              deliveryReportingSupport(); the flags that were handled by the CM are provided in
 *              messageSent().
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the message has been submitted for delivery.
 * \sa messageSent()
 */
PendingSendMessage *TextChannel::send(const QString &text,
        ChannelTextMessageType type, MessageSendingFlags flags)
{
    Message m(type, text);
    PendingSendMessage *op = new PendingSendMessage(TextChannelPtr(this), m);

    if (hasMessagesInterface()) {
        Client::ChannelInterfaceMessagesInterface *messagesInterface =
            interface<Client::ChannelInterfaceMessagesInterface>();

        connect(new QDBusPendingCallWatcher(
                    messagesInterface->SendMessage(m.parts(),
                        (uint) flags)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onMessageSent(QDBusPendingCallWatcher*)));
    } else {
        connect(new QDBusPendingCallWatcher(mPriv->textInterface->Send(type, text)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onTextSent(QDBusPendingCallWatcher*)));
    }
    return op;
}

/**
 * Request that a message be sent on this channel.
 *
 * When the message has been submitted for delivery,
 * this method will return and the messageSent() signal will be emitted.
 *
 * If the message cannot be submitted for delivery, the returned pending operation will fail and no
 * signal is emitted.
 *
 * This method requires TextChannel::FeatureCore to be ready.
 *
 * \param parts The message parts.
 * \param flags Flags affecting how the message is sent.
 *              Note that the channel may ignore some or all flags, depending on
 *              deliveryReportingSupport(); the flags that were handled by the CM are provided in
 *              messageSent().
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the message has been submitted for delivery.
 * \sa messageSent()
 */
PendingSendMessage *TextChannel::send(const MessagePartList &parts,
        MessageSendingFlags flags)
{
    Message m(parts);
    PendingSendMessage *op = new PendingSendMessage(TextChannelPtr(this), m);

    if (hasMessagesInterface()) {
        Client::ChannelInterfaceMessagesInterface *messagesInterface =
            interface<Client::ChannelInterfaceMessagesInterface>();

        connect(new QDBusPendingCallWatcher(
                    messagesInterface->SendMessage(m.parts(),
                        (uint) flags)),
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                op,
                SLOT(onMessageSent(QDBusPendingCallWatcher*)));
    } else {
        connect(new QDBusPendingCallWatcher(mPriv->textInterface->Send(
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
 * This method requires TextChannel::FeatureCore to be ready.
 *
 * \param state The new state.
 * \sa chatStateChanged()
 */
PendingOperation *TextChannel::requestChatState(ChannelChatState state)
{
    if (!interfaces().contains(TP_QT_IFACE_CHANNEL_INTERFACE_CHAT_STATE)) {
        warning() << "TextChannel::requestChatState() used with no chat "
            "state interface";
        return new PendingFailure(TP_QT_ERROR_NOT_IMPLEMENTED,
                QLatin1String("TextChannel does not support chat state interface"),
                TextChannelPtr(this));
    }

    Client::ChannelInterfaceChatStateInterface *chatStateInterface =
        interface<Client::ChannelInterfaceChatStateInterface>();
    return new PendingVoid(chatStateInterface->SetChatState(
                (uint) state), TextChannelPtr(this));
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
