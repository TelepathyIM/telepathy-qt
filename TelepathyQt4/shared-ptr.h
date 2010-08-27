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

#include <TelepathyQt4/Global>

#include <QObject>

namespace Tp
{

class RefCounted;
class WeakData;
template <class T> class SharedPtr;
template <class T> class WeakPtr;

class TELEPATHY_QT4_EXPORT WeakData
{
    Q_DISABLE_COPY(WeakData)

public:
    WeakData(RefCounted *d) : d(d), weakref(0) { }

    RefCounted *d;
    mutable QAtomicInt weakref;
};

class TELEPATHY_QT4_EXPORT RefCounted
{
    Q_DISABLE_COPY(RefCounted)

public:
    inline RefCounted() : strongref(0), wd(0) { }
    inline virtual ~RefCounted() { if (wd) { wd->d = 0; } }

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
    explicit inline SharedPtr(T *d) : d(d) { if (d) { d->ref(); } }
    inline SharedPtr(const SharedPtr<T> &o) : d(o.d) { if (d) { d->ref(); } }
    explicit inline SharedPtr(const WeakPtr<T> &o)
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
        SharedPtr<T>().swap(*this);
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
        SharedPtr<T>(o).swap(*this);
        return *this;
    }

    inline void swap(SharedPtr<T> &o)
    {
        T *tmp = d;
        d = o.d;
        o.d = tmp;
    }

    template <class X>
    static inline SharedPtr<T> staticCast(const SharedPtr<X> &src)
    {
        return SharedPtr<T>(static_cast<T*>(src.data()));
    }

    template <class X>
    static inline SharedPtr<T> dynamicCast(const SharedPtr<X> &src)
    {
        return SharedPtr<T>(dynamic_cast<T*>(src.data()));
    }

    template <class X>
    static inline SharedPtr<T> constCast(const SharedPtr<X> &src)
    {
        return SharedPtr<T>(const_cast<T*>(src.data()));
    }

    template <class X>
    static inline SharedPtr<T> qObjectCast(const SharedPtr<X> &src)
    {
        return SharedPtr<T>(qobject_cast<T*>(src.data()));
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
        if (wd && !wd->weakref.deref()) {
            if (wd->d) {
                wd->d->wd = 0;
            }
            delete wd;
        }
    }

    inline bool isNull() const { return !wd || !wd->d || wd->d->strongref == 0; }
    inline operator bool() const { return !isNull(); }
    inline bool operator!() const { return isNull(); }

    inline WeakPtr<T> &operator=(const WeakPtr<T> &o)
    {
        WeakPtr<T>(o).swap(*this);
        return *this;
    }

    inline WeakPtr<T> &operator=(const SharedPtr<T> &o)
    {
        WeakPtr<T>(o).swap(*this);
        return *this;
    }

    inline void swap(WeakPtr<T> &o)
    {
        WeakData *tmp = wd;
        wd = o.wd;
        o.wd = tmp;
    }

    SharedPtr<T> toStrongRef() const { return SharedPtr<T>(*this); }

private:
    friend class SharedPtr<T>;

    WeakData *wd;
};

} // Tp

#endif
