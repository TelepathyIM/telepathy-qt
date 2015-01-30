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

#ifndef _TelepathyQt_base_call_h_HEADER_GUARD_
#define _TelepathyQt_base_call_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif


#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>
#include <TelepathyQt/Callbacks>
#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>

#include <QDBusConnection>

class QString;

namespace Tp
{

class TP_QT_EXPORT AbstractCallContentInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractCallContentInterface)

public:
    AbstractCallContentInterface(const QString &interfaceName);
    virtual ~AbstractCallContentInterface();

private:
    friend class BaseCallContent;

    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseCallContent : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseCallContent)

public:
    static BaseCallContentPtr create(const QDBusConnection &dbusConnection,
                                     BaseChannel* channel,
                                     const QString &name,
                                     const Tp::MediaStreamType &type,
                                     const Tp::MediaStreamDirection &direction) {
        return BaseCallContentPtr(new BaseCallContent(dbusConnection, channel, name, type, direction));
    }

    virtual ~BaseCallContent();
    QVariantMap immutableProperties() const;
    bool registerObject(DBusError *error = NULL);
    virtual QString uniqueName() const;

    QList<AbstractCallContentInterfacePtr> interfaces() const;
    AbstractCallContentInterfacePtr interface(const QString &interfaceName) const;
    bool plugInterface(const AbstractCallContentInterfacePtr &interface);

    QString name() const;
    Tp::MediaStreamType type() const;
    Tp::CallContentDisposition disposition() const;
    Tp::ObjectPathList streams() const;
protected:
    BaseCallContent(const QDBusConnection &dbusConnection,
                    BaseChannel* channel,
                    const QString &name,
                    const Tp::MediaStreamType &type,
                    const Tp::MediaStreamDirection &direction);

    virtual bool registerObject(const QString &busName, const QString &objectPath,
                                DBusError *error);
    void remove();

private:
    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseCallMuteInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseCallMuteInterface)

public:
    static BaseCallMuteInterfacePtr create() {
        return BaseCallMuteInterfacePtr(new BaseCallMuteInterface());
    }
    template<typename BaseCallMuteInterfaceSubclass>
    static SharedPtr<BaseCallMuteInterfaceSubclass> create() {
        return SharedPtr<BaseCallMuteInterfaceSubclass>(
                   new BaseCallMuteInterfaceSubclass());
    }
    virtual ~BaseCallMuteInterface();

    QVariantMap immutableProperties() const;

    Tp::LocalMuteState localMuteState() const;
    void setMuteState(const Tp::LocalMuteState &state);

    typedef Callback2<void, const Tp::LocalMuteState&, DBusError*> SetMuteStateCallback;
    void setSetMuteStateCallback(const SetMuteStateCallback &cb);
Q_SIGNALS:
    void muteStateChanged(const Tp::LocalMuteState &state);
private:
    BaseCallMuteInterface();
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseCallContentDTMFInterface : public AbstractCallContentInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseCallContentDTMFInterface)

public:
    static BaseCallContentDTMFInterfacePtr create() {
        return BaseCallContentDTMFInterfacePtr(new BaseCallContentDTMFInterface());
    }
    template<typename BaseCallContentDTMFInterfaceSubclass>
    static SharedPtr<BaseCallContentDTMFInterfaceSubclass> create() {
        return SharedPtr<BaseCallContentDTMFInterfaceSubclass>(
                   new BaseCallContentDTMFInterfaceSubclass());
    }
    virtual ~BaseCallContentDTMFInterface();

    QVariantMap immutableProperties() const;

    bool currentlySendingTones() const;
    void setCurrentlySendingTones(bool sendingTones);

    QString deferredTones() const;
    void setDeferredTones(const QString &deferredTones);


    typedef Callback2<void, uchar, DBusError*> StartToneCallback;
    void setStartToneCallback(const StartToneCallback &cb);
    typedef Callback1<void, DBusError*> StopToneCallback;
    void setStopToneCallback(const StopToneCallback &cb);
    typedef Callback2<void, const QString&, DBusError*> MultipleTonesCallback;
    void setMultipleTonesCallback(const MultipleTonesCallback &cb);
Q_SIGNALS:

private:
    BaseCallContentDTMFInterface();
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};


}
#endif
