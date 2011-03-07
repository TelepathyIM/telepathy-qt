/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_shared_ptr_h_HEADER_GUARD_
#define _TelepathyQt4_shared_ptr_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>

#include <QHash>
#include <QWeakPointer>

namespace Tp
{

class RefCounted;
template <class T> class SharedPtr;

class TELEPATHY_QT4_EXPORT RefCounted
{
    Q_DISABLE_COPY(RefCounted)

public:
    inline RefCounted() : strongref(0) { }
    inline virtual ~RefCounted() { }

    inline void ref() const { strongref.ref(); }
    inline bool deref() const { return strongref.deref(); }

    mutable QAtomicInt strongref;
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
    explicit inline SharedPtr(const QWeakPointer<T> &o)
    {
        if (o.data() && o.data()->strongref > 0) {
            d = static_cast<T*>(o.data());
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
    T *d;
};

template<typename T>
inline uint qHash(const SharedPtr<T> &ptr)
{
    return QT_PREPEND_NAMESPACE(qHash<T>(ptr.data()));
}

} // Tp

#endif
