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

#ifndef _TelepathyQt_referenced_handles_h_HEADER_GUARD_
#define _TelepathyQt_referenced_handles_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#ifndef QT_NO_STL
# include <list>
#endif

#include <QList>
#include <QSet>
#include <QSharedDataPointer>
#include <QVector>

namespace Tp
{

class Connection;

class TP_QT_EXPORT ReferencedHandles
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
    ReferencedHandles(const ReferencedHandles &other);
    ~ReferencedHandles();

    ConnectionPtr connection() const;
    HandleType handleType() const;

    uint at(int i) const;

    inline uint back() const
    {
        return last();
    }

    inline uint first() const
    {
        return at(0);
    }

    inline uint front() const
    {
        return first();
    }

    inline uint last() const
    {
        return at(size() - 1);
    }

    uint value(int i, uint defaultValue = 0) const;

    const_iterator begin() const;

    inline const_iterator constBegin() const
    {
        return begin();
    }

    inline const_iterator constEnd() const
    {
        return end();
    }

    const_iterator end() const;

    bool contains(uint handle) const;

    int count(uint handle) const;

    inline int count() const
    {
        return size();
    }

    inline bool empty() const
    {
        return isEmpty();
    }

    inline bool endsWith(uint handle) const
    {
        return !isEmpty() && last() == handle;
    }

    int indexOf(uint handle, int from = 0) const;

    bool isEmpty() const;

    int lastIndexOf(uint handle, int from = -1) const;

    inline int length() const
    {
        return count();
    }

    ReferencedHandles mid(int pos, int length = -1) const;

    int size() const;

    inline bool startsWith(uint handle) const
    {
        return !isEmpty() && first() == handle;
    }

    inline void append(const ReferencedHandles& another)
    {
        *this = *this + another;
    }

    void clear();
    void move(int from, int to);

    inline void pop_back()
    {
        return removeLast();
    }

    inline void pop_front()
    {
        return removeFirst();
    }

    int removeAll(uint handle);

    void removeAt(int i);

    inline void removeFirst()
    {
        return removeAt(0);
    }

    inline void removeLast()
    {
        return removeAt(size() - 1);
    }

    bool removeOne(uint handle);

    void swap(int i, int j);

    uint takeAt(int i);

    inline uint takeFirst()
    {
        return takeAt(0);
    }

    inline uint takeLast()
    {
        return takeAt(size() - 1);
    }

    bool operator!=(const ReferencedHandles& another) const
    {
        return !(*this == another);
    }

    bool operator!=(const UIntList& another) const
    {
        return !(*this == another);
    }

    ReferencedHandles operator+(const ReferencedHandles& another) const;

    inline ReferencedHandles& operator+=(const ReferencedHandles& another)
    {
        return *this = (*this + another);
    }

    ReferencedHandles& operator<<(const ReferencedHandles& another)
    {
        return *this += another;
    }

    ReferencedHandles& operator=(const ReferencedHandles& another);

    bool operator==(const ReferencedHandles& another) const;
    bool operator==(const UIntList& list) const;

    inline uint operator[](int i) const
    {
        return at(i);
    }

    UIntList toList() const;

    inline QSet<uint> toSet() const
    {
        return toList().toSet();
    }

#ifndef QT_NO_STL
    inline std::list<uint> toStdList() const
    {
        return toList().toStdList();
    }
#endif

    inline QVector<uint> toVector() const
    {
        return toList().toVector();
    }

private:
    // For access to the "prime" constructor
    friend class ContactManager;
    friend class PendingContactAttributes;
    friend class PendingContacts;
    friend class PendingHandles;

    TP_QT_NO_EXPORT ReferencedHandles(const ConnectionPtr &connection,
            HandleType handleType, const UIntList& handles);

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

typedef QListIterator<uint> ReferencedHandlesIterator;

} // Tp

Q_DECLARE_METATYPE(Tp::ReferencedHandles);

#endif
