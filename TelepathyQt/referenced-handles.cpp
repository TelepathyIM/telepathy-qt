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

#include <TelepathyQt/ReferencedHandles>

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>

#include <QSharedData>

namespace Tp
{

struct TP_QT_NO_EXPORT ReferencedHandles::Private : public QSharedData
{
    WeakPtr<Connection> connection;
    HandleType handleType;
    UIntList handles;

    Private()
    {
        handleType = HandleTypeNone;
    }

    Private(const ConnectionPtr &conn, HandleType handleType,
            const UIntList &handles)
        : connection(conn), handleType(handleType), handles(handles)
    {
        Q_ASSERT(!conn.isNull());
        Q_ASSERT(handleType != 0);

        foreach (uint handle, handles) {
            conn->refHandle(handleType, handle);
        }
    }

    Private(const Private &a)
        : QSharedData(a),
          connection(a.connection),
          handleType(a.handleType),
          handles(a.handles)
    {
        if (!handles.isEmpty()) {
            ConnectionPtr conn(connection);
            if (!conn) {
                debug() << "  Destroyed after Connection, so the Connection "
                    "has already released the handles";
                return;
            }

            for (const_iterator i = handles.constBegin(); i != handles.constEnd(); ++i) {
                conn->refHandle(handleType, *i);
            }
        }
    }

    ~Private()
    {
        if (!handles.isEmpty()) {
            ConnectionPtr conn(connection);
            if (!conn) {
                debug() << "  Destroyed after Connection, so the Connection "
                    "has already released the handles";
                return;
            }

            for (const_iterator i = handles.constBegin(); i != handles.constEnd(); ++i) {
                conn->unrefHandle(handleType, *i);
            }
        }
    }

private:
    void operator=(const Private&);
};

/**
 * \class ReferencedHandles
 * \ingroup clientconn
 * \headerfile TelepathyQt/referenced-handles.h <TelepathyQt/ReferencedHandles>
 *
 * \brief Helper container for safe management of handle lifetimes. Every handle
 * in a ReferencedHandles container is guaranteed to be valid (and stay valid,
 * as long it's in at least one ReferencedHandles container).
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

ReferencedHandles::ReferencedHandles()
    : mPriv(new Private)
{
}

ReferencedHandles::ReferencedHandles(const ReferencedHandles &other)
    : mPriv(other.mPriv)
{
}

ReferencedHandles::~ReferencedHandles()
{
}

ConnectionPtr ReferencedHandles::connection() const
{
    return ConnectionPtr(mPriv->connection);
}

HandleType ReferencedHandles::handleType() const
{
    return mPriv->handleType;
}

uint ReferencedHandles::at(int i) const
{
    return mPriv->handles[i];
}

uint ReferencedHandles::value(int i, uint defaultValue) const
{
    return mPriv->handles.value(i, defaultValue);
}

ReferencedHandles::const_iterator ReferencedHandles::begin() const
{
    return mPriv->handles.begin();
}

ReferencedHandles::const_iterator ReferencedHandles::end() const
{
    return mPriv->handles.end();
}

bool ReferencedHandles::contains(uint handle) const
{
    return mPriv->handles.contains(handle);
}

int ReferencedHandles::count(uint handle) const
{
    return mPriv->handles.count(handle);
}

int ReferencedHandles::indexOf(uint handle, int from) const
{
    return mPriv->handles.indexOf(handle, from);
}

bool ReferencedHandles::isEmpty() const
{
    return mPriv->handles.isEmpty();
}

int ReferencedHandles::lastIndexOf(uint handle, int from) const
{
    return mPriv->handles.lastIndexOf(handle, from);
}

ReferencedHandles ReferencedHandles::mid(int pos, int length) const
{
    return ReferencedHandles(connection(), handleType(),
            mPriv->handles.mid(pos, length));
}

int ReferencedHandles::size() const
{
    return mPriv->handles.size();
}

void ReferencedHandles::clear()
{
    if (!mPriv->handles.empty()) {
        ConnectionPtr conn(mPriv->connection);
        if (conn) {
            foreach (uint handle, mPriv->handles) {
                conn->unrefHandle(handleType(), handle);
            }
        } else {
            warning() << "Connection already destroyed in "
                "ReferencedHandles::clear() so can't unref!";
        }
    }

    mPriv->handles.clear();
}

void ReferencedHandles::move(int from, int to)
{
    mPriv->handles.move(from, to);
}

int ReferencedHandles::removeAll(uint handle)
{
    int count = mPriv->handles.removeAll(handle);

    if (count > 0) {
        ConnectionPtr conn(mPriv->connection);
        if (conn) {
            for (int i = 0; i < count; ++i) {
                conn->unrefHandle(handleType(), handle);
            }
        } else {
            warning() << "Connection already destroyed in "
                "ReferencedHandles::removeAll() with handle ==" <<
                handle << "so can't unref!";
        }
    }

    return count;
}

void ReferencedHandles::removeAt(int i)
{
    ConnectionPtr conn(mPriv->connection);
    if (conn) {
        conn->unrefHandle(handleType(), at(i));
    } else {
        warning() << "Connection already destroyed in "
            "ReferencedHandles::removeAt() with i ==" <<
            i << "so can't unref!";
    }

    mPriv->handles.removeAt(i);
}

bool ReferencedHandles::removeOne(uint handle)
{
    bool wasThere = mPriv->handles.removeOne(handle);

    if (wasThere) {
        ConnectionPtr conn(mPriv->connection);
        if (conn) {
            conn->unrefHandle(handleType(), handle);
        } else {
            warning() << "Connection already destroyed in "
                "ReferencedHandles::removeOne() with handle ==" <<
                handle << "so can't unref!";
        }
    }

    return wasThere;
}

void ReferencedHandles::swap(int i, int j)
{
    mPriv->handles.swap(i, j);
}

uint ReferencedHandles::takeAt(int i)
{
    ConnectionPtr conn(mPriv->connection);
    if (conn) {
        conn->unrefHandle(handleType(), at(i));
    } else {
        warning() << "Connection already destroyed in "
            "ReferencedHandles::takeAt() with i ==" <<
            i << "so can't unref!";
    }

    return mPriv->handles.takeAt(i);
}

ReferencedHandles ReferencedHandles::operator+(const ReferencedHandles &another) const
{
    if (connection() != another.connection() ||
        handleType() != another.handleType()) {
        warning() << "Tried to concatenate ReferencedHandles instances "
            "with different connection and/or handle type";
        return *this;
    }

    return ReferencedHandles(connection(), handleType(),
            mPriv->handles + another.mPriv->handles);
}

ReferencedHandles &ReferencedHandles::operator=(
        const ReferencedHandles &another)
{
    mPriv = another.mPriv;
    return *this;
}

bool ReferencedHandles::operator==(const ReferencedHandles &another) const
{
    return connection() == another.connection() &&
        handleType() == another.handleType() &&
        mPriv->handles == another.mPriv->handles;
}

bool ReferencedHandles::operator==(const UIntList &list) const
{
    return mPriv->handles == list;
}

UIntList ReferencedHandles::toList() const
{
    return mPriv->handles;
}

ReferencedHandles::ReferencedHandles(const ConnectionPtr &connection,
        HandleType handleType, const UIntList &handles)
    : mPriv(new Private(connection, handleType, handles))
{
}

} // Tp
