/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/CapabilitiesBase>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT CapabilitiesBase::Private : public QSharedData
{
    Private(bool specificToContact);
    Private(const RequestableChannelClassSpecList &rccSpecs, bool specificToContact);

    RequestableChannelClassSpecList rccSpecs;
    bool specificToContact;
};

CapabilitiesBase::Private::Private(bool specificToContact)
    : specificToContact(specificToContact)
{
}

CapabilitiesBase::Private::Private(const RequestableChannelClassSpecList &rccSpecs,
        bool specificToContact)
    : rccSpecs(rccSpecs),
      specificToContact(specificToContact)
{
}

/**
 * \class CapabilitiesBase
 * \ingroup clientconn
 * \headerfile TelepathyQt/capabilities-base.h <TelepathyQt/CapabilitiesBase>
 *
 * \brief The CapabilitiesBase class represents the capabilities a Connection
 * or a Contact supports.
 */

/**
 * Construct a new CapabilitiesBase object.
 */
CapabilitiesBase::CapabilitiesBase()
    : mPriv(new Private(false))
{
}

/**
 * Construct a new CapabilitiesBase object.
 *
 * \param specificToContact Whether this object describes the capabilities of a
 *                          particular contact.
 */
CapabilitiesBase::CapabilitiesBase(bool specificToContact)
    : mPriv(new Private(specificToContact))
{
}

/**
 * Construct a new CapabilitiesBase object using the given \a rccs.
 *
 * \param rccs RequestableChannelClassList representing the capabilities of a
 *             connection or contact.
 * \param specificToContact Whether this object describes the capabilities of a
 *                          particular contact.
 */
CapabilitiesBase::CapabilitiesBase(const RequestableChannelClassList &rccs,
        bool specificToContact)
    : mPriv(new Private(RequestableChannelClassSpecList(rccs), specificToContact))
{
}

/**
 * Construct a new CapabilitiesBase object using the given \a rccSpecs.
 *
 * \param rccSpecs RequestableChannelClassSpecList representing the capabilities of a
 *                 connection or contact.
 * \param specificToContact Whether this object describes the capabilities of a
 *                          particular contact.
 */
CapabilitiesBase::CapabilitiesBase(const RequestableChannelClassSpecList &rccSpecs,
        bool specificToContact)
    : mPriv(new Private(rccSpecs, specificToContact))
{
}

CapabilitiesBase::CapabilitiesBase(const CapabilitiesBase &other)
    : mPriv(other.mPriv)
{
}

/**
 * Class destructor.
 */
CapabilitiesBase::~CapabilitiesBase()
{
}

CapabilitiesBase &CapabilitiesBase::operator=(const CapabilitiesBase &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

/**
 * Return the list of requestable channel class spec representing the requests that can succeed.
 *
 * This can be used by advanced clients to determine whether an unusually
 * complex request would succeed. See the \telepathy_spec
 * for details of how to interpret the returned list.
 *
 * The higher-level methods like textChats() are likely to be more
 * useful to the majority of clients.
 *
 * \return A RequestableChannelClassSpecList indicating the parameters to
 *         Account::createChannel, Account::ensureChannel,
 *         Connection::createChannel and Connection::ensureChannel
 *         that can be expected to work.
 */
RequestableChannelClassSpecList CapabilitiesBase::allClassSpecs() const
{
    return mPriv->rccSpecs;
}

void CapabilitiesBase::updateRequestableChannelClasses(
        const RequestableChannelClassList &rccs)
{
    mPriv->rccSpecs = RequestableChannelClassSpecList(rccs);
}

/**
 * Return whether this object accurately describes the capabilities of a
 * particular contact, or if it's only a guess based on the
 * capabilities of the underlying connection.
 *
 * In protocols like XMPP where each contact advertises their capabilities
 * to others, Contact::capabilities() will generally return an object where
 * this method returns true.
 *
 * In protocols like SIP where contacts' capabilities are not known,
 * Contact::capabilities() will return an object where this method returns
 * false, whose methods textChats() etc. are based on what the
 * underlying connection supports.
 *
 * This reflects the fact that the best assumption an application can make is
 * that every contact supports every channel type supported by the connection,
 * while indicating that requests to communicate might fail if the contact
 * does not actually have the necessary functionality.
 *
 * \return \c true if this object describes the capabilities of a particular
 *         contact, \c false otherwise.
 */
bool CapabilitiesBase::isSpecificToContact() const
{
    return mPriv->specificToContact;
}

/**
 * Return whether private text channels can be established by providing
 * a contact identifier.
 *
 * If the protocol is such that text chats can be established, but only via
 * a more elaborate D-Bus API than normal (because more information is needed),
 * then this method will return false.
 *
 * \return \c true if Account::ensureTextChat() can be expected to work,
 *         \c false otherwise.
 */
bool CapabilitiesBase::textChats() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::textChat())) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::audioCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::audioCall())) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::videoCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::videoCall())) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::videoCallsWithAudio() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::videoCallWithAudioAllowed()) ||
            rccSpec.supports(RequestableChannelClassSpec::audioCallWithVideoAllowed())) {
            return true;
        }
    }
    return false;
}

