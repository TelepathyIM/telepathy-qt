/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2010 Nokia Corporation
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

#include "TelepathyQt/manager-file.h"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/key-file.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/Utils>

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QDBusVariant>

namespace Tp
{

struct TP_QT_NO_EXPORT ManagerFile::Private
{
    Private();
    Private(const QString &cnName);

    void init();
    bool parse(const QString &fileName);
    bool isValid() const;

    bool hasParameter(const QString &protocol, const QString &paramName) const;
    ParamSpec *getParameter(const QString &protocol, const QString &paramName);
    QStringList protocols() const;
    ParamSpecList parameters(const QString &protocol) const;

    QVariant valueForKey(const QString &param, const QString &dbusSignature);

    struct ProtocolInfo
    {
        ProtocolInfo() {}
        ProtocolInfo(const ParamSpecList &params, const PresenceSpecList &statuses)
            : params(params),
              statuses(statuses)
        {
        }

        ParamSpecList params;
        QString vcardField;
        QString englishName;
        QString iconName;
        RequestableChannelClassList rccs;
        PresenceSpecList statuses;
        AvatarSpec avatarRequirements;
        QStringList addressableVCardFields;
        QStringList addressableUriSchemes;
    };

    QString cmName;
    KeyFile keyFile;
    QHash<QString, ProtocolInfo> protocolsMap;
    bool valid;
};

ManagerFile::Private::Private()
    : valid(false)
{
}

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
            SimpleStatusSpecMap statuses;
            QString param;
            QStringList params = keyFile.keys();
            foreach (param, params) {
                ParamSpec spec;
                SimpleStatusSpec status;
                spec.flags = 0;

                QStringList values = keyFile.value(param).split(QLatin1String(" "));

                if (param.startsWith(QLatin1String("param-"))) {
                    spec.name = param.right(param.length() - 6);

                    if (values.length() == 0) {
                        warning() << "param" << spec.name << "set but no signature defined";
                        return false;
                    }

                    if (spec.name.endsWith(QLatin1String("password"))) {
                        spec.flags |= ConnMgrParamFlagSecret;
                    }

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
                } else if (param.startsWith(QLatin1String("status-"))) {
                    QString statusName = param.right(param.length() - 7);

                    if (values.length() == 0) {
                        warning() << "status" << statusName << "set but no type defined";
                        return false;
                    }

                    bool ok;
                    status.type = values[0].toUInt(&ok);
                    if (!ok) {
                        warning() << "status" << statusName << "set but type is not an uint";
                        return false;
                    }

                    if (values.contains(QLatin1String("settable"))) {
                        status.maySetOnSelf = true;
                    } else {
                        status.maySetOnSelf = false;
                    }

                    if (values.contains(QLatin1String("message"))) {
                        status.canHaveMessage = true;
                    } else {
                        status.canHaveMessage = false;
                    }

                    if (statuses.contains(statusName)) {
                        warning() << "status" << statusName << "defined more than once, "
                            "replacing it";
                    }

                    statuses.insert(statusName, status);
                }
            }

            protocolsMap.insert(protocol, ProtocolInfo(paramSpecList, PresenceSpecList(statuses)));

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

            QStringList supportedMimeTypes = keyFile.valueAsStringList(
                    QLatin1String("SupportedAvatarMIMETypes"));
            uint minHeight = keyFile.value(QLatin1String("MinimumAvatarHeight")).toUInt();
            uint maxHeight = keyFile.value(QLatin1String("MaximumAvatarHeight")).toUInt();
            uint recommendedHeight = keyFile.value(
                    QLatin1String("RecommendedAvatarHeight")).toUInt();
            uint minWidth = keyFile.value(QLatin1String("MinimumAvatarWidth")).toUInt();
            uint maxWidth = keyFile.value(QLatin1String("MaximumAvatarWidth")).toUInt();
            uint recommendedWidth = keyFile.value(
                    QLatin1String("RecommendedAvatarWidth")).toUInt();
            uint maxBytes = keyFile.value(QLatin1String("MaximumAvatarBytes")).toUInt();
            info.avatarRequirements = AvatarSpec(supportedMimeTypes,
                    minHeight, maxHeight, recommendedHeight,
                    minWidth, maxWidth, recommendedWidth,
                    maxBytes);

