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

#include <TelepathyQt/AccountPropertyFilter>

#include "TelepathyQt/debug-internal.h"

#include <QLatin1String>
#include <QStringList>
#include <QMetaObject>
#include <QVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT AccountPropertyFilter::Private
{
    Private()
    {
        if (supportedAccountProperties.isEmpty()) {
            const QMetaObject metaObject = Account::staticMetaObject;
            for (int i = metaObject.propertyOffset(); i < metaObject.propertyCount(); ++i) {
                supportedAccountProperties << QLatin1String(metaObject.property(i).name());
            }
        }
    }

    static QStringList supportedAccountProperties;
};

QStringList AccountPropertyFilter::Private::supportedAccountProperties;

/**
 * \class Tp::AccountPropertyFilter
 * \ingroup utils
 * \headerfile TelepathyQt/account-property-filter.h <TelepathyQt/AccountPropertyFilter>
 *
 * \brief The AccountPropertyFilter class provides a filter object to be used
 * to filter accounts by properties.
 */

AccountPropertyFilter::AccountPropertyFilter()
    : GenericPropertyFilter<Account>(),
      mPriv(new Private())
{
}

AccountPropertyFilter::~AccountPropertyFilter()
{
    delete mPriv;
}

bool AccountPropertyFilter::isValid() const
{
    QVariantMap mFilter = filter();
    if (mFilter.isEmpty()) {
        return false;
    }

    QVariantMap::const_iterator i = mFilter.constBegin();
    QVariantMap::const_iterator end = mFilter.constEnd();
    while (i != end) {
        QString propertyName = i.key();
        if (!mPriv->supportedAccountProperties.contains(propertyName)) {
            warning() << "Invalid filter key" << propertyName <<
                "while filtering account by properties";
            return false;
        }
        ++i;
    }

    return true;
}

} // Tp
