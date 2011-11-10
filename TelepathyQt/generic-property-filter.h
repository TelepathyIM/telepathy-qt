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

#ifndef _TelepathyQt_generic_property_filter_h_HEADER_GUARD_
#define _TelepathyQt_generic_property_filter_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Filter>
#include <TelepathyQt/Types>

namespace Tp
{

template <class T>
class GenericPropertyFilter : public Filter<T>
{
public:
    inline virtual ~GenericPropertyFilter() { }

    inline virtual bool isValid() const { return true; }

    inline virtual bool matches(const SharedPtr<T> &t) const
    {
        for (QVariantMap::const_iterator i = mFilter.constBegin();
                i != mFilter.constEnd(); ++i) {
            QString propertyName = i.key();
            QVariant propertyValue = i.value();

            if (t->property(propertyName.toLatin1().constData()) != propertyValue) {
                return false;
            }
        }

        return true;
    }

    inline QVariantMap filter() const { return mFilter; }

    inline void addProperty(const QString &propertyName, const QVariant &propertyValue)
    {
        mFilter.insert(propertyName, propertyValue);
    }

    inline void setProperties(const QVariantMap &filter) { mFilter = filter; }

protected:
    inline GenericPropertyFilter() : Filter<T>() { }

private:
    QVariantMap mFilter;
};

} // Tp

#endif
