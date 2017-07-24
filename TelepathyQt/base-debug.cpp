/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
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

#include <TelepathyQt/BaseDebug>
#include "TelepathyQt/_gen/svc-debug.h"
#include "TelepathyQt/_gen/base-debug.moc.hpp"

#include <TelepathyQt/DBusError>

namespace Tp
{

struct TP_QT_NO_EXPORT BaseDebug::Private
{
    Private(BaseDebug *parent)
        : parent(parent),
          enabled(false),
          getMessagesLimit(500),
          lastMessageIndex(-1),
          adaptee(Service::DebugAdaptee::create())
    {
        messages.reserve(getMessagesLimit);
        adaptee->implementGetMessages([this](const Service::DebugAdaptee::GetMessagesContextPtr &context) {
            context->setFinished(getMessages());
        });
    }

    DebugMessageList getMessages() const;

    BaseDebug *parent;
    bool enabled;
    int getMessagesLimit;
    int lastMessageIndex;

    DebugMessageList messages;
    Service::DebugAdapteePtr adaptee;
};

DebugMessageList BaseDebug::Private::getMessages() const
{
    if (lastMessageIndex < 0) {
        return messages;
    } else {
        return messages.mid(lastMessageIndex + 1) + messages.mid(0, lastMessageIndex + 1);
    }
}

BaseDebug::BaseDebug(const QDBusConnection &dbusConnection) :
    DBusObject(dbusConnection),
    mPriv(new Private(this))
{
    plugInterfaceAdaptee(mPriv->adaptee);
}

bool BaseDebug::isEnabled() const
{
    return mPriv->adaptee->isEnabled();
}

int BaseDebug::getMessagesLimit() const
{
    return mPriv->getMessagesLimit;
}

Tp::DebugMessageList BaseDebug::getMessages() const
{
    return mPriv->getMessages();
}

void BaseDebug::setEnabled(bool enabled)
{
    mPriv->enabled = enabled;
}

void BaseDebug::setGetMessagesLimit(int limit)
{
    mPriv->getMessagesLimit = limit;
    DebugMessageList messages;

    if (mPriv->lastMessageIndex < 0) {
        messages = mPriv->messages;
    } else {
        messages = mPriv->messages.mid(mPriv->lastMessageIndex + 1) + mPriv->messages.mid(0, mPriv->lastMessageIndex + 1);
    }

    mPriv->lastMessageIndex = -1;

    if (mPriv->messages.count() <= limit) {
        mPriv->messages = messages;
    } else {
        mPriv->messages = messages.mid(messages.count() - limit, limit);
    }
    messages.reserve(limit);
}

void BaseDebug::clear()
{
    mPriv->messages.clear();
    mPriv->lastMessageIndex = -1;
}

void BaseDebug::newDebugMessage(const QString &domain, DebugLevel level, const QString &message)
{
    const qint64 msec = QDateTime::currentMSecsSinceEpoch();
    const double time = msec / 1000 + (msec % 1000 / 1000.0);
    newDebugMessage(time, domain, level, message);
}

void BaseDebug::newDebugMessage(double time, const QString &domain, DebugLevel level, const QString &message)
{
    if (mPriv->getMessagesLimit != 0) {
        DebugMessage newMessage;
        newMessage.timestamp = time;
        newMessage.domain = domain;
        newMessage.level = level;
        newMessage.message = message;

        if (mPriv->messages.count() == mPriv->getMessagesLimit) {
            ++mPriv->lastMessageIndex;

            if (mPriv->lastMessageIndex >= mPriv->messages.count()) {
                mPriv->lastMessageIndex = 0;
            }

            mPriv->messages[mPriv->lastMessageIndex] = newMessage;
        } else { // The limit is not hitted yet or there is no limit at all (negative limit)
            mPriv->messages << newMessage;
        }
    }

    if (!isEnabled()) {
        return;
    }

    mPriv->adaptee->emitNewDebugMessage(time, domain, level, message);
}

bool BaseDebug::registerObject(DBusError *error)
{
    DBusError _error;
    if (!error) {
        error = &_error;
    }
    return DBusObject::registerObject(TP_QT_DEBUG_OBJECT_PATH, error);
}

} // namespace Tp
