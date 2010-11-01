/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_generic_capability_filter_h_HEADER_GUARD_
#define _TelepathyQt4_generic_capability_filter_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/ConnectionCapabilities>
#include <TelepathyQt4/Filter>
#include <TelepathyQt4/Types>

namespace Tp
{

template <class T>
class GenericCapabilityFilter : public Filter<T>
{
public:
    static SharedPtr<GenericCapabilityFilter<T> > create(const RequestableChannelClassList &rccs
            = RequestableChannelClassList())
    {
        return SharedPtr<GenericCapabilityFilter<T> >(new GenericCapabilityFilter<T>(rccs));
    }

    static SharedPtr<GenericCapabilityFilter<T> > create(
            const RequestableChannelClassSpecList &rccSpecs)
    {
        return SharedPtr<GenericCapabilityFilter<T> >(new GenericCapabilityFilter<T>(
                    rccSpecs.bareClasses()));
    }

    inline virtual ~GenericCapabilityFilter() { }

    inline virtual bool isValid() const { return true; }

    inline virtual bool matches(const SharedPtr<T> &t) const
    {
        bool supportedRcc;
        RequestableChannelClassList objectRccs = t->capabilities() ?
            t->capabilities()->allClassSpecs().bareClasses() :
            RequestableChannelClassList();
        Q_FOREACH (const RequestableChannelClass &filterRcc, mFilter) {
            supportedRcc = false;

            Q_FOREACH (const RequestableChannelClass &objectRcc, objectRccs) {
                /* check if fixed properties match */
                if (filterRcc.fixedProperties == objectRcc.fixedProperties) {
                    supportedRcc = true;

                    /* check if all allowed properties in the filter RCC
                     * are in the object RCC allowed properties */
                    Q_FOREACH (const QString &value, filterRcc.allowedProperties) {
                        if (!objectRcc.allowedProperties.contains(value)) {
                            /* one of the properties in the filter RCC
                             * allowed properties is not in the object RCC
                             * allowed properties */
                            supportedRcc = false;
                            break;
                        }
                    }

                    /* this RCC is supported, no need to check anymore */
                    if (supportedRcc) {
                        break;
                    }
                }
            }

            /* one of the filter RCC is not supported, this object
             * won't match filter */
            if (!supportedRcc) {
                return false;
            }
        }

        return true;
    }

    // FIXME: (API/ABI break) Return a RCCSpecList instead
    inline RequestableChannelClassList filter() const { return mFilter; }

    inline void addRequestableChannelClassSubset(const RequestableChannelClass &rcc)
    {
        mFilter.append(rcc);
    }

    inline void addRequestableChannelClassSubset(const RequestableChannelClassSpec &rccSpec)
    {
        mFilter.append(rccSpec.bareClass());
    }

    inline void setRequestableChannelClassesSubset(const RequestableChannelClassList &rccs)
    {
        mFilter = rccs;
    }

    inline void setRequestableChannelClassesSubset(const RequestableChannelClassSpecList &rccSpecs)
    {
        mFilter = rccSpecs.bareClasses();
    }

private:
    GenericCapabilityFilter(const RequestableChannelClassList &rccs) : Filter<T>(), mFilter(rccs) { }

    // FIXME: (API/ABI break) Use RCCSpecList instead
    RequestableChannelClassList mFilter;
};

} // Tp

#endif
