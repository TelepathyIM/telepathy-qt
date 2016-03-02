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
#include "TelepathyQt/base-debug-internal.h"

#include <TelepathyQt/DBusObject>

#include "TelepathyQt/_gen/base-debug.moc.hpp"
#include "TelepathyQt/_gen/base-debug-internal.moc.hpp"

namespace Tp
{

struct TP_QT_NO_EXPORT BaseDebug::Private
{
    Private(BaseDebug *parent, const QDBusConnection &dbusConnection)
        : parent(parent),
          enabled(false),
          getMessagesLimit(0),
          lastMessageIndex(-1),
          adaptee(new BaseDebug::Adaptee(dbusConnection, parent))
    {
    }

    BaseDebug *parent;
    bool enabled;
    int getMessagesLimit;
    int lastMessageIndex;

    DebugMessageList messages;

    GetMessagesCallback getMessageCB;
    BaseDebug::Adaptee *adaptee;
};

BaseDebug::Adaptee::Adaptee(const QDBusConnection &dbusConnection, BaseDebug *interface)
    : QObject(interface),
      mInterface(interface)
{
    (void) new Service::DebugAdaptor(dbusConnection, this, interface->dbusObject());
}

bool BaseDebug::Adaptee::isEnabled()
{
    return mInterface->isEnabled();
}

void BaseDebug::Adaptee::setEnabled(bool enabled)
{
    mInterface->mPriv->enabled = enabled;
}

void BaseDebug::Adaptee::getMessages(const Service::DebugAdaptor::GetMessagesContextPtr &context)
{
    DBusError error;
    DebugMessageList messages = mInterface->getMessages(&error);

    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished(messages);
}

BaseDebug::BaseDebug(const QDBusConnection &dbusConnection) :
    DBusService(dbusConnection),
    mPriv(new Private(this, dbusConnection))
{
}

bool BaseDebug::isEnabled() const
{
    return mPriv->enabled;
}

int BaseDebug::getMessagesLimit() const
{
    return mPriv->getMessagesLimit;
}

void BaseDebug::setGetMessagesCallback(const BaseDebug::GetMessagesCallback &cb)
{
    mPriv->getMessageCB = cb;
}

DebugMessageList BaseDebug::getMessages(Tp::DBusError *error) const
{
    if (!mPriv->getMessageCB.isValid()) {
        if (mPriv->getMessagesLimit) {
            if (mPriv->lastMessageIndex < 0) {
                return mPriv->messages;
            } else {
                return mPriv->messages.mid(mPriv->lastMessageIndex + 1) + mPriv->messages.mid(0, mPriv->lastMessageIndex + 1);
            }
        }
        error->set(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return DebugMessageList();
    }

    return mPriv->getMessageCB(error);
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
}

void BaseDebug::clear()
{
    mPriv->messages.clear();
    mPriv->lastMessageIndex = -1;
}

void BaseDebug::newDebugMessage(const QString &domain, DebugLevel level, const QString &message)
{
    qint64 msec = QDateTime::currentMSecsSinceEpoch();
    double time = msec / 1000 + (msec % 1000 / 1000.0);

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
        } else { // This works when the limit is not hitted yet, or when there is no limit at all (negative limit number)
            mPriv->messages << newMessage;
        }
    }

    if (!isEnabled()) {
        return;
    }

    QMetaObject::invokeMethod(mPriv->adaptee, "newDebugMessage",
                              Q_ARG(double, time), Q_ARG(QString, domain),
                              Q_ARG(uint, level), Q_ARG(QString, message)); //Can simply use emit in Qt5
}

QVariantMap BaseDebug::immutableProperties() const
{
    // There is no immutable properties.
    return QVariantMap();
}

bool BaseDebug::registerObject(Tp::DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    DBusError _error;
    bool ret = DBusService::registerObject(TP_QT_IFACE_DEBUG, TP_QT_DEBUG_OBJECT_PATH, &_error);

    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

}
