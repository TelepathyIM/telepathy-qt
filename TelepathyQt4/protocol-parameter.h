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

#ifndef _TelepathyQt4_protocol_parameter_h_HEADER_GUARD_
#define _TelepathyQt4_protocol_parameter_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Global>

#include <QDBusSignature>
#include <QSharedDataPointer>
#include <QString>
#include <QVariant>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ProtocolParameter
{
public:
    ProtocolParameter();
    ProtocolParameter(const ProtocolParameter &other);
    ~ProtocolParameter();

    bool isValid() const { return mPriv.constData() != 0; }

    ProtocolParameter &operator=(const ProtocolParameter &other);
    bool operator==(const ProtocolParameter &other) const;
    bool operator==(const QString &name) const;

    QString name() const;
    QDBusSignature dbusSignature() const;
    QVariant::Type type() const;
    QVariant defaultValue() const;

    bool isRequired() const;
    bool isSecret() const;
    bool isRequiredForRegistration() const;

private:
    friend class ConnectionManager;
    friend class ProtocolInfo;

    ProtocolParameter(const QString &name,
                      const QDBusSignature &dbusSignature,
                      QVariant defaultValue,
                      ConnMgrParamFlag flags);

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

typedef QList<ProtocolParameter> ProtocolParameterList;

} // Tp

Q_DECLARE_METATYPE(Tp::ProtocolParameter);
Q_DECLARE_METATYPE(Tp::ProtocolParameterList);

#endif
