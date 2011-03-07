/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_and_filter_h_HEADER_GUARD_
#define _TelepathyQt4_and_filter_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Filter>
#include <TelepathyQt4/Types>

namespace Tp
{

template <class T>
class AndFilter : public Filter<T>
{
public:
    static SharedPtr<AndFilter<T> > create(
            const QList<SharedPtr<const Filter<T> > > &filters = QList<SharedPtr<const Filter<T> > >())
    {
        return SharedPtr<AndFilter<T> >(new AndFilter<T>(filters));
    }

    inline virtual ~AndFilter() { }

    inline virtual bool isValid() const
    {
        Q_FOREACH (const SharedPtr<const Filter<T> > &filter, mFilters) {
            if (!filter || !filter->isValid()) {
                return false;
            }
        }
        return true;
    }

    inline virtual bool matches(const SharedPtr<T> &t) const
    {
        if (!isValid()) {
            return false;
        }

        Q_FOREACH (const SharedPtr<const Filter<T> > &filter, mFilters) {
            if (!filter->matches(t)) {
                return false;
            }
        }
        return true;
    }

    inline QList<SharedPtr<const Filter<T> > > filters() const { return mFilters; }

private:
    AndFilter(const QList<SharedPtr<const Filter<T> > > &filters)
        : Filter<T>(), mFilters(filters) { }

    QList<SharedPtr<const Filter<T> > > mFilters;
};

} // Tp

#endif
