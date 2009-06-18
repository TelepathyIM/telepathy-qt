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

#include <TelepathyQt4/ReferencedHandles>

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>

#include <QPointer>
#include <QSharedData>

namespace Tp
{

struct ReferencedHandles::Private : public QSharedData
{
    WeakPtr<Connection> connection;
    uint handleType;
    UIntList handles;

    Private()
    {
        handleType = 0;
    }

    Private(const ConnectionPtr &connection, uint handleType,
            const UIntList &handles)
        : connection(connection), handleType(handleType), handles(handles)
    {
        Q_ASSERT(connection);
        Q_ASSERT(handleType != 0);

        foreach (uint handle, handles) {
            connection->refHandle(handleType, handle);
        }
    }

    Private(const Private &a)
        : QSharedData(a),
          connection(a.connection),
          handleType(a.handleType),
          handles(a.handles)
    {
        if (!handles.isEmpty()) {
            if (!connection) {
                debug() << "  Destroyed after Connection, so the Connection "
                    "has already released the handles";
                return;
            }

            ConnectionPtr conn(connection);
            for (const_iterator i = handles.begin(); i != handles.end(); ++i) {
                conn->refHandle(handleType, *i);
            }
        }
    }

    ~Private()
    {
        if (!handles.isEmpty()) {
            if (!connection) {
                debug() << "  Destroyed after Connection, so the Connection "
                    "has already released the handles";
                return;
            }

            ConnectionPtr conn(connection);
            for (const_iterator i = handles.begin(); i != handles.end(); ++i) {
                conn->unrefHandle(handleType, *i);
            }
        }
    }

private:
    void operator=(const Private&);
};

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

uint ReferencedHandles::handleType() const
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
        if (mPriv->connection) {
            ConnectionPtr conn(mPriv->connection);
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
        if (mPriv->connection) {
            ConnectionPtr conn(mPriv->connection);
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
    if (mPriv->connection) {
        ConnectionPtr conn(mPriv->connection);
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
        if (mPriv->connection) {
            ConnectionPtr conn(mPriv->connection);
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
    if (mPriv->connection) {
        ConnectionPtr conn(mPriv->connection);
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
        uint handleType, const UIntList &handles)
    : mPriv(new Private(connection, handleType, handles))
{
}

} // Tp
