/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_Client_Channel_HEADER_GUARD_
#define _TelepathyQt4_Client_Channel_HEADER_GUARD_

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientchannel Channel proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Channels and their optional
 * interfaces.
 */

#include <TelepathyQt4/_gen/cli-channel.h>

#include <QSet>

#include <QDBusPendingCallWatcher>

#include <TelepathyQt4/Client/DBus>
#include <TelepathyQt4/Client/OptionalInterfaceFactory>

namespace Telepathy
{
namespace Client
{

/**
 * \class Channel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/cli-channel.h <TelepathyQt4/Client/Channel>
 *
 * High-level proxy object for accessing remote %Telepathy %Channel objects.
 *
 * It adds the following features compared to using ChannelInterface directly:
 * <ul>
 *  <li>Life cycle tracking</li>
 *  <li>Getting the channel type, handle type, handle and interfaces automatically</li>
 *  <li>Shared optional interface proxy instances</li>
 * </ul>
 *
 * The remote object state accessor functions on this object (interfaces(),
 * channelType(), handleType(), handle()) don't make any DBus calls; instead,
 * they return values cached from a previous introspection run. The introspection
 * process populates their values in the most efficient way possible based on
 * what the service implements.
 */
class Channel : public ChannelInterface, private OptionalInterfaceFactory
{
    Q_OBJECT
    Q_ENUMS(Readiness)

public:
    /**
     * Describes readiness of the Channel for usage. The readiness depends on
     * the state of the remote object. In suitable states, an asynchronous
     * introspection process is started, and the Channel becomes more ready when
     * that process is completed.
     */
    enum Readiness {
        /**
         * The object has just been created and introspection is still in
         * progress. No functionality is available.
         *
         * The readiness can change to any other state depending on the result
         * of the initial state query to the remote object.
         */
        ReadinessJustCreated = 0,

        /**
         * The remote object is alive and all introspection has been completed.
         * Most functionality is available.
         *
         * The readiness can change to ReadinessDead.
         */
        ReadinessFull = 5,

        /**
         * The remote object has gone into a state where it can no longer be
         * used. No functionality is available.
         *
         * No further readiness changes are possible.
         */
        ReadinessDead = 10,

        _ReadinessInvalid = 0xffff
    };

    /**
     * Creates a Channel associated with the given object on the session bus.
     *
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Channel(const QString& serviceName,
            const QString& objectPath,
            QObject* parent = 0);

    /**
     * Creates a Channel associated with the given object on the given bus.
     *
     * \param connection  The bus via which the object can be reached.
     * \param serviceName Name of the service the object is on.
     * \param objectPath  Path to the object on the service.
     * \param parent      Passed to the parent class constructor.
     */
    Channel(const QDBusConnection& connection,
            const QString &serviceName,
            const QString &objectPath,
            QObject* parent = 0);

    /**
     * Class destructor.
     */
    ~Channel();

    /**
     * Returns the current readiness of the Channel.
     *
     * \return The readiness, as defined in #Readiness.
     */
    Readiness readiness() const;

    /**
     * Returns a list of optional interfaces implemented by the remote object.
     *
     * \return D-Bus names of the implemented optional interfaces.
     */
    QStringList interfaces() const;

    /**
     * Returns the type of this channel.
     *
     * \return D-Bus interface name for the type of the channel.
     */
    QString channelType() const;

    /**
     * Returns the type of the handle returned by #targetHandle().
     *
     * \return The type of the handle, as specified in #HandleType.
     */
    uint targetHandleType() const;

    /**
     * Returns the handle of the remote party with which this channel
     * communicates.
     *
     * \return The handle, which is of the type #targetHandleType() indicates.
     */
    uint targetHandle() const;

    /**
     * \name Optional interface proxy factory
     *
     * Factory functions fabricating proxies for optional %Channel interfaces and
     * interfaces for specific channel types.
     */
    //@{

    /**
     * Returns a pointer to a valid instance of a given %Channel optional
     * interface class, associated with the same remote object the Channel is
     * associated with, and destroyed together with the Channel.
     *
     * If the list returned by interfaces() doesn't contain the name of the
     * interface requested, or the Channel doesn't have readiness
     * #ReadinessFull, <code>0</code> is returned. This check can be bypassed by
     * specifying <code>true</code> for <code>forcePresent</code>, in which case
     * a valid instance is always returned.
     *
     * \see OptionalInterfaceFactory::interface
     *
     * \tparam Interface Class of the optional interface to get.
     * \param forcePresent Should an instance be returned even if it can't be
     *                     determined that the remote object supports the
     *                     requested interface.
     * \return Pointer to an instance of the interface class, or <code>0</code>.
     */
    template <class Interface>
    inline Interface* optionalInterface(bool forcePresent = false) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (!forcePresent && !interfaces().contains(name))
            return 0;

        // If present or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory::interface<Interface>(*this);
    }

