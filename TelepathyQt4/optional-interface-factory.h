/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt4_optional_interface_factory_h_HEADER_GUARD_
#define _TelepathyQt4_optional_interface_factory_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace Tp
{

class AbstractInterface;

class OptionalInterfaceCache
{
    Q_DISABLE_COPY(OptionalInterfaceCache)

public:
    explicit OptionalInterfaceCache(QObject *proxy);

    ~OptionalInterfaceCache();

protected:
    AbstractInterface *getCached(const QString &name) const;
    void cache(AbstractInterface *interface) const;
    QObject *proxy() const;

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

template <typename DBusProxySubclass> class OptionalInterfaceFactory
    : private OptionalInterfaceCache
{
    Q_DISABLE_COPY(OptionalInterfaceFactory)

public:
    enum InterfaceSupportedChecking
    {
        CheckInterfaceSupported,
        BypassInterfaceCheck
    };

    inline OptionalInterfaceFactory(DBusProxySubclass *this_)
        : OptionalInterfaceCache(this_)
    {
    }

    inline ~OptionalInterfaceFactory()
    {
    }

    inline QStringList interfaces() const { return mInterfaces; }

    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        QString name(Interface::staticInterfaceName());
        if (check == CheckInterfaceSupported && !mInterfaces.contains(name)) {
            return 0;
        }

        // If present or forced, delegate to OptionalInterfaceFactory
        return interface<Interface>();
    }

    template <typename Interface>
    inline Interface *interface() const
    {
        AbstractInterface* interfaceMustBeASubclassOfAbstractInterface = static_cast<Interface *>(NULL);
        Q_UNUSED(interfaceMustBeASubclassOfAbstractInterface);

        // If there is a interface cached already, return it
        QString name(Interface::staticInterfaceName());
        AbstractInterface *cached = getCached(name);
        if (cached)
            return static_cast<Interface *>(cached);

        // Otherwise, cache and return a newly constructed proxy
        Interface *interface = new Interface(
                static_cast<DBusProxySubclass *>(proxy()));
        cache(interface);
        return interface;
    }

protected:
    inline void setInterfaces(const QStringList &interfaces)
    {
        mInterfaces = interfaces;
    }

private:
    QStringList mInterfaces;
};

} // Tp

#endif
