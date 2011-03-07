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

#ifndef _TelepathyQt4_object_h_HEADER_GUARD_
#define _TelepathyQt4_object_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Global>
#include <TelepathyQt4/RefCounted>

#include <QObject>
#include <QString>

namespace Tp
{

class TELEPATHY_QT4_EXPORT Object : public QObject, public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(Object)

public:
    virtual ~Object();

Q_SIGNALS:
    void propertyChanged(const QString &propertyName);

protected:
    Object();

    void notify(const char *propertyName);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
