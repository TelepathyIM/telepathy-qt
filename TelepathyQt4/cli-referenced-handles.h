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

#ifndef _TelepathyQt4_cli_referenced_handles_h_HEADER_GUARD_
#define _TelepathyQt4_cli_referenced_handles_h_HEADER_GUARD_

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
 * \defgroup clientconn Connection proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing remote Telepathy Connections and their optional
 * interfaces.
 */

namespace Telepathy
{
namespace Client
{
class ReferencedHandles;
}
}

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Connection>

#include <QSharedDataPointer>
#include <QList>

namespace Telepathy
{
namespace Client
{

/**
 * \class ReferencedHandles
 * \ingroup clientconn
 * \headerfile <TelepathyQt4/cli-referenced-handles.h> <TelepathyQt4/Client/ReferencedHandles>
 *
 * Helper container for safe management of handle lifetimes. Every handle in a
 * ReferencedHandles container is guaranteed to be valid (and stay valid, as
 * long it's in at least one ReferencedHandles container).
 *
 * The class offers a QList-style API. However, from the mutable operations,
 * only the operations for which the validity guarantees can be preserved are
 * provided. This means no functions which can add an arbitrary handle to the
 * container are included - the only way to add handles to the container is to
 * reference them using Connection::referenceHandles() and appending the
 * resulting ReferenceHandles instance.
 *
 * ReferencedHandles is a implicitly shared class.
 */
class ReferencedHandles
{
    public:
        typedef UIntList::const_iterator const_iterator;
        typedef UIntList::ConstIterator ConstIterator;
        typedef UIntList::const_pointer const_pointer;
        typedef UIntList::const_reference const_reference;
        typedef UIntList::difference_type difference_type;
        typedef UIntList::pointer pointer;
        typedef UIntList::reference reference;
        typedef UIntList::size_type size_type;
        typedef UIntList::value_type value_type;

        ReferencedHandles();
        ReferencedHandles(const ReferencedHandles& other);
        ~ReferencedHandles();

        Connection* connection() const;
        uint handleType() const;

        uint at(int i) const;
        uint back() const;
        uint first() const;
        uint front() const;
        uint last() const;
        uint value(int i) const;
        uint value(int i, uint defaultValue) const;

        const_iterator begin() const;
        const_iterator constBegin() const;
        const_iterator constEnd() const;
        const_iterator end() const;

        bool contains(uint handle) const;
        int count(uint value) const;
        int count() const;
        bool empty() const;
        bool endsWith(uint handle) const;
        int indexOf(uint handle, int from = 0) const;
        bool isEmpty() const;
        int lastIndexOf(uint handle, int from = -1) const;
        int length() const;
        ReferencedHandles mid(int pos, int from = -1) const;
        int size() const;
        bool startsWith(uint handle) const;

        void append(const ReferencedHandles& another);
        void clear();
        void move(int from, int to);
        void pop_back();
        void pop_front();
        int removeAll(uint handle);
        void removeAt(int i);
        void removeFirst();
        void removeLast();
        bool removeOne(uint handle);
        void replace(int i, uint handle);
        void swap(int i, int j);
        uint takeAt(int i);
        uint takeFirst();
        uint takeLast();

        bool operator!=(const ReferencedHandles& another) const;
        bool operator!=(const UIntList& another) const;
        ReferencedHandles operator+(const ReferencedHandles& another);
        ReferencedHandles& operator+=(const ReferencedHandles& another);
        ReferencedHandles& operator<<(const ReferencedHandles& another);
        ReferencedHandles& operator=(const ReferencedHandles& another);
        bool operator==(const ReferencedHandles& another) const;
        bool operator==(const UIntList& another) const;
        uint operator[](int i) const;

    private:
        // For access to the "prime" constructor
        friend class PendingHandles;

        ReferencedHandles(Connection* connection, uint handleType, const UIntList& handles);

        struct Private;
        QSharedDataPointer<Private> mPriv;
};

typedef QListIterator<uint> ReferencedHandlesIterator;

}
}

#endif
