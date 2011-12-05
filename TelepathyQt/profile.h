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

#ifndef _TelepathyQt_profile_h_HEADER_GUARD_
#define _TelepathyQt_profile_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/Types>

#include <QDBusSignature>
#include <QObject>
#include <QString>
#include <QVariant>

namespace Tp
{

class ProtocolInfo;

class TP_QT_EXPORT Profile : public RefCounted
{
    Q_DISABLE_COPY(Profile);

public:
    static ProfilePtr createForServiceName(const QString &serviceName);
    static ProfilePtr createForFileName(const QString &fileName);

    ~Profile();

    QString serviceName() const;

    bool isValid() const;

    bool isFake() const;

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

        // TODO Add matches(Tp::Presence) method

        Parameter &operator=(const Parameter &other);

    private:
        friend class Profile;

        TP_QT_NO_EXPORT void setName(const QString &name);
        TP_QT_NO_EXPORT void setDBusSignature(const QDBusSignature &dbusSignature);
        TP_QT_NO_EXPORT void setValue(const QVariant &value);
        TP_QT_NO_EXPORT void setLabel(const QString &label);
        TP_QT_NO_EXPORT void setMandatory(bool mandatory);

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
        bool canHaveStatusMessage() const;

        bool isDisabled() const;

        Presence &operator=(const Presence &other);

    private:
        friend class Profile;

        TP_QT_NO_EXPORT void setId(const QString &id);
        TP_QT_NO_EXPORT void setLabel(const QString &label);
        TP_QT_NO_EXPORT void setIconName(const QString &iconName);
        TP_QT_NO_EXPORT void setMessage(const QString &message);
        TP_QT_NO_EXPORT void setDisabled(bool disabled);

        struct Private;
        friend struct Private;
        Private *mPriv;
    };
    typedef QList<Presence> PresenceList;

    bool allowOtherPresences() const;
    PresenceList presences() const;
    bool hasPresence(const QString &id) const;
    Presence presence(const QString &id) const;

    RequestableChannelClassSpecList unsupportedChannelClassSpecs() const;

private:
    friend class Account;
    friend class ProfileManager;

    TP_QT_NO_EXPORT Profile();
    TP_QT_NO_EXPORT Profile(const QString &serviceName, const QString &cmName,
            const QString &protocolName, const ProtocolInfo &protocolInfo);

    TP_QT_NO_EXPORT void setServiceName(const QString &serviceName);
    TP_QT_NO_EXPORT void setFileName(const QString &fileName);

    TP_QT_NO_EXPORT static QStringList searchDirs();

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::Profile::Parameter);
Q_DECLARE_METATYPE(Tp::Profile::Presence);

#endif
