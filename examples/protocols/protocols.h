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

#ifndef _TelepathyQt_examples_protocols_protocols_h_HEADER_GUARD_
#define _TelepathyQt_examples_protocols_protocols_h_HEADER_GUARD_

#include <TelepathyQt/Types>

#include "cm-wrapper.h"

#include <QList>
#include <QObject>

using namespace Tp;

namespace Tp
{
class PendingOperation;
}

class Protocols : public QObject
{
    Q_OBJECT

public:
   Protocols();
   ~Protocols();

private Q_SLOTS:
    void onListNamesFinished(Tp::PendingOperation *op);
    void onCMWrapperFinished();

private:
    QList<CMWrapper *> mCMWrappers;
    int cmWrappersFinished;
};

#endif
