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

#include "cli-referenced-handles.h"

#include <QPointer>
#include <QSharedData>

#include "debug-internal.hpp"

namespace Telepathy
{
namespace Client
{

struct ReferencedHandles::Private : public QSharedData
{
    QPointer<Connection> connection;
    uint handleType;
    UIntList handles;

    Private()
    {
        debug() << "ReferencedHandles::Private(default)";

        handleType = 0;
    }

    Private(const Private& a)
        : connection(a.connection),
          handleType(a.handleType),
          handles(a.handles)
    {
        debug() << "ReferencedHandles::Private(copy)";

        if (!handles.isEmpty()) {
            if (!connection) {
                warning() << "ReferencedHandles with" << handles.size() << "handles detached with connection destroyed so can't reference";
                return;
            }

            for (const_iterator i = handles.begin();
                                i != handles.end();
                                ++i)
                connection->refHandle(handleType, *i);
        }
    }

    ~Private()
    {
        debug() << "~ReferencedHandles::Private()";

        if (!handles.isEmpty()) {
            if (!connection) {
                warning() << "ReferencedHandles with last copy of" << handles.size() << "handles destroyed with connection destroyed so can't unreference";
                return;
            }

            for (const_iterator i = handles.begin();
                                i != handles.end();
                                ++i)
                connection->unrefHandle(handleType, *i);
        }
    }

private:
    void operator=(const Private&);
};

ReferencedHandles::ReferencedHandles()
    : mPriv(new Private)
{
    debug() << "ReferencedHandles(default)";
}

ReferencedHandles::ReferencedHandles(const ReferencedHandles& other)
    : mPriv(other.mPriv)
{
    debug() << "ReferencedHandles(copy)";
}

ReferencedHandles::~ReferencedHandles()
{
    debug() << "~ReferencedHandles()";
}

Connection* ReferencedHandles::connection() const
{
    return mPriv->connection;
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
    return ReferencedHandles(connection(), handleType(), mPriv->handles.mid(pos, length));
}

int ReferencedHandles::size() const
{
    return mPriv->handles.size();
}

void ReferencedHandles::clear()
{
    mPriv->handles.clear();
}

void ReferencedHandles::move(int from, int to)
{
    mPriv->handles.move(from, to);
}

int ReferencedHandles::removeAll(uint handle)
{
    return mPriv->handles.removeAll(handle);
}

void ReferencedHandles::removeAt(int i)
{
    mPriv->handles.removeAt(i);
}

bool ReferencedHandles::removeOne(uint handle)
{
    return mPriv->handles.removeOne(handle);
}

void ReferencedHandles::swap(int i, int j)
{
    mPriv->handles.swap(i, j);
}

uint ReferencedHandles::takeAt(int i)
{
    return mPriv->handles.takeAt(i);
}

ReferencedHandles ReferencedHandles::operator+(const ReferencedHandles& another) const
{
    if (connection() != another.connection() || handleType() != another.handleType()) {
        warning() << "Tried to concatenate ReferencedHandles instances with different connection and/or handle type";
        return *this;
    }

    return ReferencedHandles(connection(), handleType(), mPriv->handles + another.mPriv->handles);
}

ReferencedHandles& ReferencedHandles::operator=(const ReferencedHandles& another)
{
    mPriv = another.mPriv;
    return *this;
}

bool ReferencedHandles::operator==(const ReferencedHandles& another) const
{
    return connection() == another.connection()
        && handleType() == another.handleType()
        && mPriv->handles == another.mPriv->handles;
}

bool ReferencedHandles::operator==(const UIntList& list) const
{
    return mPriv->handles == list;
}

ReferencedHandles::ReferencedHandles(Connection* connection, uint handleType, const UIntList& handles)
{
    debug() << "ReferencedHandles(prime)";

    Q_ASSERT(connection != NULL);
    Q_ASSERT(handleType != 0);

    mPriv->connection = connection;
    mPriv->handleType = handleType;
    mPriv->handles = handles;

    for (const_iterator i = handles.begin();
                        i != handles.end();
                        ++i)
        connection->refHandle(handleType, *i);
}

}
}
