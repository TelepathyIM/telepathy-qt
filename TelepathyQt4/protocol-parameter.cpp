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

#include <TelepathyQt4/ProtocolParameter>

#include <TelepathyQt4/ManagerFile>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ProtocolParameter::Private : public QSharedData
{
    Private(const QString &name, const QDBusSignature &dbusSignature, QVariant::Type type,
            const QVariant &defaultValue, ConnMgrParamFlag flags)
        : name(name), dbusSignature(dbusSignature), type(type), defaultValue(defaultValue),
          flags(flags) {}

    QString name;
    QDBusSignature dbusSignature;
    QVariant::Type type;
    QVariant defaultValue;
    ConnMgrParamFlag flags;
};

ProtocolParameter::ProtocolParameter()
{
}

ProtocolParameter::ProtocolParameter(const QString &name,
                                     const QDBusSignature &dbusSignature,
                                     QVariant defaultValue,
                                     ConnMgrParamFlag flags)
    : mPriv(new Private(name, dbusSignature,
                ManagerFile::variantTypeFromDBusSignature(dbusSignature.signature()), defaultValue,
                flags))
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

    return (mPriv->name == other.name());
}

bool ProtocolParameter::operator==(const QString &name) const
{
    if (!isValid()) {
        return false;
    }

    return (mPriv->name == name);
}

QString ProtocolParameter::name() const
{
    if (!isValid()) {
        return QString();
    }

    return mPriv->name;
}

QDBusSignature ProtocolParameter::dbusSignature() const
{
    if (!isValid()) {
        return QDBusSignature();
    }

    return mPriv->dbusSignature;
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

    return mPriv->defaultValue;
}

bool ProtocolParameter::isRequired() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->flags & ConnMgrParamFlagRequired;
}

bool ProtocolParameter::isSecret() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->flags & ConnMgrParamFlagSecret;
}

bool ProtocolParameter::isRequiredForRegistration() const
{
    if (!isValid()) {
        return false;
    }

    return mPriv->flags & ConnMgrParamFlagRegister;
}

} // Tp
