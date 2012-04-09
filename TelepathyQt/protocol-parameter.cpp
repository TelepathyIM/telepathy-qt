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

#include <TelepathyQt/ProtocolParameter>

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Utils>

namespace Tp
{

struct TP_QT_NO_EXPORT ProtocolParameter::Private : public QSharedData
{
    Private(const ParamSpec &sp)
        : spec(sp),
          type(variantTypeFromDBusSignature(spec.signature))
    {
        init();
    }

    Private(const QString &name, const QString &dbusSignature,
            ConnMgrParamFlags flags, const QVariant &defaultValue)
        : type(variantTypeFromDBusSignature(dbusSignature))
    {
        spec.name = name;
        spec.flags = flags;
        spec.signature = dbusSignature;
        spec.defaultValue = QDBusVariant(defaultValue);
        init();
    }

    void init()
    {
        if (spec.flags & ConnMgrParamFlagHasDefault) {
            if (spec.defaultValue.variant() == QVariant::Invalid) {
                // flags contains HasDefault but no default value is passed,
                // lets warn and build a default value from signature
                warning() << "Building ProtocolParameter with flags containing ConnMgrParamFlagHasDefault"
                    " and no default value, generating a dummy one from signature";
                spec.defaultValue = QDBusVariant(
                        parseValueWithDBusSignature(QString(), spec.signature));
            }
        } else {
            if (spec.defaultValue.variant() != QVariant::Invalid) {
                // flags does not contain HasDefault but a default value is passed,
                // lets add HasDefault to flags
                debug() << "Building ProtocolParameter with flags not containing ConnMgrParamFlagHasDefault"
                    " and a default value, updating flags to contain ConnMgrParamFlagHasDefault";
                spec.flags |= ConnMgrParamFlagHasDefault;
            }
        }
    }

    ParamSpec spec;
    QVariant::Type type;
};

/**
 * \class ProtocolParameter
 * \ingroup clientcm
 * \headerfile TelepathyQt/protocol-parameter.h <TelepathyQt/ProtocolParameter>
 *
 * \brief The ProtocolParameter class represents a Telepathy protocol parameter.
 */


ProtocolParameter::ProtocolParameter()
{
}

ProtocolParameter::ProtocolParameter(const ParamSpec &spec)
    : mPriv(new Private(spec))
{
}

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     ConnMgrParamFlags flags,
                                     QVariant defaultValue)
    : mPriv(new Private(name, dbusSignature.signature(), flags, defaultValue))
{
}

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QString &dbusSignature,
                                     ConnMgrParamFlags flags,
                                     QVariant defaultValue)
    : mPriv(new Private(name, dbusSignature, flags, defaultValue))
{
}

ProtocolParameter::ProtocolParameter(const ProtocolParameter &other)
    : mPriv(other.mPriv)
{
}

ProtocolParameter::~ProtocolParameter()
{
}

ProtocolParameter &ProtocolParameter::operator=(const ProtocolParameter &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

bool ProtocolParameter::operator==(const ProtocolParameter &other) const
{
    if (!isValid() || !other.isValid()) {
        if (!isValid() && !other.isValid()) {
            return true;
        }
        return false;
    }

    return (mPriv->spec.name == other.name());
}

bool ProtocolParameter::operator==(const QString &name) const
{
    if (!isValid()) {
        return false;
    }

    return (mPriv->spec.name == name);
}

bool ProtocolParameter::operator<(const Tp::ProtocolParameter& other) const
{
    return mPriv->spec.name < other.name();
}

QString ProtocolParameter::name() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->spec.name;
}

QDBusSignature ProtocolParameter::dbusSignature() const
{
    if (!isValid()) {
        return QDBusSignature();
    }

    return QDBusSignature(mPriv->spec.signature);
}

QVariant::Type ProtocolParameter::type() const
{
    if (!isValid()) {
        return QVariant::Invalid;
    }

    return mPriv->type;
}

QVariant ProtocolParameter::defaultValue() const
{
    if (!isValid()) {
        return QVariant();
    }

    return mPriv->spec.defaultValue.variant();
}

bool ProtocolParameter::isRequired() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->spec.flags & ConnMgrParamFlagRequired;
}

bool ProtocolParameter::isSecret() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->spec.flags & ConnMgrParamFlagSecret;
}

bool ProtocolParameter::isRequiredForRegistration() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->spec.flags & ConnMgrParamFlagRegister;
}

ParamSpec ProtocolParameter::bareParameter() const
{
    if (!isValid()) {
        return ParamSpec();
    }

    return mPriv->spec;
}

uint qHash(const ProtocolParameter& parameter)
{
    return qHash(parameter.name());
}

} // Tp
