/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2009 Nokia Corporation
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

#ifndef _TelepathyQt_optional_interface_factory_h_HEADER_GUARD_
#define _TelepathyQt_optional_interface_factory_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QObject>
#include <QStringList>
#include <QtGlobal>

namespace Tp
{

class AbstractInterface;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

class TP_QT_EXPORT OptionalInterfaceCache
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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

template <typename DBusProxySubclass> class OptionalInterfaceFactory
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    : private OptionalInterfaceCache
#endif
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

    inline bool hasInterface(const QString &name) const
    {
        return mInterfaces.contains(name);
    }

    template <class Interface>
    inline Interface *optionalInterface(
            InterfaceSupportedChecking check = CheckInterfaceSupported) const
    {
        // Check for the remote object supporting the interface
        // Note that extra whitespace on "name" declaration is significant to avoid
        // vexing-parse
        QString name( (QLatin1String(Interface::staticInterfaceName())) );
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
        // Note that extra whitespace on "name" declaration is significant to avoid
        // vexing-parse
        QString name( (QLatin1String(Interface::staticInterfaceName())) );
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
