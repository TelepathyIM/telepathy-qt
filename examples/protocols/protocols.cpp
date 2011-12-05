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

#include "protocols.h"
#include "_gen/protocols.moc.hpp"

#include <TelepathyQt/Debug>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingStringList>

#include <QCoreApplication>
#include <QDebug>

Protocols::Protocols()
    : cmWrappersFinished(0)
{
    qDebug() << "Listing names";
    connect(ConnectionManager::listNames(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onListNamesFinished(Tp::PendingOperation *)));
}

Protocols::~Protocols()
{
}

void Protocols::onListNamesFinished(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Error listing connection manager names -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    PendingStringList *ps = qobject_cast<PendingStringList *>(op);

    qDebug() << "Supported CMs:" << ps->result();

    foreach (const QString &cmName, ps->result()) {
        CMWrapper *cmWrapper = new CMWrapper(cmName, this);
        mCMWrappers.append(cmWrapper);
        connect(cmWrapper,
                SIGNAL(finished()),
                SLOT(onCMWrapperFinished()));
    }
}

void Protocols::onCMWrapperFinished()
{
    if (++cmWrappersFinished == mCMWrappers.size()) {
        QCoreApplication::quit();
    }
}

