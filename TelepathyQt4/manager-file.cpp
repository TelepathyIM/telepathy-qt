/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt4/ManagerFile>

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QDBusVariant>

#include <TelepathyQt4/KeyFile>
#include <TelepathyQt4/Constants>

#include "TelepathyQt4/debug-internal.h"

namespace Tp
{

struct ManagerFile::Private
{
    QString cmName;
    KeyFile keyFile;
    QHash<QString, ParamSpecList> protocolParams;

    Private(const QString &cnName);

    void init();
    bool parse(const QString &fileName);
    bool isValid() const;

    bool hasParameter(const QString &protocol, const QString &paramName) const;
    ParamSpec *getParameter(const QString &protocol, const QString &paramName);
    QStringList protocols() const;
    ParamSpecList parameters(const QString &protocol) const;

    QVariant valueForKey(const QString &param, const QString &signature);
};

ManagerFile::Private::Private(const QString &cmName)
    : cmName(cmName)
{
    init();
}

void ManagerFile::Private::init()
{
    // TODO: should we cache the configDirs anywhere?
    QStringList configDirs;

    QString xdgDataHome = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
    if (xdgDataHome.isEmpty()) {
        configDirs << QDir::homePath() + "/.local/share/data/telepathy/managers/";
    }
    else {
        configDirs << xdgDataHome + QLatin1String("/telepathy/managers/");
    }

    QString xdgDataDirsEnv = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS"));
    if (xdgDataDirsEnv.isEmpty()) {
        configDirs << "/usr/local/share/telepathy/managers/";
        configDirs << "/usr/share/telepathy/managers/";
    }
    else {
        QStringList xdgDataDirs = xdgDataDirsEnv.split(QLatin1Char(':'));
        Q_FOREACH (const QString xdgDataDir, xdgDataDirs) {
            configDirs << xdgDataDir + QLatin1String("/telepathy/managers/");
        }
    }

    Q_FOREACH (const QString configDir, configDirs) {
        QString fileName = configDir + cmName + QLatin1String(".manager");
        if (QFile::exists(fileName)) {
            debug() << "parsing manager file" << fileName;
            if (!parse(fileName)) {
                warning() << "error parsing manager file" << fileName;
                continue;
            }
            return;
        }
    }
}

bool ManagerFile::Private::parse(const QString &fileName)
{
    keyFile.setFileName(fileName);
    if (keyFile.status() != KeyFile::NoError) {
        return false;
    }

    /* read supported protocols and parameters */
    QString protocol;
    QStringList groups = keyFile.allGroups();
    Q_FOREACH (const QString group, groups) {
        if (group.startsWith("Protocol ")) {
            protocol = group.right(group.length() - 9);
            keyFile.setGroup(group);

            ParamSpecList paramSpecList;
            QString param;
            QStringList params = keyFile.keys();
            Q_FOREACH (param, params) {
                ParamSpec spec;
                spec.flags = 0;
                if (param.startsWith("param-")) {
                    spec.name = param.right(param.length() - 6);

                    if (spec.name.endsWith("password")) {
                        spec.flags |= ConnMgrParamFlagSecret;
                    }

                    QStringList values = keyFile.value(param).split(QChar(' '));

                    spec.signature = values[0];
                    if (values.contains("secret")) {
                        spec.flags |= ConnMgrParamFlagSecret;
                    }
                    if (values.contains("dbus-property")) {
                        spec.flags |= ConnMgrParamFlagDBusProperty;
                    }
                    if (values.contains("required")) {
                        spec.flags |= ConnMgrParamFlagRequired;
                    }
                    if (values.contains("register")) {
                        spec.flags |= ConnMgrParamFlagRegister;
                    }

                    paramSpecList.append(spec);
                }
            }

            protocolParams[protocol] = paramSpecList;

            /* now that we have all param-* created, let's find their default values */
            Q_FOREACH (param, params) {
                if (param.startsWith("default-")) {
                    QString paramName = param.right(param.length() - 8);

                    if (!hasParameter(protocol, paramName)) {
                        warning() << "param" << paramName
                                  << "has default value set, but not a definition";
                        continue;
                    }

                    ParamSpec *spec = getParameter(protocol, paramName);

                    spec->flags |= ConnMgrParamFlagHasDefault;

                    /* map based on the param dbus signature, otherwise use
                     * QString */
                    QVariant value = valueForKey(param, spec->signature);
                    spec->defaultValue = QDBusVariant(value);
                }
            }
        }
    }

    return true;
}

bool ManagerFile::Private::isValid() const
{
    return (keyFile.status() == KeyFile::NoError);
}

bool ManagerFile::Private::hasParameter(const QString &protocol,
                                        const QString &paramName) const
{
    ParamSpecList paramSpecList = protocolParams[protocol];
    Q_FOREACH (ParamSpec paramSpec, paramSpecList) {
        if (paramSpec.name == paramName) {
            return true;
        }
    }
    return false;
}

ParamSpec *ManagerFile::Private::getParameter(const QString &protocol,
                                              const QString &paramName)
{
    ParamSpecList &paramSpecList = protocolParams[protocol];
    for (int i = 0; i < paramSpecList.size(); ++i) {
        ParamSpec &paramSpec = paramSpecList[i];
        if (paramSpec.name == paramName) {
            return &paramSpec;
        }
    }
    return NULL;
}

QStringList ManagerFile::Private::protocols() const
{
    return protocolParams.keys();
}

ParamSpecList ManagerFile::Private::parameters(const QString &protocol) const
{
    return protocolParams.value(protocol);
}

QVariant ManagerFile::Private::valueForKey(const QString &param,
                                           const QString &signature)
{
    QVariant::Type type = ManagerFile::variantTypeFromDBusSignature(signature);

    if (type == QVariant::Invalid) {
        return QVariant(type);
    }

    switch (type) {
        case QVariant::Bool:
            {
                QString value = keyFile.value(param);
                if (value.toLower() == "true" || value == "1") {
                    return QVariant(true);
                }
                else {
                    return QVariant(false);
                }
                break;
            }
        case QVariant::Int:
            {
                QString value = keyFile.value(param);
                return QVariant(value.toInt());
            }
        case QVariant::UInt:
            {
                QString value = keyFile.value(param);
                return QVariant(value.toUInt());
            }
        case QVariant::LongLong:
            {
                QString value = keyFile.value(param);
                return QVariant(value.toLongLong());
            }
        case QVariant::ULongLong:
            {
                QString value = keyFile.value(param);
                return QVariant(value.toULongLong());
            }
        case QVariant::Double:
            {
                QString value = keyFile.value(param);
                return QVariant(value.toDouble());
            }
        case QVariant::StringList:
            {
                QStringList value = keyFile.valueAsStringList(param);
                return QVariant(value);
            }
        default:
            {
                QString value = keyFile.value(param);
                return QVariant(value);
            }
    }
}


/**
 * \class ManagerFile
 * \headerfile <TelepathyQt4/manager-file.h> <TelepathyQt4/ManagerFile>
 *
 * The ManagerFile class provides an easy way to read telepathy manager files
 * according to http://telepathy.freedesktop.org/spec.html.
 */

/**
 * Create a ManagerFile object used to read .manager compliant files.
 *
 * \param cmName Name of the connection manager to read the file for.
 */
ManagerFile::ManagerFile(const QString &cmName)
    : mPriv(new Private(cmName))
{
}

/**
 * Class destructor.
 */
ManagerFile::~ManagerFile()
{
    delete mPriv;
}

/**
 * Check whether or not a ManagerFile object is valid. If the file for the
 * specified connection manager cannot be found it will be considered invalid.
 *
 * \return true if valid, false otherwise.
 */
bool ManagerFile::isValid() const
{
    return mPriv->isValid();
}

/**
 * Return a list of all protocols defined in the manager file.
 *
 * \return List of all protocols defined in the file.
 */
QStringList ManagerFile::protocols() const
{
    return mPriv->protocols();
}

/**
 * Return a list of all protocols defined in the manager file.
 *
 * \param protocol Name of the protocol to look for.
 * \return List of ParamSpec of a specific protocol defined in the file, or an
 *         empty list if the protocol is not defined.
 */
ParamSpecList ManagerFile::parameters(const QString &protocol) const
{
    return mPriv->parameters(protocol);
}

QVariant::Type ManagerFile::variantTypeFromDBusSignature(const QString &signature)
{
    QVariant::Type type;
    if (signature == "b")
        type = QVariant::Bool;
    else if (signature == "n" || signature == "i")
        type = QVariant::Int;
    else if (signature == "q" || signature == "u")
        type = QVariant::UInt;
    else if (signature == "x")
        type = QVariant::LongLong;
    else if (signature == "t")
        type = QVariant::ULongLong;
    else if (signature == "d")
        type = QVariant::Double;
    else if (signature == "as")
        type = QVariant::StringList;
    else if (signature == "s" || signature == "o")
        type = QVariant::String;
    else
        type = QVariant::Invalid;

    return type;
}

} // Tp
