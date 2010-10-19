/*
 * This file is part of TelepathyQt4
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

#include <TelepathyQt4/CapabilitiesBase>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT CapabilitiesBase::Private
{
    Private(bool specificToContact);
    Private(const RequestableChannelClassList &classes, bool specificToContact);

    RequestableChannelClassList classes;
    bool specificToContact;
};

CapabilitiesBase::Private::Private(bool specificToContact)
    : specificToContact(specificToContact)
{
}

CapabilitiesBase::Private::Private(const RequestableChannelClassList &classes,
        bool specificToContact)
    : classes(classes),
      specificToContact(specificToContact)
{
}

/**
 * \class CapabilitiesBase
 * \ingroup clientconn
 * \headerfile TelepathyQt4/capabilities-base.h <TelepathyQt4/CapabilitiesBase>
 *
 * \brief The CapabilitiesBase class provides an object representing the
 * capabilities a Connection or a Contact supports.
 */

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
 * Construct a new CapabilitiesBase object using the given \a classes.
 *
 * \param classes RequestableChannelClassList representing the capabilities of a
 *                connection or contact.
 * \param specificToContact Whether this object describes the capabilities of a
 *                          particular contact.
 */
CapabilitiesBase::CapabilitiesBase(const RequestableChannelClassList &classes,
        bool specificToContact)
    : mPriv(new Private(classes, specificToContact))
{
}

/**
 * Class destructor.
 */
CapabilitiesBase::~CapabilitiesBase()
{
    delete mPriv;
}

/**
 * Return the underlying data structure used by Telepathy to represent
 * the requests that can succeed.
 *
 * This can be used by advanced clients to determine whether an unusually
 * complex request would succeed. See the Telepathy D-Bus API Specification
 * for details of how to interpret the returned list of QVariantMap objects.
 *
 * The higher-level methods like textChats() are likely to be more
 * useful to the majority of clients.
 *
 * \return A RequestableChannelClassList indicating the parameters to
 *         Account::createChannel, Account::ensureChannel,
 *         Connection::createChannel and Connection::ensureChannel
 *         that can be expected to work.
 */
RequestableChannelClassList CapabilitiesBase::requestableChannelClasses() const
{
    return mPriv->classes;
}

void CapabilitiesBase::updateRequestableChannelClasses(
        const RequestableChannelClassList &classes)
{
    mPriv->classes = classes;
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
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &cls, mPriv->classes) {
        if (!cls.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT) &&
            targetHandleType == HandleTypeContact) {
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
 * \return \c true if Account::ensureMediaCall() can be expected to work,
 *         \c false otherwise.
 * \sa audioCalls(), videoCalls()
 */
bool CapabilitiesBase::mediaCalls() const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &cls, mPriv->classes) {
        if (!cls.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) &&
            targetHandleType == HandleTypeContact) {
            return true;
        }
    }
    return false;
}

/**
 * Return whether private audio calls can be established by providing a
 * contact identifier.
 *
 * Call UpgradingCalls() to determine whether such calls are
 * likely to be upgradable to have a video stream later.
 *
 * If the protocol is such that these calls can be established, but only via
 * a more elaborate D-Bus API than normal (because more information is needed),
 * then this method will return false.
 *
 * In some older connection managers, audioCalls() and videoCalls() might both return
 * false, even though mediaCalls() returns true. This indicates that only an older
 * API is supported - clients of these connection managers must call
 * Account::ensureMediaCall() to get an empty call, then add audio and/or
 * video streams to it.
 *
 * \return \c true if Account::ensureAudioCall() can be expected to work,
 *         \c false otherwise.
 * \sa mediaCalls(), videoCalls()
 */
bool CapabilitiesBase::audioCalls() const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &cls, mPriv->classes) {
        if (!cls.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) &&
            targetHandleType == HandleTypeContact &&
            cls.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio"))) {
                return true;
        }
    }
    return false;
}

/**
 * Return whether private video calls can be established by providing a
 * contact identifier.
 *
 * The same comments as for audioCalls() apply to this method.
 *
 * \param withAudio If true (the default), check for the ability to make calls
 *                  with both audio and video; if false, do not require the
 *                  ability to include an audio stream.
 * \return \c true if Account::ensureVideoCall() can be expected to work,
 *         if given the same \a withAudio parameter, \c false otherwise
 * \sa mediaCalls(), audioCalls()
 */
bool CapabilitiesBase::videoCalls(bool withAudio) const
{
    QString channelType;
    uint targetHandleType;
    foreach (const RequestableChannelClass &cls, mPriv->classes) {
        if (!cls.fixedProperties.size() == 2) {
            continue;
        }

        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        targetHandleType = qdbus_cast<uint>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) &&
            targetHandleType == HandleTypeContact &&
            cls.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialVideo")) &&
            (!withAudio || cls.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".InitialAudio")))) {
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
 * beginning, for instance with ensureVideoCall(). This is indicated by
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
bool CapabilitiesBase::upgradingCalls() const
{
    QString channelType;
    foreach (const RequestableChannelClass &cls, mPriv->classes) {
        channelType = qdbus_cast<QString>(cls.fixedProperties.value(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")));
        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA) &&
            !cls.allowedProperties.contains(
                QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA ".ImmutableStreams"))) {
                // TODO should we test all classes that have channelType
                //      StreamedMedia or just one is fine?
                return true;
        }
    }
    return false;
}

/**
 * \deprecated Use textChats instead.
 */
bool CapabilitiesBase::supportsTextChats() const
{
    return textChats();
}

/**
 * \deprecated Use mediaCalls instead.
 */
bool CapabilitiesBase::supportsMediaCalls() const
{
    return mediaCalls();
}

/**
 * \deprecated Use audioCalls instead.
 */
bool CapabilitiesBase::supportsAudioCalls() const
{
    return audioCalls();
}

/**
 * \deprecated Use videoCalls() instead.
 */
bool CapabilitiesBase::supportsVideoCalls(bool withAudio) const
{
    return videoCalls(withAudio);
}

/**
 * \deprecated Use upgradingCalls() instead.
 */
bool CapabilitiesBase::supportsUpgradingCalls() const
{
    return upgradingCalls();
}

} // Tp
