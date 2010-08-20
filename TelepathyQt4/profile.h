/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2010 Nokia Corporation
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

#ifndef _TelepathyQt4_profile_h_HEADER_GUARD_
#define _TelepathyQt4_profile_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Types>

#include <QDBusSignature>
#include <QObject>
#include <QString>
#include <QVariant>

namespace Tp
{

class TELEPATHY_QT4_EXPORT Profile : public RefCounted
{
    Q_DISABLE_COPY(Profile);

public:
    static ProfilePtr createForServiceName(const QString &serviceName);
    static ProfilePtr createForFileName(const QString &fileName);

    ~Profile();

    QString serviceName() const;

    bool isValid() const;

    QString type() const;
    QString provider() const;
    QString name() const;
    QString iconName() const;
    QString cmName() const;
    QString protocolName() const;

    class Parameter
    {
    public:
        Parameter();
        Parameter(const Parameter &other);
        Parameter(const QString &name,
                  const QDBusSignature &dbusSignature,
                  const QVariant &value,
                  const QString &label,
                  bool mandatory);
        ~Parameter();

        QString name() const;
        QDBusSignature dbusSignature() const;
        QVariant::Type type() const;
        QVariant value() const;
        QString label() const;

        bool isMandatory() const;

        Parameter &operator=(const Parameter &other);

    private:
        friend class Profile;

        void setName(const QString &name);
        void setDBusSignature(const QDBusSignature &dbusSignature);
        void setValue(const QVariant &value);
        void setLabel(const QString &label);
        void setMandatory(bool mandatory);

        struct Private;
        friend struct Private;
        Private *mPriv;
    };
    typedef QList<Parameter> ParameterList;

    ParameterList parameters() const;
    bool hasParameter(const QString &name) const;
    Parameter parameter(const QString &name) const;

    class Presence
    {
    public:
        Presence();
        Presence(const Presence &other);
        Presence(const QString &id,
                 const QString &label,
                 const QString &iconName,
                 const QString &message,
                 bool disabled);
        ~Presence();

        QString id() const;
        QString label() const;
        QString iconName() const;
        QString message() const;

        bool isDisabled() const;

        Presence &operator=(const Presence &other);

    private:
        friend class Profile;

        void setId(const QString &id);
        void setLabel(const QString &label);
        void setIconName(const QString &iconName);
        void setMessage(const QString &message);
        void setDisabled(bool disabled);

        struct Private;
        friend struct Private;
        Private *mPriv;
    };
    typedef QList<Presence> PresenceList;

    bool allowOthersPresences() const;
    PresenceList presences() const;
    bool hasPresence(const QString &id) const;
    Presence presence(const QString &id) const;

    RequestableChannelClassList unsupportedChannelClasses() const;

private:
    friend class ProfileManager;

    Profile();

    void setServiceName(const QString &serviceName);
    void setFileName(const QString &fileName);

    static QStringList searchDirs();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
