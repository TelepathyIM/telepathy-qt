/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
 * @copyright Copyright 2013 Canonical Ltd.
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

#include <TelepathyQt/BaseCall>
#include <TelepathyQt/BaseChannel>
#include "TelepathyQt/base-call-internal.h"

#include "TelepathyQt/_gen/base-call.moc.hpp"
#include "TelepathyQt/_gen/base-call-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/DBusObject>
#include <TelepathyQt/Utils>
#include <TelepathyQt/AbstractProtocolInterface>
#include <QString>
#include <QVariantMap>

namespace Tp
{

/**
 * \class AbstractCallContentInterface
 * \ingroup servicechannel
 * \headerfile TelepathyQt/base-call.h <TelepathyQt/BaseCall>
 *
 * \brief Base class for all the CallContent object interface implementations.
 */

AbstractCallContentInterface::AbstractCallContentInterface(const QString &interfaceName)
    : AbstractDBusServiceInterface(interfaceName)
{
}

AbstractCallContentInterface::~AbstractCallContentInterface()
{
}

BaseCallContent::Adaptee::Adaptee(const QDBusConnection &dbusConnection,
                              BaseCallContent *content)
    : QObject(content),
      mContent(content)
{
    debug() << "Creating service::CallContentAdaptor for " << content->dbusObject();
    mAdaptor = new Service::CallContentAdaptor(dbusConnection, this, content->dbusObject());
}

QStringList BaseCallContent::Adaptee::interfaces() const
{
    QStringList ret;
    foreach(const AbstractCallContentInterfacePtr & iface, mContent->interfaces()) {
        ret << iface->interfaceName();
    }
    ret << TP_QT_IFACE_PROPERTIES;
    return ret;
}

BaseCallContent::Adaptee::~Adaptee()
{
}

void BaseCallContent::Adaptee::remove(const Tp::Service::CallContentAdaptor::RemoveContextPtr &context)
{
    context->setFinished();
}

struct TP_QT_NO_EXPORT BaseCallContent::Private {
    Private(BaseCallContent *parent,
            const QDBusConnection &dbusConnection,
            BaseChannel* channel,
            const QString &name,
            const Tp::MediaStreamType &type,
            const Tp::MediaStreamDirection &direction)
        : parent(parent),
          channel(channel),
          name(name),
          type(type),
          disposition(Tp::CallContentDispositionNone),
          direction(direction),
          adaptee(new BaseCallContent::Adaptee(dbusConnection, parent)) {
    }

    BaseCallContent *parent;
    BaseChannel *channel;

    QString name;
    Tp::MediaStreamType type;
    Tp::CallContentDisposition disposition;
    Tp::ObjectPathList streams;

    Tp::MediaStreamDirection direction;

