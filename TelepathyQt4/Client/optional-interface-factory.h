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

#ifndef _TelepathyQt4_cli_optional_interface_factory_h_HEADER_GUARD_
#define _TelepathyQt4_cli_optional_interface_factory_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

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

#include <QtGlobal>

#include <TelepathyQt4/Client/AbstractInterface>

namespace Telepathy
{
namespace Client
{

/**
 * \class OptionalInterfaceFactory
 * \ingroup clientsideproxies
 * \headerfile <TelepathyQt4/Client/optional-interface-factory.h> <TelepathyQt4/Client/OptionalInterfaceFactory>
 *
 * Implementation helper class for high-level proxy classes willing to offer
 * access to shared instances of interface proxies for optional interfaces.
 *
 * This class is included in the public API for the benefit of high-level
 * proxies in extensions.
 */
class OptionalInterfaceFactory
{
    public:
        /**
         * Class constructor.
         */
        OptionalInterfaceFactory();

        /**
         * Class destructor.
         *
         * Frees all interface instances constructed by this factory.
         */
        ~OptionalInterfaceFactory();

        /**
         * Return a pointer to a valid instance of a interface class, associated
         * with the same remote object as the given main interface instance. The
         * given main interface must be of the class the optional interface is
         * generated for (for eg. ChannelInterfaceGroupInterface this means
         * ChannelInterface) or a subclass.
         *
         * First invocation of this method for a particular optional interface
         * class will construct the instance; subsequent calls will return a
         * pointer to the same instance.
         *
         * The returned instance is freed when the factory is destroyed; using
         * it after destroying the factory will likely produce a crash. As the
         * instance is shared, it should not be freed directly.
         *
         * \tparam OptionalInterface Class of the interface instance to get.
         * \tparam MainInterface Class of the main interface.
         * \param mainInterface Main interface instance to use.
         * \return A pointer to an optional interface instance.
         */
        template <typename OptionalInterface, typename MainInterface>
        inline OptionalInterface* interface(const MainInterface& mainInterface) const
        {
            // Check that the types given are both subclasses of AbstractInterface
            AbstractInterface* mainInterfaceMustBeASubclassOfAbstractInterface = static_cast<MainInterface*>(NULL);
            AbstractInterface* optionalInterfaceMustBeASubclassOfAbstractInterface = static_cast<OptionalInterface*>(NULL);
            Q_UNUSED(mainInterfaceMustBeASubclassOfAbstractInterface);
            Q_UNUSED(optionalInterfaceMustBeASubclassOfAbstractInterface);

            // If there is a interface cached already, return it
            QString name(OptionalInterface::staticInterfaceName());
            AbstractInterface* cached = getCached(name);
            if (cached)
                return static_cast<OptionalInterface*>(cached);

            // Otherwise, cache and return a newly constructed proxy
            OptionalInterface* interface = new OptionalInterface(mainInterface, 0);
            cache(interface);
            return interface;
        }

    private:
        AbstractInterface *getCached(const QString &name) const;
        void cache(AbstractInterface *interface) const;

        struct Private;
        Private* mPriv;
};

}
}

#endif
