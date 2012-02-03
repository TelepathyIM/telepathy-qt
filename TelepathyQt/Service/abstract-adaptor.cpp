/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#include <TelepathyQt/Service/AbstractAdaptor>

#include "TelepathyQt/Service/_gen/abstract-adaptor.moc.hpp"

#include "TelepathyQt/debug-internal.h"

namespace Tp
{
namespace Service
{

struct TP_QT_SVC_NO_EXPORT AbstractAdaptor::Private
{
    Private(QObject *adaptee)
        : adaptee(adaptee)
    {
    }

    QObject *adaptee;
};

AbstractAdaptor::AbstractAdaptor(QObject *adaptee, QObject *parent)
    : QDBusAbstractAdaptor(parent),
      mPriv(new Private(adaptee))
{
    setAutoRelaySignals(false);
}

AbstractAdaptor::~AbstractAdaptor()
{
    delete mPriv;
}

void AbstractAdaptor::setAdaptee(QObject *adaptee)
{
    mPriv->adaptee = adaptee;
}

QObject *AbstractAdaptor::adaptee() const
{
    return mPriv->adaptee;
}

}
}
