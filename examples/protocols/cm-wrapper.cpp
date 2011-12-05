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

#include "cm-wrapper.h"
#include "_gen/cm-wrapper.moc.hpp"

#include <TelepathyQt/Debug>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/PendingReady>

#include <QDebug>

CMWrapper::CMWrapper(const QString &cmName, QObject *parent)
    : QObject(parent),
      mCM(ConnectionManager::create(cmName))
{
    connect(mCM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCMReady(Tp::PendingOperation *)));
}

CMWrapper::~CMWrapper()
{
}

ConnectionManagerPtr CMWrapper::cm() const
{
    return mCM;
}

void CMWrapper::onCMReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CM" << mCM->name() << "cannot become ready -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "CM" << mCM->name() << "ready!";
    qDebug() << "Supported protocols:";
    foreach (const QString &protocol, mCM->supportedProtocols()) {
        qDebug() << "\t" << protocol;
    }

    emit finished();
}
