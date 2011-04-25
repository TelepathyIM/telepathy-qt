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
            CallDirection direction, CallType type);

    SimpleCallObserver *parent;
    AccountPtr account;
    QString contactIdentifier;
    CallDirection direction;
    CallType type;
    SimpleObserverPtr observer;
};

SimpleCallObserver::Private::Private(SimpleCallObserver *parent,
        const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction, CallType type)
    : parent(parent),
      account(account),
      contactIdentifier(contactIdentifier),
      direction(direction),
      type(type)
{
    ChannelClassSpec channelFilter = ChannelClassSpec::streamedMediaCall();
    if (direction == CallDirectionIncoming) {
        channelFilter.setRequested(false);
    } else if (direction == CallDirectionOutgoing) {
        channelFilter.setRequested(true);
    }
    if (type & CallTypeAudio) {
        channelFilter.setStreamedMediaInitialAudioFlag();
    }
    if (type & CallTypeVideo) {
        channelFilter.setStreamedMediaInitialVideoFlag();
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
 *        in an account and can be optionally filtered by a contact, type or
 *        direction.
 */

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account that respect \a direction and \a type.
 *
 * \param account The account used to listen to events.
 * \param direction The direction of the calls used to filter events.
 * \param type The type of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        CallDirection direction, CallType type)
{
    return create(account, QString(), false, direction, type);
}

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account established with \a contact and
 * that respect \a direction and \a type.
 *
 * \param account The account used to listen to events.
 * \param contact The contact used to filter events.
 * \param direction The direction of the calls used to filter events.
 * \param type The type of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const ContactPtr &contact,
        CallDirection direction, CallType type)
{
    if (contact) {
        return create(account, contact->id(), false, direction, type);
    }
    return create(account, QString(), false, direction, type);
}

/**
 * Create a new SimpleCallObserver object.
 *
 * Events will be signalled for all calls in \a account established with a contact identified by \a
 * contactIdentifier and that respect \a direction and \a type.
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param direction The direction of the calls used to filter events.
 * \param type The type of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const QString &contactIdentifier,
        CallDirection direction, CallType type)
{
    return create(account, contactIdentifier, true, direction, type);
}

SimpleCallObserverPtr SimpleCallObserver::create(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction, CallType type)
{
    return SimpleCallObserverPtr(
            new SimpleCallObserver(account, contactIdentifier,
                requiresNormalization, direction, type));
}

/**
 * Construct a new SimpleCallObserver object.
 *
 * \param account The account used to listen to events.
 * \param contactIdentifier The identifier of the contact used to filter events.
 * \param requiresNormalization Whether \a contactIdentifier needs to be
 *                              normalized.
 * \param direction The direction of the calls used to filter events.
 * \param type The type of the calls used to filter events.
 * \return An SimpleCallObserverPtr object pointing to the newly created
 *         SimpleCallObserver object.
 */
SimpleCallObserver::SimpleCallObserver(const AccountPtr &account,
        const QString &contactIdentifier, bool requiresNormalization,
        CallDirection direction, CallType type)
    : mPriv(new Private(this, account, contactIdentifier, requiresNormalization, direction, type))
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
 * \return The account used to listen to events.
 */
AccountPtr SimpleCallObserver::account() const
{
    return mPriv->account;
}

/**
 * Return the identifier of the contact used to filter events, or an empty string if none was
 * provided at construction.
 *
 * \return The identifier of the contact used to filter events.
 */
QString SimpleCallObserver::contactIdentifier() const
{
    return mPriv->contactIdentifier;
}

/**
 * Return the direction of the calls used to filter events.
 *
 * \return The direction of the calls used to filter events.
 */
SimpleCallObserver::CallDirection SimpleCallObserver::direction() const
{
    return mPriv->direction;
}

/**
 * Return the type of the calls used to filter events.
 *
 * \return The type of the calls used to filter events.
 */
SimpleCallObserver::CallType SimpleCallObserver::type() const
{
    return mPriv->type;
}

/**
 * Return the list of streamed media calls currently being observed.
 *
 * \return The list of streamed media calls currently being observed.
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
        emit streamedMediaCallStarted(StreamedMediaChannelPtr::qObjectCast(channel));
    }
}

void SimpleCallObserver::onChannelInvalidated(const ChannelPtr &channel,
        const QString &errorName, const QString &errorMessage)
{
    emit streamedMediaCallEnded(StreamedMediaChannelPtr::qObjectCast(channel),
            errorName, errorMessage);
}

/**
 * \fn void SimpleCallObserver::streamedMediaCallStarted(const Tp::StreamedMediaChannelPtr &channel)
 *
 * This signal is emitted whenever a streamed media call that matches this observer's criteria is
 * started.
 *
 * \param channel The channel representing the streamed media call that started.
 */

/**
 * \fn void SimpleCallObserver::streamedMediaCallEnded(const Tp::StreamedMediaChannelPtr &channel,
 *          const QString &errorName, const QString &errorMessage)
 *
 * This signal is emitted whenever a streamed media call that matches this observer's criteria has
 * ended.
 *
 * \param channel The channel representing the streamed media call that ended.
 * \param errorName A D-Bus error name (a string in a subset
 *                  of ASCII, prefixed with a reversed domain name).
 * \param errorMessage A debugging message associated with the error.
 */

} // Tp