    QHash<QString, AbstractCallContentInterfacePtr> interfaces;
    BaseCallContent::Adaptee *adaptee;
};

BaseCallContent::BaseCallContent(const QDBusConnection &dbusConnection,
                         BaseChannel* channel,
                         const QString &name,
                         const Tp::MediaStreamType &type,
                         const Tp::MediaStreamDirection &direction)
    : DBusService(dbusConnection),
      mPriv(new Private(this, dbusConnection, channel,
                        name, type, direction))
{
}

BaseCallContent::~BaseCallContent()
{
    delete mPriv;
}

QVariantMap BaseCallContent::immutableProperties() const
{
    QVariantMap map;
    map.insert(TP_QT_IFACE_CALL_CONTENT + QLatin1String(".Interfaces"),
               QVariant::fromValue(mPriv->adaptee->interfaces()));
    map.insert(TP_QT_IFACE_CALL_CONTENT + QLatin1String(".Name"),
               QVariant::fromValue(mPriv->adaptee->name()));
    map.insert(TP_QT_IFACE_CALL_CONTENT + QLatin1String(".Type"),
               QVariant::fromValue((uint)mPriv->adaptee->type()));
    map.insert(TP_QT_IFACE_CALL_CONTENT + QLatin1String(".Disposition"),
               QVariant::fromValue((uint)mPriv->adaptee->disposition()));
    return map;
}

QString BaseCallContent::name() const {
    return mPriv->name;
}

Tp::MediaStreamType BaseCallContent::type() const {
    return mPriv->type;
}

Tp::CallContentDisposition BaseCallContent::disposition() const {
    return mPriv->disposition;
}

Tp::ObjectPathList BaseCallContent::streams() const {
    return mPriv->streams;
}

QString BaseCallContent::uniqueName() const
{
    return QString(QLatin1String("_%1")).arg((quintptr) this, 0, 16);
}

bool BaseCallContent::registerObject(DBusError *error)
{
    if (isRegistered()) {
        return true;
    }

    QString name = mPriv->name;
    QString busName = mPriv->channel->busName();
    QString objectPath = QString(QLatin1String("%1/%2"))
                         .arg(mPriv->channel->objectPath(), name);
    debug() << "Registering Content: busName: " << busName << " objectName: " << objectPath;
    DBusError _error;

    debug() << "CallContent: registering interfaces  at " << dbusObject();
    foreach(const AbstractCallContentInterfacePtr & iface, mPriv->interfaces) {
        if (!iface->registerInterface(dbusObject())) {
            // lets not fail if an optional interface fails registering, lets warn only
            warning() << "Unable to register interface" << iface->interfaceName();
        }
    }

    bool ret = registerObject(busName, objectPath, &_error);
    if (!ret && error) {
        error->set(_error.name(), _error.message());
    }
    return ret;
}

/**
 * Reimplemented from DBusService.
 */
bool BaseCallContent::registerObject(const QString &busName,
                                 const QString &objectPath, DBusError *error)
{
    return DBusService::registerObject(busName, objectPath, error);
}

QList<AbstractCallContentInterfacePtr> BaseCallContent::interfaces() const
{
    return mPriv->interfaces.values();
}

bool BaseCallContent::plugInterface(const AbstractCallContentInterfacePtr &interface)
{
    if (isRegistered()) {
        warning() << "Unable to plug protocol interface " << interface->interfaceName() <<
                  "- protocol already registered";
        return false;
    }

    if (interface->isRegistered()) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- interface already registered";
        return false;
    }

    if (mPriv->interfaces.contains(interface->interfaceName())) {
        warning() << "Unable to plug protocol interface" << interface->interfaceName() <<
                  "- another interface with same name already plugged";
        return false;
    }

    debug() << "Interface" << interface->interfaceName() << "plugged";
    mPriv->interfaces.insert(interface->interfaceName(), interface);
    return true;
}

