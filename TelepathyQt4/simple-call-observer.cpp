/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include <TelepathyQt4/SimpleCallObserver>

#include "TelepathyQt4/_gen/simple-call-observer.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Account>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingComposite>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingSuccess>
#include <TelepathyQt4/SimpleObserver>
#include <TelepathyQt4/StreamedMediaChannel>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT SimpleCallObserver::Private
{
    Private(SimpleCallObserver *parent, const AccountPtr &account,
            const QString &contactIdentifier, bool requiresNormalization,
            CallDirection direction);

    SimpleCallObserver *parent;
    AccountPtr account;
    QString contactIdentifier;
    CallDirection direction;
    SimpleObserverPtr observer;
};

SimpleCallObserver::Private::Private(SimpleCallObserver *parent,
        const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction)
    : parent(parent),
      account(account),
      contactIdentifier(contactIdentifier),
      direction(direction)
{
    debug() << "Creating a new SimpleCallObserver";
    ChannelClassSpec channelFilter = ChannelClassSpec::streamedMediaCall();
    if (direction == CallDirectionIncoming) {
        channelFilter.setRequested(false);
    } else if (direction == CallDirectionOutgoing) {
        channelFilter.setRequested(true);
    }

    observer = SimpleObserver::create(account, ChannelClassSpecList() << channelFilter,
            contactIdentifier, requiresNormalization, QList<ChannelClassFeatures>());

    parent->connect(observer.data(),
            SIGNAL(newChannels(QList<Tp::ChannelPtr>)),
            SLOT(onNewChannels(QList<Tp::ChannelPtr>)));
    parent->connect(observer.data(),
            SIGNAL(channelInvalidated(Tp::ChannelPtr,QString,QString)),
            SLOT(onChannelInvalidated(Tp::ChannelPtr,QString,QString)));
}

/**
 * \class SimpleCallObserver
 * \ingroup utils
 * \headerfile TelepathyQt4/simple-call-observer.h <TelepathyQt4/SimpleCallObserver>
 *
 * \brief The SimpleCallObserver class provides an easy way to track calls
 *        in an account and can be optionally filtered by a contact and/or
 *        call direction.
 */

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account that respect \a direction.
 *
 * \param account The account used to listen to events.
 * \param direction The direction of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        CallDirection direction)
{
    return create(account, QString(), false, direction);
}

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account established with \a contact and
 * that respect \a direction.
 *
 * \param account The account used to listen to events.
 * \param contact The contact used to filter events.
 * \param direction The direction of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const ContactPtr &contact,
        CallDirection direction)
{
    if (contact) {
        return create(account, contact->id(), false, direction);
    }
    return create(account, QString(), false, direction);
}

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account established with a contact identified by \a
 * contactIdentifier and that respect \a direction.
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param direction The direction of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const QString &contactIdentifier,
        CallDirection direction)
{
    return create(account, contactIdentifier, true, direction);
}

SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction)
{
    return SimpleCallObserverPtr(
            new SimpleCallObserver(account, contactIdentifier,
                requiresNormalization, direction));
}

/**
 * Construct a new SimpleCallObserver object.
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param requiresNormalization Whether \a contactIdentifier needs to be
 *                              normalized.
 * \param direction The direction of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserver::SimpleCallObserver(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction)
    : mPriv(new Private(this, account, contactIdentifier, requiresNormalization, direction))
{
}

/**
 * Class destructor.
 */
SimpleCallObserver::~SimpleCallObserver()
{
    delete mPriv;
}

/**
 * Return the account used to listen to events.
 *
 * \return A pointer to the Account object.
 */
AccountPtr SimpleCallObserver::account() const
{
    return mPriv->account;
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact.
 */
QString SimpleCallObserver::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

/**
 * Return the direction of the calls used to filter events.
 *
 * \return The direction of the calls as SimpleCallObserver::CallDirection.
 */
SimpleCallObserver::CallDirection SimpleCallObserver::direction() const
{
    return mPriv->direction;
}

/**
 * Return the list of streamed media calls currently being observed.
 *
 * \return A list of pointers to StreamedMediaChannel objects.
 */
QList<StreamedMediaChannelPtr> SimpleCallObserver::streamedMediaCalls() const
{
    QList<StreamedMediaChannelPtr> ret;
    foreach (const ChannelPtr &channel, mPriv->observer->channels()) {
        StreamedMediaChannelPtr smChannel = StreamedMediaChannelPtr::qObjectCast(channel);
        if (smChannel) {
            ret << smChannel;
        }
    }
    return ret;
}

void SimpleCallObserver::onNewChannels(const QList<ChannelPtr> &channels)
{
    foreach (const ChannelPtr &channel, channels) {
        StreamedMediaChannelPtr smChannel = StreamedMediaChannelPtr::qObjectCast(channel);
        if (!smChannel) {
            if (channel->channelType() != TP_QT4_IFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
                warning() << "Channel received to observe is not of type StreamedMedia, service "
                    "confused. Ignoring channel";
            } else {
                warning() << "Channel received to observe is not a subclass of "
                    "StreamedMediaChannel. ChannelFactory set on this observer's account must "
                    "construct StreamedMediaChannel subclasses for channels of type StreamedMedia. "
                    "Ignoring channel";
            }
            continue;
        }

        emit streamedMediaCallStarted(StreamedMediaChannelPtr::qObjectCast(channel));
    }
}

void SimpleCallObserver::onChannelInvalidated(const ChannelPtr &channel,
        const QString &errorName, const QString &errorMessage)
{
    StreamedMediaChannelPtr smChannel = StreamedMediaChannelPtr::qObjectCast(channel);
    if (!smChannel) {
        if (channel->channelType() != TP_QT4_IFACE_CHANNEL_TYPE_STREAMED_MEDIA) {
            warning() << "Channel received by this observer is not of type StreamedMedia, service "
                "confused. Ignoring channel";
        } else {
            warning() << "Channel received by this observer is not a subclass of "
                "StreamedMediaChannel. ChannelFactory set on this observer's account must "
                "construct StreamedMediaChannel subclasses for channels of type StreamedMedia. "
                "Ignoring channel";
        }
        return;
    }
    emit streamedMediaCallEnded(smChannel, errorName, errorMessage);
}

/**
 * \fn void SimpleCallObserver::streamedMediaCallStarted(const Tp::StreamedMediaChannelPtr &channel)
 *
 * Emitted whenever a streamed media call that matches this observer's criteria is
 * started.
 *
 * \param channel The channel representing the streamed media call that started.
 */

/**
 * \fn void SimpleCallObserver::streamedMediaCallEnded(const Tp::StreamedMediaChannelPtr &channel,
 *          const QString &errorName, const QString &errorMessage)
 *
 * Emitted whenever a streamed media call that matches this observer's criteria has
 * ended.
 *
 * \param channel The channel representing the streamed media call that ended.
 * \param errorName A D-Bus error name (a string in a subset
 *                  of ASCII, prefixed with a reversed domain name).
 * \param errorMessage A debugging message associated with the error.
 */

} // Tp