    /**
     * Convenience function for getting a CallState interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceCallStateInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceCallStateInterface* callStateInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceCallStateInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a ChatState interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceChatStateInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceChatStateInterface* chatStateInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceChatStateInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a DTMF interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceDTMFInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceDTMFInterface* DTMFInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceDTMFInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a Group interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceGroupInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceGroupInterface* groupInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceGroupInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a Hold interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceHoldInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceHoldInterface* holdInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceHoldInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a MediaSignalling interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfaceMediaSignallingInterface>(forcePresent)</code>
     */
    inline ChannelInterfaceMediaSignallingInterface* mediaSignallingInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfaceMediaSignallingInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a Password interface proxy.
     *
     * \see optionalInterface()
     *
     * \param forcePresent If the interface being present check should be
     *                     bypassed.
     * \return <code>optionalInterface<ChannelInterfacePasswordInterface>(forcePresent)</code>
     */
    inline ChannelInterfacePasswordInterface* passwordInterface(bool forcePresent = false) const
    {
        return optionalInterface<ChannelInterfacePasswordInterface>(forcePresent);
    }

    /**
     * Convenience function for getting a Properties interface proxy. The
     * Properties interface is not necessarily reported by the services, so a
     * <code>forcePresent</code> parameter is not provided, and the interface is
     * always assumed to be present.
     *
     * \see optionalInterface()
     *
     * \return <code>optionalInterface<DBus::PropertiesInterface>(true)</code>
     */
    inline DBus::PropertiesInterface* propertiesInterface() const
    {
        return optionalInterface<DBus::PropertiesInterface>(true);
    }

    /**
     * Returns a pointer to a valid instance of a given %Channel type interface
     * class, associated with the same remote object the Channel is
     * associated with, and destroyed together with the Channel.
     *
     * If the interface name returned by channelType() isn't equivalent to the
     * name of the requested interface, or the Channel doesn't have readiness
     * #ReadinessFull, <code>0</code> is returned. This check can be bypassed by
     * specifying <code>true</code> for <code>forceType</code>, in which case a
     * valid instance is always returned.
     *
     * Convenience functions are provided for well-known channel types. However,
     * there is no convenience getter for TypeContactList because the proxy for
     * that interface doesn't actually have any functionality.
     *
     * \see OptionalInterfaceFactory::interface
     *
     * \tparam Interface Class of the optional interface to get.
     * \param forceType Should an instance be returned even if it can't be
     *                  determined that the remote object is of the requested
     *                  channel type.
     * \return Pointer to an instance of the interface class, or <code>0</code>.
     */
    template <class Interface>
    inline Interface* typeInterface(bool forceType = false) const
    {
        // Check for the remote object having the correct channel type
        QString name(Interface::staticInterfaceName());
        if (!forceType && channelType() != name)
            return 0;

        // If correct type or forced, delegate to OptionalInterfaceFactory
        return OptionalInterfaceFactory::interface<Interface>(*this);
    }

    /**
     * Convenience function for getting a TypeRoomList interface proxy.
     *
     * \see typeInterface()
     *
     * \param forceType If the channel being of the type check should be
     *                  bypassed.
     * \return <code>typeInterface<ChannelTypeRoomListInterface>(forceType)</code>
     */
    inline ChannelTypeRoomListInterface* roomListInterface(bool forceType) const
    {
        return optionalInterface<ChannelTypeRoomListInterface>(forceType);
    }

    /**
     * Convenience function for getting a TypeStreamedMedia interface proxy.
     *
     * \see typeInterface()
     *
     * \param forceType If the channel being of the type check should be
     *                  bypassed.
     * \return <code>typeInterface<ChannelTypeStreamedMediaInterface>(forceType)</code>
     */
    inline ChannelTypeStreamedMediaInterface* streamedMediaInterface(bool forceType) const
    {
        return optionalInterface<ChannelTypeStreamedMediaInterface>(forceType);
    }

    /**
     * Convenience function for getting a TypeText interface proxy.
     *
     * \see typeInterface()
     *
     * \param forceType If the channel being of the type check should be
     *                  bypassed.
     * \return <code>typeInterface<ChannelTypeTextInterface>(forceType)</code>
     */
    inline ChannelTypeTextInterface* textInterface(bool forceType) const
    {
        return optionalInterface<ChannelTypeTextInterface>(forceType);
    }

    /**
     * Convenience function for getting a TypeTubes interface proxy.
     *
     * \see typeInterface()
     *
     * \param forceType If the channel being of the type check should be
     *                  bypassed.
     * \return <code>typeInterface<ChannelTypeTubesInterface>(forceType)</code>
     */
    inline ChannelTypeTubesInterface* tubesInterface(bool forceType) const
    {
        return optionalInterface<ChannelTypeTubesInterface>(forceType);
    }

    //@}

Q_SIGNALS:
    /**
     * Emitted whenever the readiness of the Channel changes.
     *
     * \param newReadiness The new readiness, as defined in #Readiness.
     */
    void readinessChanged(Telepathy::Client::Channel::Readiness newReadiness);

private Q_SLOTS:
    void gotMainProperties(QDBusPendingCallWatcher* watcher);
    void gotChannelType(QDBusPendingCallWatcher* watcher);
    void gotHandle(QDBusPendingCallWatcher* watcher);
    void gotInterfaces(QDBusPendingCallWatcher* watcher);
    void onClosed();

    void gotGroupProperties(QDBusPendingCallWatcher* watcher);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
}

#endif