// Call.I.Mute
BaseCallMuteInterface::Adaptee::Adaptee(BaseCallMuteInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseCallMuteInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseCallMuteInterface::Private {
    Private(BaseCallMuteInterface *parent, Tp::LocalMuteState state)
        : state(state),
          adaptee(new BaseCallMuteInterface::Adaptee(parent)) {
    }

    SetMuteStateCallback setMuteStateCB;
    Tp::LocalMuteState state;
    BaseCallMuteInterface::Adaptee *adaptee;
};

void BaseCallMuteInterface::Adaptee::requestMuted(bool mute, const Tp::Service::CallInterfaceMuteAdaptor::RequestMutedContextPtr &context)
{
    if (!mInterface->mPriv->setMuteStateCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    Tp::LocalMuteState state = Tp::LocalMuteStateUnmuted;
    if (mute) {
        state = Tp::LocalMuteStateMuted;
    }

    DBusError error;
    mInterface->mPriv->setMuteStateCB(state, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}


/**
 * \class BaseCallMuteInterface
 * \ingroup servicechannel
 * \headerfile TelepathyQt/base-call.h <TelepathyQt/BaseCall>
 *
 * \brief Base class for implementations of Call.Interface.Mute
 *
 */

/**
 * Class constructor.
 */
BaseCallMuteInterface::BaseCallMuteInterface()
    : AbstractChannelInterface(TP_QT_IFACE_CALL_INTERFACE_MUTE),
      mPriv(new Private(this, Tp::LocalMuteStateUnmuted))
{
}

Tp::LocalMuteState BaseCallMuteInterface::localMuteState() const
{
    return mPriv->state;
}

void BaseCallMuteInterface::setMuteState(const Tp::LocalMuteState &state)
{
    if (mPriv->state != state) {
        mPriv->state = state;
        QMetaObject::invokeMethod(mPriv->adaptee, "muteStateChanged", Q_ARG(uint, state));
    }
}

void BaseCallMuteInterface::setSetMuteStateCallback(const SetMuteStateCallback &cb)
{
    mPriv->setMuteStateCB = cb;
}

/**
 * Class destructor.
 */
BaseCallMuteInterface::~BaseCallMuteInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseCallMuteInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseCallMuteInterface::createAdaptor()
{
    (void) new Service::CallInterfaceMuteAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

// Call.Content.Interface.DTMF
BaseCallContentDTMFInterface::Adaptee::Adaptee(BaseCallContentDTMFInterface *interface)
    : QObject(interface),
      mInterface(interface)
{
}

BaseCallContentDTMFInterface::Adaptee::~Adaptee()
{
}

struct TP_QT_NO_EXPORT BaseCallContentDTMFInterface::Private {
    Private(BaseCallContentDTMFInterface *parent)
        : currentlySendingTones(false),
          adaptee(new BaseCallContentDTMFInterface::Adaptee(parent)) {
    }

    StartToneCallback startToneCB;
    StopToneCallback stopToneCB;
    MultipleTonesCallback multipleTonesCB;
    bool currentlySendingTones;
    QString deferredTones;
    BaseCallContentDTMFInterface::Adaptee *adaptee;
};

void BaseCallContentDTMFInterface::Adaptee::startTone(uchar event, const Tp::Service::CallContentInterfaceDTMFAdaptor::StartToneContextPtr &context)
{
    if (!mInterface->mPriv->startToneCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->startToneCB(event, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

void BaseCallContentDTMFInterface::Adaptee::stopTone(const Tp::Service::CallContentInterfaceDTMFAdaptor::StopToneContextPtr &context)
{
    if (!mInterface->mPriv->stopToneCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->stopToneCB(&error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}


void BaseCallContentDTMFInterface::Adaptee::multipleTones(const QString& tones, const Tp::Service::CallContentInterfaceDTMFAdaptor::MultipleTonesContextPtr &context)
{
    if (!mInterface->mPriv->multipleTonesCB.isValid()) {
        context->setFinishedWithError(TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
        return;
    }

    DBusError error;
    mInterface->mPriv->multipleTonesCB(tones, &error);
    if (error.isValid()) {
        context->setFinishedWithError(error.name(), error.message());
        return;
    }
    context->setFinished();
}

/**
 * \class BaseCallContentDTMFInterface
 * \ingroup servicechannel
 * \headerfile TelepathyQt/base-call.h <TelepathyQt/BaseCall>
 *
 * \brief Base class for implementations of Call.Content.Interface.DTMF
 *
 */

/**
 * Class constructor.
 */
BaseCallContentDTMFInterface::BaseCallContentDTMFInterface()
    : AbstractCallContentInterface(TP_QT_IFACE_CALL_CONTENT_INTERFACE_DTMF),
      mPriv(new Private(this))
{
}

bool BaseCallContentDTMFInterface::currentlySendingTones() const
{
    return mPriv->currentlySendingTones;
}

void BaseCallContentDTMFInterface::setCurrentlySendingTones(bool sending)
{
    mPriv->currentlySendingTones = sending;
}

QString BaseCallContentDTMFInterface::deferredTones() const
{
    return mPriv->deferredTones;
}

void BaseCallContentDTMFInterface::setDeferredTones(const QString &tones)
{
    mPriv->deferredTones = tones;
}


void BaseCallContentDTMFInterface::setStartToneCallback(const StartToneCallback &cb)
{
    mPriv->startToneCB = cb;
}

void BaseCallContentDTMFInterface::setStopToneCallback(const StopToneCallback &cb)
{
    mPriv->stopToneCB = cb;
}


void BaseCallContentDTMFInterface::setMultipleTonesCallback(const MultipleTonesCallback &cb)
{
    mPriv->multipleTonesCB = cb;
}

/**
 * Class destructor.
 */
BaseCallContentDTMFInterface::~BaseCallContentDTMFInterface()
{
    delete mPriv;
}

/**
 * Return the immutable properties of this interface.
 *
 * Immutable properties cannot change after the interface has been registered
 * on a service on the bus with registerInterface().
 *
 * \return The immutable properties of this interface.
 */
QVariantMap BaseCallContentDTMFInterface::immutableProperties() const
{
    QVariantMap map;
    return map;
}

void BaseCallContentDTMFInterface::createAdaptor()
{
    (void) new Service::CallContentInterfaceDTMFAdaptor(dbusObject()->dbusConnection(),
            mPriv->adaptee, dbusObject());
}

}

