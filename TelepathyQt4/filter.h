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

#ifndef _TelepathyQt4_filter_h_HEADER_GUARD_
#define _TelepathyQt4_filter_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/SharedPtr>
#include <TelepathyQt4/Types>

namespace Tp
{

/*
 * TODO filter:
 *
 * Make filters not always be AND - as in, if you have multiple filters, you might want to do both
 * if ANY of them match or if (A AND B match) OR (C MATCHES) etc - so provide generic "combinator
 * filters"
 */
template <class T>
class Filter : public RefCounted
{
    Q_DISABLE_COPY(Filter)

public:
    virtual ~Filter() {}

    virtual bool isValid() const { return false; }

    virtual bool matches(const SharedPtr<T> &t) const { return false; }

protected:
    Filter() {}

private:
    struct Private;
    Private *mPriv; // Just a placeholder really
};

} // Tp

#endif
