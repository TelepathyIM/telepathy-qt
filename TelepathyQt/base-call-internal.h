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

#include "TelepathyQt/_gen/svc-call.h"

#include <TelepathyQt/Global>
#include <TelepathyQt/MethodInvocationContext>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>
#include "TelepathyQt/debug-internal.h"


namespace Tp
{

class TP_QT_NO_EXPORT BaseCallContent::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(uint type READ type)
    Q_PROPERTY(uint disposition READ disposition)
    Q_PROPERTY(Tp::ObjectPathList streams READ streams)
    Q_PROPERTY(QStringList interfaces READ interfaces)

public:
    Adaptee(const QDBusConnection &dbusConnection, BaseCallContent *content);
    ~Adaptee();
    QStringList interfaces() const;

    QString name() const {
        return mContent->name();
    }

    uint type() const {
        return mContent->type();
    }

    uint disposition() const {
        return mContent->disposition();
    }

    Tp::ObjectPathList streams() const {
        return mContent->streams();
    }

public Q_SLOTS:
    void remove(const Tp::Service::CallContentAdaptor::RemoveContextPtr &context);


Q_SIGNALS:
    void streamsAdded(const Tp::ObjectPathList &streams);
    void streamsRemoved(const Tp::ObjectPathList &streams, const Tp::CallStateReason &reason);

private:
    BaseCallContent *mContent;
    Service::CallContentAdaptor *mAdaptor;
};


class TP_QT_NO_EXPORT BaseCallMuteInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint localMuteState READ localMuteState)

public:
    Adaptee(BaseCallMuteInterface *content);
    ~Adaptee();
    QStringList interfaces() const;

    uint localMuteState() const {
        return mInterface->localMuteState();
    }

public Q_SLOTS:
    void requestMuted(bool muted, const Tp::Service::CallInterfaceMuteAdaptor::RequestMutedContextPtr &context);

Q_SIGNALS:
    void muteStateChanged(uint muteState);

public:
    BaseCallMuteInterface *mInterface;
    Service::CallInterfaceMuteAdaptor *mAdaptor;
};

class TP_QT_NO_EXPORT BaseCallContentDTMFInterface::Adaptee : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool currentlySendingTones READ currentlySendingTones)
    Q_PROPERTY(QString deferredTones READ deferredTones)

public:
    Adaptee(BaseCallContentDTMFInterface *interface);
    ~Adaptee();
    QStringList interfaces() const;

    bool currentlySendingTones() const {
        return mInterface->currentlySendingTones();
    }
    QString deferredTones() const {
        return mInterface->deferredTones();
    }

public Q_SLOTS:
    void multipleTones(const QString& tones, const Tp::Service::CallContentInterfaceDTMFAdaptor::MultipleTonesContextPtr &context);
    void startTone(uchar event, const Tp::Service::CallContentInterfaceDTMFAdaptor::StartToneContextPtr &context);
    void stopTone(const Tp::Service::CallContentInterfaceDTMFAdaptor::StopToneContextPtr &context);
Q_SIGNALS:
    void tonesDeferred(const QString& tones);
    void sendingTones(const QString& tones);
    void stoppedTones(bool cancelled);

public:
    BaseCallContentDTMFInterface *mInterface;
    Service::CallContentInterfaceDTMFAdaptor *mAdaptor;
};

}
