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

#ifndef _TelepathyQt_protocol_parameter_h_HEADER_GUARD_
#define _TelepathyQt_protocol_parameter_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>

#include <QDBusSignature>
#include <QSharedDataPointer>
#include <QString>
#include <QVariant>

namespace Tp
{

class TP_QT_EXPORT ProtocolParameter
{
public:
    ProtocolParameter();
    ProtocolParameter(const ParamSpec &spec);
    ProtocolParameter(const QString &name,
                      const QDBusSignature &dbusSignature,
                      ConnMgrParamFlags flags,
                      QVariant defaultValue = QVariant());
    ProtocolParameter(const QString &name,
                      const QString &dbusSignature,
                      ConnMgrParamFlags flags,
                      QVariant defaultValue = QVariant());
    ProtocolParameter(const ProtocolParameter &other);
    ~ProtocolParameter();

    bool isValid() const { return mPriv.constData() != 0; }

    ProtocolParameter &operator=(const ProtocolParameter &other);
    bool operator==(const ProtocolParameter &other) const;
    bool operator==(const QString &name) const;
    bool operator<(const ProtocolParameter &other) const;

    QString name() const;
    QDBusSignature dbusSignature() const;
    QVariant::Type type() const;
    QVariant defaultValue() const;

    bool isRequired() const;
    bool isSecret() const;
    bool isRequiredForRegistration() const;

    ParamSpec bareParameter() const;

private:
    friend class ConnectionManager;
    friend class ProtocolInfo;

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

typedef QList<ProtocolParameter> ProtocolParameterList;

uint qHash(const ProtocolParameter &parameter);

} // Tp

Q_DECLARE_METATYPE(Tp::ProtocolParameter);
Q_DECLARE_METATYPE(Tp::ProtocolParameterList);

#endif