bool CapabilitiesBase::upgradingCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.channelType() == TP_QT_IFACE_CHANNEL_TYPE_CALL &&
            rccSpec.allowsProperty(TP_QT_IFACE_CHANNEL_TYPE_CALL + QLatin1String(".MutableContents"))) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether private audio and/or video calls can be established by
 * providing a contact identifier.
 *
 * If the protocol is such that these calls can be established, but only via
 * a more elaborate D-Bus API than normal (because more information is needed),
 * then this method will return false.
 *
 * \return \c true if Account::ensureStreamedMediaCall() can be expected to work,
 *         \c false otherwise.
 * \sa streamedMediaAudioCalls(), streamedMediaVideoCalls(),
 *     streamedMediaVideoCallsWithAudio()
 */
bool CapabilitiesBase::streamedMediaCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::streamedMediaCall())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether private audio calls can be established by providing a
 * contact identifier.
 *
 * Call upgradingCalls() to determine whether such calls are
 * likely to be upgradable to have a video stream later.
 *
 * If the protocol is such that these calls can be established, but only via
 * a more elaborate D-Bus API than normal (because more information is needed),
 * then this method will return false.
 *
 * In some older connection managers, streamedMediaAudioCalls() and
 * streamedMediaVideoCalls() might both return false, even though streamedMediaCalls() returns
 * true. This indicates that only an older API is supported - clients of these connection managers
 * must call Account::ensureStreamedMediaCall() to get an empty call, then add audio and/or
 * video streams to it.
 *
 * \return \c true if Account::ensureStreamedMediaAudioCall() can be expected to work,
 *         \c false otherwise.
 * \sa streamedMediaCalls(), streamedMediaVideoCalls(), streamedMediaVideoCallsWithAudio()
 */
bool CapabilitiesBase::streamedMediaAudioCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::streamedMediaAudioCall())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether private video calls can be established by providing a
 * contact identifier.
 *
 * The same comments as for streamedMediaAudioCalls() apply to this method.
 *
 * \return \c true if Account::ensureStreamedMediaVideoCall() can be expected to work,
 *         if given \c false as \a withAudio parameter, \c false otherwise.
 * \sa streamedMediaCalls(), streamedMediaAudioCalls(), streamedMediaVideoCallsWithAudio()
 */
bool CapabilitiesBase::streamedMediaVideoCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::streamedMediaVideoCall())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether private video calls with audio can be established by providing a
 * contact identifier.
 *
 * The same comments as for streamedMediaAudioCalls() apply to this method.
 *
 * \return \c true if Account::ensureStreamedMediaVideoCall() can be expected to work,
 *         if given \c true as \a withAudio parameter, \c false otherwise.
 * \sa streamedMediaCalls(), streamedMediaAudioCalls(), streamedMediaVideoCalls()
 */
bool CapabilitiesBase::streamedMediaVideoCallsWithAudio() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::streamedMediaVideoCallWithAudio())) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether the protocol supports adding streams of a different type
 * to ongoing media calls.
 *
 * In some protocols and clients (such as XMPP Jingle), all calls potentially
 * support both audio and video. This is indicated by returning true.
 *
 * In other protocols and clients (such as MSN, and the variant of XMPP Jingle
 * used by Google clients), the streams are fixed at the time the call is
 * started, so if you will ever want video, you have to ask for it at the
 * beginning, for instance with ensureStreamedMediaVideoCall(). This is indicated by
 * returning false.
 *
 * User interfaces can use this method as a UI hint. If it returns false,
 * then a UI wishing to support both audio and video calls will have to
 * provide separate "audio call" and "video call" buttons or menu items;
 * if it returns true, a single button that makes an audio call is sufficient,
 * because video can be added later.
 *
 * (The underlying Telepathy feature is the ImmutableStreams property; if this
 * method returns true, then ImmutableStreams is false, and vice versa).
 *
 * \return \c true if audio calls can be upgraded to audio + video,
 *         \c false otherwise.
 */
bool CapabilitiesBase::upgradingStreamedMediaCalls() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.channelType() == TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !rccSpec.allowsProperty(TP_QT_IFACE_CHANNEL_TYPE_STREAMED_MEDIA + QLatin1String(".ImmutableStreams"))) {
            // TODO should we test all classes that have channelType
            //      StreamedMedia or just one is fine?
            return true;
        }
    }
    return false;
}

/**
 * Return whether file transfer can be established by providing a contact identifier
 *
 * \return \c true if file transfers can be expected to work,
 *         \c false otherwise.
 */
bool CapabilitiesBase::fileTransfers() const
{
    foreach (const RequestableChannelClassSpec &rccSpec, mPriv->rccSpecs) {
        if (rccSpec.supports(RequestableChannelClassSpec::fileTransfer())) {
            return true;
        }
    }
    return false;
}

} // Tp
