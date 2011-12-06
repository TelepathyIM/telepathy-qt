/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2011 Nokia Corporation
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

#ifndef _TelepathyQt_shared_ptr_h_HEADER_GUARD_
#define _TelepathyQt_shared_ptr_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Global>

#include <QHash>
#include <QObject>

namespace Tp
{

class RefCounted;
template <class T> class SharedPtr;
template <class T> class WeakPtr;

class TP_QT_EXPORT RefCounted
{
    Q_DISABLE_COPY(RefCounted)

    class SharedCount
    {
        Q_DISABLE_COPY(SharedCount)

    public:
        SharedCount(RefCounted *d)
            : d(d), strongref(0), weakref(0)
        {
        }

    private:
        template <class T> friend class SharedPtr;
        template <class T> friend class WeakPtr;
        friend class RefCounted;

        RefCounted *d;
        mutable QAtomicInt strongref;
        mutable QAtomicInt weakref;
    };

public:
    inline RefCounted() : sc(new SharedCount(this))
    {
        sc->weakref.ref();
    }

    inline virtual ~RefCounted()
    {
        sc->d = 0;
        if (!sc->weakref.deref()) {
            delete sc;
        }
    }

private:
    template <class T> friend class SharedPtr;
    template <class T> friend class WeakPtr;
    // TODO: Remove when Conn.I.ContactList, etc becomes mandatory. This is required to circumvent
    //       a reference cycle when using contact list channels, due to the fact that Channels hold
    //       strong references to their parent Connection, but not needed when using
    //       Conn.I.ContactList and friends.
    friend class ContactManager;

    inline void ref() const { sc->strongref.ref(); }
    inline bool deref() const { return sc->strongref.deref(); }

    SharedCount *sc;
};

template <class T>
class SharedPtr
{
    typedef bool (SharedPtr<T>::*UnspecifiedBoolType)() const;

public:
    inline SharedPtr() : d(0) { }
    explicit inline SharedPtr(T *d) : d(d) { if (d) { d->ref(); } }
    template <typename Subclass>
        inline SharedPtr(const SharedPtr<Subclass> &o) : d(o.data()) { if (d) { d->ref(); } }
    inline SharedPtr(const SharedPtr<T> &o) : d(o.d) { if (d) { d->ref(); } }
    explicit inline SharedPtr(const WeakPtr<T> &o)
    {
        RefCounted::SharedCount *sc = o.sc;
        if (sc) {
            // increase the strongref, but never up from zero
            // or less (negative is used on untracked objects)
            register int tmp = sc->strongref.fetchAndAddOrdered(0);
            while (tmp > 0) {
                // try to increment from "tmp" to "tmp + 1"
                if (sc->strongref.testAndSetRelaxed(tmp, tmp + 1)) {
                    // succeeded
                    break;
                }
                // failed, try again
                tmp = sc->strongref.fetchAndAddOrdered(0);
            }

            if (tmp > 0) {
                d = dynamic_cast<T*>(sc->d);
                Q_ASSERT(d != NULL);
            } else {
                d = 0;
            }
        } else {
            d = 0;
        }
    }

    inline ~SharedPtr()
    {
        if (d && !d->deref()) {
            T *saved = d;
            d = 0;
            delete saved;
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
    inline bool operator!() const { return isNull(); }
    operator UnspecifiedBoolType() const { return !isNull() ? &SharedPtr<T>::operator! : 0; }

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

template<typename T>
inline uint qHash(const SharedPtr<T> &ptr)
{
    return QT_PREPEND_NAMESPACE(qHash<T>(ptr.data()));
}

template<typename T> inline uint qHash(const WeakPtr<T> &ptr);

template <class T>
class WeakPtr
{
    typedef bool (WeakPtr<T>::*UnspecifiedBoolType)() const;

public:
    inline WeakPtr() : sc(0) { }
    explicit inline WeakPtr(T *d)
    {
        if (d) {
            sc = d->sc;
            sc->weakref.ref();
        } else {
            sc = 0;
        }
    }
    inline WeakPtr(const WeakPtr<T> &o) : sc(o.sc) { if (sc) { sc->weakref.ref(); } }
    inline WeakPtr(const SharedPtr<T> &o)
    {
        if (o.d) {
            sc = o.d->sc;
            sc->weakref.ref();
        } else {
            sc = 0;
        }
    }
    inline ~WeakPtr()
    {
        if (sc && !sc->weakref.deref()) {
            delete sc;
        }
    }

    inline bool isNull() const { return !sc || sc->strongref.fetchAndAddOrdered(0) <= 0; }
    inline bool operator!() const { return isNull(); }
    operator UnspecifiedBoolType() const { return !isNull() ? &WeakPtr<T>::operator! : 0; }

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
        RefCounted::SharedCount *tmp = sc;
        sc = o.sc;
        o.sc = tmp;
    }

    SharedPtr<T> toStrongRef() const { return SharedPtr<T>(*this); }

private:
    friend class SharedPtr<T>;
    friend uint qHash<T>(const WeakPtr<T> &ptr);

    RefCounted::SharedCount *sc;
};

template<typename T>
inline uint qHash(const WeakPtr<T> &ptr)
{
    T *actualPtr = ptr.sc ? ptr.sc.d : 0;
    return QT_PREPEND_NAMESPACE(qHash<T>(actualPtr));
}

} // Tp

#endif