            info.addressableVCardFields = keyFile.valueAsStringList(
                    QLatin1String("AddressableVCardFields"));
            info.addressableUriSchemes = keyFile.valueAsStringList(
                    QLatin1String("AddressableURISchemes"));

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

                rcc.fixedProperties.clear();
                rcc.allowedProperties.clear();
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
                                           const QString &dbusSignature)
{
    QString value = keyFile.rawValue(param);
    return parseValueWithDBusSignature(value, dbusSignature);
}


/**
 * \class ManagerFile
 * \ingroup utils
 * \headerfile TelepathyQt/manager-file.h <TelepathyQt/ManagerFile>
 *
 * \brief The ManagerFile class provides an easy way to read Telepathy manager
 * files according to the \telepathy_spec.
 */

/**
 * Create a ManagerFile object used to read .manager compliant files.
 */
ManagerFile::ManagerFile()
    : mPriv(new Private())
{
}

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
 * Create a ManagerFile object used to read .manager compliant files.
 */
ManagerFile::ManagerFile(const ManagerFile &other)
    : mPriv(new Private())
{
    mPriv->cmName = other.mPriv->cmName;
    mPriv->keyFile = other.mPriv->keyFile;
    mPriv->protocolsMap = other.mPriv->protocolsMap;
    mPriv->valid = other.mPriv->valid;
}

/**
 * Class destructor.
 */
ManagerFile::~ManagerFile()
{
    delete mPriv;
}

ManagerFile &ManagerFile::operator=(const ManagerFile &other)
{
    mPriv->cmName = other.mPriv->cmName;
    mPriv->keyFile = other.mPriv->keyFile;
    mPriv->protocolsMap = other.mPriv->protocolsMap;
    mPriv->valid = other.mPriv->valid;
    return *this;
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
 * Return the name of the most common vcard field used for the given \a protocol's
 * contact identifiers, normalized to lower case.
 *
 * \param protocol Name of the protocol to look for.
 * \return The most common vcard field used for the given protocol's contact
 *         identifiers, or an empty string if there is no such field or the
 *         protocol is not defined.
 */
QString ManagerFile::vcardField(const QString &protocol) const
{
    return mPriv->protocolsMap.value(protocol).vcardField;
}

QStringList ManagerFile::addressableVCardFields(const QString &protocol) const
{
    return mPriv->protocolsMap.value(protocol).addressableVCardFields;
}

QStringList ManagerFile::addressableUriSchemes(const QString &protocol) const
{
    return mPriv->protocolsMap.value(protocol).addressableUriSchemes;
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
    return mPriv->protocolsMap.value(protocol).englishName;
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
    return mPriv->protocolsMap.value(protocol).iconName;
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
    return mPriv->protocolsMap.value(protocol).rccs;
}

/**
 * Return a list of PresenceSpec representing the possible presence statuses
 * from a connection to the given \a protocol.
 *
 * \param protocol Name of the protocol to look for.
 * \return A list of PresenceSpec representing the possible presence statuses
 *         from a connection to the given \a protocol or an empty list
 *         if the protocol is not defined.
 */
PresenceSpecList ManagerFile::allowedPresenceStatuses(const QString &protocol) const
{
    return mPriv->protocolsMap.value(protocol).statuses;
}

/**
 * Return the requirements (size limits, supported MIME types, etc)
 * for avatars used on the given \a protocol.
 *
 * \param protocol Name of the protocol to look for.
 * \return The requirements for avatars used on the given \a protocol or an invalid
 *         AvatarSpec if the protocol is not defined.
 */
AvatarSpec ManagerFile::avatarRequirements(const QString &protocol) const
{
    return mPriv->protocolsMap.value(protocol).avatarRequirements;
}

} // Tp
