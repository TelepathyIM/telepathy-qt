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

#include <TelepathyQt4/Client/TextChannel>

#include "TelepathyQt4/Client/_gen/text-channel.moc.hpp"

#include <TelepathyQt4/Client/PendingReadyChannel>

#include "TelepathyQt4/debug-internal.h"

namespace Telepathy
{
namespace Client
{

struct TextChannel::Private
{
    inline Private();
    inline ~Private();

    TextChannel::Features features;
    TextChannel::Features desiredFeatures;
    QList<PendingReadyChannel *> pendingReady;
    void continueReadying(TextChannel *channel);
    void failReadying(TextChannel *channel, const QString &error,
            const QString &message);

    // requires FeatureMessageCapabilities
    QStringList supportedContentTypes;
    MessagePartSupportFlags messagePartSupport;
    DeliveryReportingSupportFlags deliveryReportingSupport;
};

TextChannel::Private::Private()
    : features(0),
      desiredFeatures(0),
      pendingReady(),
      supportedContentTypes(),
      messagePartSupport(0),
      deliveryReportingSupport(0)
{
}

TextChannel::Private::~Private()
{
}

/**
 * \class TextChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/Client/text-channel.h <TelepathyQt4/Client/TextChannel>
 *
 * High-level proxy object for accessing remote %Channel objects of the Text
 * channel type.
 *
 * This subclass of Channel will eventually provide a high-level API for the
 * Text and Messages interface. Until then, it's just a Channel.
 */

/**
 * Creates a TextChannel associated with the given object on the same service
 * as the given connection.
 *
 * \param connection  Connection owning this TextChannel, and specifying the
 *                    service.
 * \param objectPath  Path to the object on the service.
 * \param parent      Passed to the parent class constructor.
 */
TextChannel::TextChannel(Connection *connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        QObject *parent)
    : Channel(connection, objectPath, immutableProperties, parent),
      mPriv(new Private())
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
 * If it does not, some advanced functionality will be unavailable.
 *
 * \return true if the Messages interface is supported
 */
bool TextChannel::hasMessagesInterface() const
{
    return interfaces().contains(QLatin1String(
                TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES));
}

/**
 * Return whether the desired features are ready for use.
 *
 * \return true if all the requested features are ready
 */
bool TextChannel::isReady(Channel::Features channelFeatures,
        Features textFeatures) const
{
    debug() << "Checking whether ready:" << channelFeatures << textFeatures;
    debug() << "Am I ready? mPriv->features =" << mPriv->features;
    return Channel::isReady(channelFeatures) &&
        ((mPriv->features & textFeatures) == textFeatures);
}

PendingReadyChannel *TextChannel::becomeReady(
        Channel::Features channelFeatures,
        Features textFeatures)
{
    PendingReadyChannel *textReady;

    if (!isValid()) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinishedWithError(invalidationReason(),
                invalidationMessage());
        return textReady;
    }

    if (isReady(channelFeatures, textFeatures)) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinished();
        return textReady;
    }

    if ((textFeatures & (FeatureMessageQueue | FeatureMessageCapabilities))
            != textFeatures) {
        textReady = new PendingReadyChannel(channelFeatures, this);
        textReady->setFinishedWithError(TELEPATHY_ERROR_INVALID_ARGUMENT,
                "Invalid features argument");
        return textReady;
    }

    PendingReadyChannel *channelReady = Channel::becomeReady(channelFeatures);

    textReady = new PendingReadyChannel(channelFeatures, this);

    connect(channelReady,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onChannelReady(Telepathy::Client::PendingOperation *)));

    mPriv->pendingReady.append(textReady);
    mPriv->desiredFeatures |= textFeatures;
    return textReady;
}

void TextChannel::onChannelReady(Telepathy::Client::PendingOperation *op)
{
    // Handle the error and insanity cases
    if (op->isError()) {
        mPriv->failReadying(this, op->errorName(), op->errorMessage());
    }
    if (channelType() != QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)) {
        mPriv->failReadying(this,
                QLatin1String(TELEPATHY_ERROR_INVALID_ARGUMENT),
                QLatin1String("TextChannel object with non-Text channel"));
        return;
    }

    // Now that the basic Channel stuff is ready, we can know whether we have
    // the Messages interface.
    if (hasMessagesInterface()) {
        connect(messagesInterface(),
                SIGNAL(MessageSent(const Telepathy::MessagePartList &,
                        uint, const QString &)),
                this,
                SLOT(onMessageSent(const Telepathy::MessagePartList &,
                        uint, const QString &)));
    } else {
        // For plain Text channels, FeatureMessageCapabilities is trivial to
        // implement - we don't do anything special at all - so we might as
        // well set it up even if the library user didn't actually care.
        mPriv->supportedContentTypes =
            (QStringList(QLatin1String("text/plain")));
        mPriv->messagePartSupport = 0;
        mPriv->deliveryReportingSupport = 0;
        mPriv->features |= FeatureMessageCapabilities;

        connect(textInterface(),
                SIGNAL(Sent(uint, uint, const QString &)),
                this,
                SLOT(onTextSent(uint, uint, const QString &)));
    }

    mPriv->continueReadying(this);
}

