/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt_channel_dispatch_operation_internal_h_HEADER_GUARD_
#define _TelepathyQt_channel_dispatch_operation_internal_h_HEADER_GUARD_

#include <TelepathyQt/AbstractClientHandler>
#include <TelepathyQt/PendingOperation>

namespace Tp
{

class TP_QT_NO_EXPORT ChannelDispatchOperation::PendingClaim : public PendingOperation
{
    Q_OBJECT

public:
    PendingClaim(const ChannelDispatchOperationPtr &op,
            const AbstractClientHandlerPtr &handler = AbstractClientHandlerPtr());
    ~PendingClaim();

private Q_SLOTS:
    TP_QT_NO_EXPORT void onClaimFinished(Tp::PendingOperation *op);

private:
    ChannelDispatchOperationPtr mDispatchOp;
    AbstractClientHandlerPtr mHandler;
};

} // Tp

#endif
