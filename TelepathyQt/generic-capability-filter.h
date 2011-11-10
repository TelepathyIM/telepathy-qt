/**
 * This file is part of TelepathyQt
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

#ifndef _TelepathyQt_generic_capability_filter_h_HEADER_GUARD_
#define _TelepathyQt_generic_capability_filter_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/Filter>
#include <TelepathyQt/Types>

namespace Tp
{

template <class T>
class GenericCapabilityFilter : public Filter<T>
{
public:
    static SharedPtr<GenericCapabilityFilter<T> > create(
            const RequestableChannelClassSpecList &rccSpecs = RequestableChannelClassSpecList())
    {
        return SharedPtr<GenericCapabilityFilter<T> >(new GenericCapabilityFilter<T>(
                    rccSpecs));
    }

    inline virtual ~GenericCapabilityFilter() { }

    inline virtual bool isValid() const { return true; }

    inline virtual bool matches(const SharedPtr<T> &t) const
    {
        bool supportedRcc;
        RequestableChannelClassSpecList objectRccSpecs = t->capabilities().allClassSpecs();
        Q_FOREACH (const RequestableChannelClassSpec &filterRccSpec, mFilter) {
            supportedRcc = false;

            Q_FOREACH (const RequestableChannelClassSpec &objectRccSpec, objectRccSpecs) {
                /* check if fixed properties match */
                if (filterRccSpec.fixedProperties() == objectRccSpec.fixedProperties()) {
                    supportedRcc = true;

                    /* check if all allowed properties in the filter RCC
                     * are in the object RCC allowed properties */
                    Q_FOREACH (const QString &value, filterRccSpec.allowedProperties()) {
                        if (!objectRccSpec.allowsProperty(value)) {
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

    inline RequestableChannelClassSpecList filter() const { return mFilter; }

    inline void addRequestableChannelClassSubset(const RequestableChannelClassSpec &rccSpec)
    {
        mFilter.append(rccSpec.bareClass());
    }

    inline void setRequestableChannelClassesSubset(const RequestableChannelClassSpecList &rccSpecs)
    {
        mFilter = rccSpecs.bareClasses();
    }

private:
    GenericCapabilityFilter(const RequestableChannelClassSpecList &rccSpecs)
        : Filter<T>(), mFilter(rccSpecs) { }

    RequestableChannelClassSpecList mFilter;
};

} // Tp

#endif
