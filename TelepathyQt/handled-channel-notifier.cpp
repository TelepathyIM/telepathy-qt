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

#include <TelepathyQt/HandledChannelNotifier>

#include "TelepathyQt/_gen/handled-channel-notifier.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/request-temporary-handler-internal.h"

#include <TelepathyQt/ChannelRequestHints>
#include <TelepathyQt/ClientRegistrar>

namespace Tp
{

struct TP_QT_NO_EXPORT HandledChannelNotifier::Private
{
    Private(const ClientRegistrarPtr &cr,
            const SharedPtr<RequestTemporaryHandler> &handler)
        : cr(cr),
          handler(handler),
          channel(handler->channel())
    {
    }

    ClientRegistrarPtr cr;
    SharedPtr<RequestTemporaryHandler> handler;
    ChannelPtr channel; // needed to keep channel alive, since RTH maintains only a weak ref
};

/**
 * \class HandledChannelNotifier
 * \ingroup clientchannel
 * \headerfile TelepathyQt/handled-channel-notifier.h <TelepathyQt/HandledChannelNotifier>
 *
 * \brief The HandledChannelNotifier class can be used to keep track of
 * channel() being re-requested.
 *
 * Instances of this class cannot be constructed directly; the only way to get
 * one is trough PendingChannel.
 */

HandledChannelNotifier::HandledChannelNotifier(const ClientRegistrarPtr &cr,
        const SharedPtr<RequestTemporaryHandler> &handler)
    : mPriv(new Private(cr, handler))
{
    connect(mPriv->channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated()));
    connect(handler.data(),
            SIGNAL(channelReceived(Tp::ChannelPtr,QDateTime,Tp::ChannelRequestHints)),
            SLOT(onChannelReceived(Tp::ChannelPtr,QDateTime,Tp::ChannelRequestHints)));
}

HandledChannelNotifier::~HandledChannelNotifier()
{
    delete mPriv;
}

ChannelPtr HandledChannelNotifier::channel() const
{
    return mPriv->channel;
}

void HandledChannelNotifier::onChannelReceived(const Tp::ChannelPtr &channel,
        const QDateTime &userActionTime, const Tp::ChannelRequestHints &requestHints)
{
    emit handledAgain(userActionTime, requestHints);
}

void HandledChannelNotifier::onChannelInvalidated()
{
    deleteLater();
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void HandledChannelNotifier::connectNotify(const QMetaMethod &signal)
{
    if (signal == QMetaMethod::fromSignal(&HandledChannelNotifier::handledAgain)) {
        mPriv->handler->setQueueChannelReceived(false);
    }
}
#else
void HandledChannelNotifier::connectNotify(const char *signalName)
{
    if (qstrcmp(signalName, SIGNAL(handledAgain(QDateTime,Tp::ChannelRequestHints))) == 0) {
        mPriv->handler->setQueueChannelReceived(false);
    }
}
#endif

} // Tp
