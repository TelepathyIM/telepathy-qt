/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
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

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/KeyFile>
#include <TelepathyQt4/Utils>

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QDBusVariant>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ManagerFile::Private
{
    Private(const QString &cnName);

    void init();
    bool parse(const QString &fileName);
    bool isValid() const;

    bool hasParameter(const QString &protocol, const QString &paramName) const;
    ParamSpec *getParameter(const QString &protocol, const QString &paramName);
    QStringList protocols() const;
    ParamSpecList parameters(const QString &protocol) const;

    QVariant valueForKey(const QString &param, const QString &signature);

    struct ProtocolInfo
    {
        ProtocolInfo() {}
        ProtocolInfo(const ParamSpecList &params)
            : params(params)
        {
        }

        ParamSpecList params;
        QString vcardField;
        QString englishName;
        QString iconName;
        RequestableChannelClassList rccs;
    };

    QString cmName;
    KeyFile keyFile;
    QHash<QString, ProtocolInfo> protocolsMap;
    bool valid;
};

ManagerFile::Private::Private(const QString &cmName)
    : cmName(cmName),
      valid(false)
{
    init();
}

void ManagerFile::Private::init()
{
    // TODO: should we cache the configDirs anywhere?
    QStringList configDirs;

    QString xdgDataHome = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
    if (xdgDataHome.isEmpty()) {
        configDirs << QDir::homePath() + QLatin1String("/.local/share/data/telepathy/managers/");
    }
    else {
        configDirs << xdgDataHome + QLatin1String("/telepathy/managers/");
    }

    QString xdgDataDirsEnv = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS"));
    if (xdgDataDirsEnv.isEmpty()) {
        configDirs << QLatin1String("/usr/local/share/telepathy/managers/");
        configDirs << QLatin1String("/usr/share/telepathy/managers/");
    }
    else {
        QStringList xdgDataDirs = xdgDataDirsEnv.split(QLatin1Char(':'));
        foreach (const QString xdgDataDir, xdgDataDirs) {
            configDirs << xdgDataDir + QLatin1String("/telepathy/managers/");
        }
    }

    foreach (const QString configDir, configDirs) {
        QString fileName = configDir + cmName + QLatin1String(".manager");
        if (QFile::exists(fileName)) {
            debug() << "parsing manager file" << fileName;
            protocolsMap.clear();
            if (!parse(fileName)) {
                warning() << "error parsing manager file" << fileName;
                continue;
            }
            valid = true;
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
    foreach (const QString group, groups) {
        if (group.startsWith(QLatin1String("Protocol "))) {
            protocol = group.right(group.length() - 9);
            keyFile.setGroup(group);

            ParamSpecList paramSpecList;
            QString param;
            QStringList params = keyFile.keys();
            foreach (param, params) {
                ParamSpec spec;
                spec.flags = 0;
                if (param.startsWith(QLatin1String("param-"))) {
                    spec.name = param.right(param.length() - 6);

                    if (spec.name.endsWith(QLatin1String("password"))) {
                        spec.flags |= ConnMgrParamFlagSecret;
                    }

                    QStringList values = keyFile.value(param).split(QLatin1String(" "));

                    spec.signature = values[0];
                    if (values.contains(QLatin1String("secret"))) {
                        spec.flags |= ConnMgrParamFlagSecret;
                    }
                    if (values.contains(QLatin1String("dbus-property"))) {
                        spec.flags |= ConnMgrParamFlagDBusProperty;
                    }
                    if (values.contains(QLatin1String("required"))) {
                        spec.flags |= ConnMgrParamFlagRequired;
                    }
                    if (values.contains(QLatin1String("register"))) {
                        spec.flags |= ConnMgrParamFlagRegister;
                    }

                    paramSpecList.append(spec);
                }
            }

            protocolsMap.insert(protocol, ProtocolInfo(paramSpecList));

            /* now that we have all param-* created, let's find their default values */
            foreach (param, params) {
                if (param.startsWith(QLatin1String("default-"))) {
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
                    if (value.type() == QVariant::Invalid) {
                        warning() << "param" << paramName
                                  << "has invalid signature";
                        protocolsMap.clear();
                        return false;
                    }
                    spec->defaultValue = QDBusVariant(value);
                }
            }

            ProtocolInfo &info = protocolsMap[protocol];
            info.vcardField = keyFile.value(QLatin1String("VCardField"));
            info.englishName = keyFile.value(QLatin1String("EnglishName"));
            if (info.englishName.isEmpty()) {
                QStringList words = protocol.split(QLatin1Char('-'));
                for (int i = 0; i < words.size(); ++i) {
                    words[i][0] = words[i].at(0).toUpper();
                }
                info.englishName = words.join(QLatin1String(" "));
            }

            info.iconName = keyFile.value(QLatin1String("Icon"));
            if (info.iconName.isEmpty()) {
                info.iconName = QString(QLatin1String("im-%1")).arg(protocol);
            }

            QStringList rccGroups = keyFile.valueAsStringList(
                    QLatin1String("RequestableChannelClasses"));
            RequestableChannelClass rcc;
            foreach (const QString &rccGroup, rccGroups) {
                keyFile.setGroup(rccGroup);

                foreach (const QString &key, keyFile.keys()) {
                    int spaceIdx = key.indexOf(QLatin1String(" "));
                    if (spaceIdx == -1) {
                        continue;
                    }

                    QString propertyName = key.mid(0, spaceIdx);
                    QString signature = key.mid(spaceIdx + 1);
                    QString param = keyFile.value(key);
                    QVariant value = valueForKey(key, signature);
                    rcc.fixedProperties.insert(propertyName, value);
                }

                rcc.allowedProperties = keyFile.valueAsStringList(
                        QLatin1String("allowed"));

                info.rccs.append(rcc);
            }
        }
    }

    return true;
}

bool ManagerFile::Private::isValid() const
{
    return ((keyFile.status() == KeyFile::NoError) && (valid));
}

bool ManagerFile::Private::hasParameter(const QString &protocol,
                                        const QString &paramName) const
{
    ParamSpecList paramSpecList = protocolsMap[protocol].params;
    foreach (const ParamSpec &paramSpec, paramSpecList) {
        if (paramSpec.name == paramName) {
            return true;
        }
    }
    return false;
}

ParamSpec *ManagerFile::Private::getParameter(const QString &protocol,
                                              const QString &paramName)
{
    ParamSpecList &paramSpecList = protocolsMap[protocol].params;
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
    return protocolsMap.keys();
}

ParamSpecList ManagerFile::Private::parameters(const QString &protocol) const
{
    return protocolsMap.value(protocol).params;
}

QVariant ManagerFile::Private::valueForKey(const QString &param,
                                           const QString &signature)
{
    QString value = keyFile.value(param);
    return variantFromValueWithDBusSignature(value, QDBusSignature(signature));
}


/**
 * \class ManagerFile
 * \headerfile TelepathyQt4/manager-file.h <TelepathyQt4/ManagerFile>
 *
 * \brief The ManagerFile class provides an easy way to read telepathy manager
 * files according to http://telepathy.freedesktop.org/spec.html.
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
 * Return a list of parameters for the given \a protocol.
 *
 * \param protocol Name of the protocol to look for.
 * \return List of ParamSpec of a specific protocol defined in the file, or an
 *         empty list if the protocol is not defined.
 */
ParamSpecList ManagerFile::parameters(const QString &protocol) const
{
    return mPriv->parameters(protocol);
}

/**
 * Return the name of the most common vCard field used for the given \a protocol's
 * contact identifiers, normalized to lower case.
 *
 * \param protocol Name of the protocol to look for.
 * \return The most common vCard field used for the given protocol's contact
 *         identifiers, or an empty string if there is no such field or the
 *         protocol is not defined.
 */
QString ManagerFile::vcardField(const QString &protocol) const
{
    return mPriv->protocolsMap[protocol].vcardField;
}

/**
 * Return the English-language name of the given \a protocol, such as "AIM" or "Yahoo!".
 *
 * The name can be used as a fallback if an application doesn't have a localized name for the
 * protocol.
 *
 * If the manager file doesn't specify the english name, it is inferred from the protocol name, such
 * that for example "google-talk" becomes "Google Talk", but "local-xmpp" becomes "Local Xmpp".
 *
 * \param protocol Name of the protocol to look for.
 * \return An English-language name for the given \a protocol.
 */
QString ManagerFile::englishName(const QString &protocol) const
{
    return mPriv->protocolsMap[protocol].englishName;
}

/**
 * Return the name of an icon for the given \a protocol in the system's icon
 * theme, such as "im-msn".
 *
 * If the manager file doesn't specify the icon name, "im-<protocolname>" is assumed.
 *
 * \param protocol Name of the protocol to look for.
 * \return The likely name of an icon for the given \a protocol.
 */
QString ManagerFile::iconName(const QString &protocol) const
{
    return mPriv->protocolsMap[protocol].iconName;
}

/**
 * Return a list of channel classes which might be requestable from a connection
 * to the given \a protocol.
 *
 * \param protocol Name of the protocol to look for.
 * \return A list of channel classes which might be requestable from a
 *         connection to the given \a protocol or a default constructed
 *         RequestableChannelClassList instance if the protocol is not defined.
 */
RequestableChannelClassList ManagerFile::requestableChannelClasses(
        const QString &protocol) const
{
    return mPriv->protocolsMap[protocol].rccs;
}

QVariant::Type ManagerFile::variantTypeFromDBusSignature(const QString &signature)
{
    QVariant::Type type;
    if (signature == QLatin1String("b")) {
        type = QVariant::Bool;
    } else if (signature == QLatin1String("n") || signature == QLatin1String("i")) {
        type = QVariant::Int;
    } else if (signature == QLatin1String("q") || signature == QLatin1String("u")) {
        type = QVariant::UInt;
    } else if (signature == QLatin1String("x")) {
        type = QVariant::LongLong;
    } else if (signature == QLatin1String("t")) {
        type = QVariant::ULongLong;
    } else if (signature == QLatin1String("d")) {
        type = QVariant::Double;
    } else if (signature == QLatin1String("as")) {
        type = QVariant::StringList;
    } else if (signature == QLatin1String("s") || signature == QLatin1String("o")) {
        type = QVariant::String;
    } else {
        type = QVariant::Invalid;
    }

    return type;
}

} // Tp
