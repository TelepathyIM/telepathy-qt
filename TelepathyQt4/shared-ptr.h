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

#ifndef _TelepathyQt4_shared_ptr_h_HEADER_GUARD_
#define _TelepathyQt4_shared_ptr_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <QObject>

namespace Telepathy
{

class SharedData;
class WeakData;
template <class T> class SharedPtr;
template <class T> class WeakPtr;

class WeakData
{
    Q_DISABLE_COPY(WeakData)

public:
    WeakData(SharedData *d) : d(d), weakref(0) { }

    SharedData *d;
    mutable QAtomicInt weakref;
};

class SharedData
{
    Q_DISABLE_COPY(SharedData)

public:
    inline SharedData() : strongref(0), wd(0) { }
    inline ~SharedData() { if (wd) { wd->d = 0; } }

    inline void ref() { strongref.ref(); }
    inline bool deref() { return strongref.deref(); }

    mutable QAtomicInt strongref;
    WeakData *wd;
};

template <class T>
class SharedPtr
{
public:
    inline SharedPtr() : d(0) { }
    inline SharedPtr(T *d) : d(d) { if (d) { d->ref(); } }
    inline SharedPtr(const SharedPtr<T> &o) : d(o.d) { if (d) { d->ref(); } }
    template<class X>
    inline SharedPtr(const SharedPtr<X> &o) : d(static_cast<T *>(o.data())) { if (d) { d->ref(); } }
    inline SharedPtr(const WeakPtr<T> &o)
    {
        if (o.wd && o.wd->d) {
            d = static_cast<T*>(o.wd->d);
            d->ref();
        } else {
            d = 0;
        }
    }
    inline ~SharedPtr()
    {
        if (d && !d->deref()) {
            delete d;
        }
    }

    inline void reset()
    {
        if (d && !d->deref()) {
            delete d;
        }
        d = 0;
    }

    inline T *data() const { return d; }
    inline const T *constData() const { return d; }
    inline T *operator->() { return d; }
    inline T *operator->() const { return d; }

    inline bool isNull() const { return !d; }
    inline operator bool() const { return !isNull(); }
    inline bool operator!() const { return isNull(); }

    inline bool operator==(const SharedPtr<T> &o) const { return d == o.d; }
    inline bool operator!=(const SharedPtr<T> &o) const { return d != o.d; }
    inline bool operator==(const T *ptr) const { return d == ptr; }
    inline bool operator!=(const T *ptr) const { return d != ptr; }

    inline SharedPtr<T> &operator=(const SharedPtr<T> &o)
    {
        if (o.d != d) {
            if (o.d) {
                o.d->ref();
            }
            if (d && !d->deref()) {
                delete d;
            }
            d = o.d;
        }
        return *this;
    }

    inline SharedPtr<T> &operator=(const WeakPtr<T> &o)
    {
        if (o.wd) {
            if (o.wd->d != d) {
                if (d && !d->deref()) {
                    delete d;
                }
                if (o.wd->d) {
                    o.wd->d->ref();
                }
                d = static_cast<T*>(o.wd->d);
            }
        } else {
            d = 0;
        }
        return *this;
    }


private:
    friend class WeakPtr<T>;

    T *d;
};

template <class T>
class WeakPtr
{
public:
    inline WeakPtr() : wd(0) { }
    inline WeakPtr(const WeakPtr<T> &o) : wd(o.wd) { if (wd) { wd->weakref.ref(); } }
    inline WeakPtr(const SharedPtr<T> &o)
    {
        if (o.d) {
            if (!o.d->wd) {
                o.d->wd = new WeakData(o.d);
            }
            wd = o.d->wd;
            wd->weakref.ref();
        } else {
            wd = 0;
        }
    }
    inline ~WeakPtr()
    {
        deref();
    }

    inline bool isNull() const { return !wd || !wd->d || wd->d->strongref == 0; }
    inline operator bool() const { return !isNull(); }
    inline bool operator!() const { return isNull(); }

    inline WeakPtr<T> &operator=(const WeakPtr<T> &o)
    {
        if (o.wd != wd) {
            if (o.wd) {
                o.wd->weakref.ref();
            }
            deref();
            wd = o.wd;
        }
        return *this;
    }

    inline WeakPtr<T> &operator=(const SharedPtr<T> &o)
    {
        if (o.d) {
            if (o.d->wd != wd || o.d->wd == 0) {
                if (!o.d->wd) {
                    o.d->wd = new WeakData(o.d);
                }
                o.d->wd->weakref.ref();
                deref();
                wd = o.d->wd;
            }
        } else {
            deref();
            wd = 0;
        }
        return *this;
    }

private:
    friend class SharedPtr<T>;

    inline void deref()
    {
        if (wd && !wd->weakref.deref()) {
            if (wd->d) {
                wd->d->wd = 0;
            }
            delete wd;
        }
    }

    WeakData *wd;
};

} // Telepathy

#endif