void TextChannel::Private::failReadying(TextChannel *channel,
        const QString &error, const QString &message)
{
    QList<PendingReadyChannel *> ops = pendingReady;
    pendingReady.clear();

    foreach (PendingReadyChannel *op, ops) {
        op->setFinishedWithError(error, message);
    }
    channel->invalidate(error, message);
}

void TextChannel::Private::continueReadying(TextChannel *channel)
{
    if ((desiredFeatures & features) == desiredFeatures) {
        // everything we wanted is ready
        QList<PendingReadyChannel *> ops = pendingReady;
        pendingReady.clear();
        foreach (PendingReadyChannel *op, ops) {
            op->setFinished();
        }
        return;
    }

    // else there's more work to do yet

    if (channel->hasMessagesInterface()) {
        // FeatureMessageQueue needs signal connections + Get (but we
        //  might as well do GetAll and reduce the number of code paths)
        // FeatureMessageCapabilities needs GetAll

        if (desiredFeatures & TextChannel::FeatureMessageQueue) {
            channel->connect(channel->messagesInterface(),
                    SIGNAL(MessageReceived(const Telepathy::MessagePartList &)),
                    SLOT(onMessageReceived(const Telepathy::MessagePartList &)));

            channel->connect(channel->messagesInterface(),
                    SIGNAL(PendingMessagesRemoved(const Telepathy::UIntList &)),
                    SLOT(onPendingMessagesRemoved(const Telepathy::UIntList &)));
        }

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                channel->propertiesInterface()->GetAll(
                    QLatin1String(TELEPATHY_INTERFACE_CHANNEL_INTERFACE_MESSAGES)),
                    channel);
        channel->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onGetAllMessagesReply(QDBusPendingCallWatcher *)));
    } else {
        // FeatureMessageQueue needs signal connections +
        //  ListPendingMessages
        // FeatureMessageCapabilities already done

        // this is all we can have right now
        Q_ASSERT(desiredFeatures == TextChannel::FeatureMessageQueue);

        channel->connect(channel->textInterface(),
                SIGNAL(Received(uint, uint, uint, uint, uint,
                        const QString &)),
                SLOT(onTextReceived(uint, uint, uint, uint, uint,
                        const QString &)));

        // we present SendError signals as if they were incoming messages,
        // to be consistent with Messages
        channel->connect(channel->textInterface(),
                SIGNAL(SendError(uint, uint, uint, const QString &)),
                SLOT(onTextSendError(uint, uint, uint, const QString &)));

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                channel->textInterface()->ListPendingMessages(false), channel);
        channel->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(onListPendingMessagesReply(QDBusPendingCallWatcher *)));
    }
}

void TextChannel::onMessageSent(const Telepathy::MessagePartList &parts,
        uint flags,
        const QString &sentMessageToken)
{
}

void TextChannel::onMessageReceived(const Telepathy::MessagePartList &parts)
{
}

void TextChannel::onPendingMessagesRemoved(const Telepathy::UIntList &ids)
{
}

void TextChannel::onTextSent(uint timestamp, uint type, const QString &text)
{
}

void TextChannel::onTextReceived(uint id, uint timestamp, uint sender,
        uint type, uint flags, const QString &text)
{
}

void TextChannel::onTextSendError(uint error, uint timestamp, uint type,
        const QString &text)
{
}

void TextChannel::onGetAllMessagesReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Properties::GetAll(Channel.Interface.Messages) returned";
        props = reply.value();
    } else {
        warning().nospace() << "Properties::GetAll(Channel.Interface.Messages)"
            " failed with " << reply.error().name() << ": " <<
            reply.error().message();
        // ... and act as though props had been empty
    }

    if (mPriv->desiredFeatures & FeatureMessageQueue) {
        // FIXME: actually put the messages in the queue

        mPriv->features |= FeatureMessageQueue;
    }

    mPriv->supportedContentTypes = qdbus_cast<QStringList>(
            props["SupportedContentTypes"]);
    if (mPriv->supportedContentTypes.isEmpty()) {
        mPriv->supportedContentTypes << QLatin1String("text/plain");
    }
    mPriv->messagePartSupport = MessagePartSupportFlags(qdbus_cast<uint>(
            props["MessagePartSupportFlags"]));
    mPriv->deliveryReportingSupport = DeliveryReportingSupportFlags(
            qdbus_cast<uint>(props["DeliveryReportingSupport"]));

    mPriv->features |= FeatureMessageCapabilities;
    mPriv->continueReadying(this);
}

void TextChannel::onListPendingMessagesReply(QDBusPendingCallWatcher *watcher)
{
    // FIXME: actually put the messages in the queue

    mPriv->features |= FeatureMessageQueue;
    mPriv->continueReadying(this);
}

} // Telepathy::Client
} // Telepathy
