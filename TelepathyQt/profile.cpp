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

#include <TelepathyQt/Profile>

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/manager-file.h"

#include <TelepathyQt/ProtocolInfo>
#include <TelepathyQt/ProtocolParameter>
#include <TelepathyQt/Utils>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QXmlAttributes>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

namespace Tp
{

struct TP_QT_NO_EXPORT Profile::Private
{
    Private();

    void setServiceName(const QString &serviceName);
    void setFileName(const QString &fileName);

    void lookupProfile();
    bool parse(QFile *file);
    void invalidate();

    struct Data
    {
        Data();

        void clear();

        QString type;
        QString provider;
        QString name;
        QString iconName;
        QString cmName;
        QString protocolName;
        Profile::ParameterList parameters;
        bool allowOtherPresences;
        Profile::PresenceList presences;
        RequestableChannelClassSpecList unsupportedChannelClassSpecs;
    };

    class XmlHandler;

    QString serviceName;
    bool valid;
    bool fake;
    bool allowNonIMType;
    Data data;
};

Profile::Private::Data::Data()
    : allowOtherPresences(false)
{
}

void Profile::Private::Data::clear()
{
    type = QString();
    provider = QString();
    name = QString();
    iconName = QString();
    protocolName = QString();
    parameters = Profile::ParameterList();
    allowOtherPresences = false;
    presences = Profile::PresenceList();
    unsupportedChannelClassSpecs = RequestableChannelClassSpecList();
}


class TP_QT_NO_EXPORT Profile::Private::XmlHandler :
                public QXmlDefaultHandler
{
public:
    XmlHandler(const QString &serviceName, bool allowNonIMType, Profile::Private::Data *outputData);

    bool startElement(const QString &namespaceURI, const QString &localName,
            const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
            const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    bool attributeValueAsBoolean(const QXmlAttributes &attributes,
            const QString &qName);

    QString mServiceName;
    bool allowNonIMType;
    Profile::Private::Data *mData;
    QStack<QString> mElements;
    QString mCurrentText;
    Profile::Parameter mCurrentParameter;
    RequestableChannelClass mCurrentCC;
    QString mCurrentPropertyName;
    QString mCurrentPropertyType;
    QString mErrorString;
    bool mMetServiceTag;

    static const QString xmlNs;

    static const QString elemService;
    static const QString elemName;
    static const QString elemParams;
    static const QString elemParam;
    static const QString elemPresences;
    static const QString elemPresence;
    static const QString elemUnsupportedCCs;
    static const QString elemCC;
    static const QString elemProperty;

    static const QString elemAttrId;
    static const QString elemAttrName;
    static const QString elemAttrType;
    static const QString elemAttrProvider;
    static const QString elemAttrManager;
    static const QString elemAttrProtocol;
    static const QString elemAttrIcon;
    static const QString elemAttrLabel;
    static const QString elemAttrMandatory;
    static const QString elemAttrAllowOthers;
    static const QString elemAttrMessage;
    static const QString elemAttrDisabled;
};

const QString Profile::Private::XmlHandler::xmlNs = QLatin1String("http://telepathy.freedesktop.org/wiki/service-profile-v1");

const QString Profile::Private::XmlHandler::elemService = QLatin1String("service");
const QString Profile::Private::XmlHandler::elemName = QLatin1String("name");
const QString Profile::Private::XmlHandler::elemParams = QLatin1String("parameters");
const QString Profile::Private::XmlHandler::elemParam = QLatin1String("parameter");
const QString Profile::Private::XmlHandler::elemPresences = QLatin1String("presences");
const QString Profile::Private::XmlHandler::elemPresence = QLatin1String("presence");
const QString Profile::Private::XmlHandler::elemUnsupportedCCs = QLatin1String("unsupported-channel-classes");
const QString Profile::Private::XmlHandler::elemCC = QLatin1String("channel-class");
const QString Profile::Private::XmlHandler::elemProperty = QLatin1String("property");

const QString Profile::Private::XmlHandler::elemAttrId = QLatin1String("id");
const QString Profile::Private::XmlHandler::elemAttrName = QLatin1String("name");
const QString Profile::Private::XmlHandler::elemAttrType = QLatin1String("type");
const QString Profile::Private::XmlHandler::elemAttrProvider = QLatin1String("provider");
const QString Profile::Private::XmlHandler::elemAttrManager = QLatin1String("manager");
const QString Profile::Private::XmlHandler::elemAttrProtocol = QLatin1String("protocol");
const QString Profile::Private::XmlHandler::elemAttrLabel = QLatin1String("label");
const QString Profile::Private::XmlHandler::elemAttrMandatory = QLatin1String("mandatory");
const QString Profile::Private::XmlHandler::elemAttrAllowOthers = QLatin1String("allow-others");
const QString Profile::Private::XmlHandler::elemAttrIcon = QLatin1String("icon");
const QString Profile::Private::XmlHandler::elemAttrMessage = QLatin1String("message");
const QString Profile::Private::XmlHandler::elemAttrDisabled = QLatin1String("disabled");

Profile::Private::XmlHandler::XmlHandler(const QString &serviceName,
        bool allowNonIMType,
        Profile::Private::Data *outputData)
    : mServiceName(serviceName),
      allowNonIMType(allowNonIMType),
      mData(outputData),
      mMetServiceTag(false)
{
}

bool Profile::Private::XmlHandler::startElement(const QString &namespaceURI,
        const QString &localName, const QString &qName,
        const QXmlAttributes &attributes)
{
    if (!mMetServiceTag && qName != elemService) {
        mErrorString = QLatin1String("the file is not a profile file");
        return false;
    }

    if (namespaceURI != xmlNs) {
        // ignore all elements with unknown xmlns
        debug() << "Ignoring unknown xmlns" << namespaceURI;
        return true;
    }

#define CHECK_ELEMENT_IS_CHILD_OF(parentElement) \
    if (mElements.top() != parentElement) { \
        mErrorString = QString(QLatin1String("element '%1' is not a " \
                    "child of element '%2'")) \
            .arg(qName) \
            .arg(parentElement); \
        return false; \
    }
#define CHECK_ELEMENT_ATTRIBUTES_COUNT(value) \
    if (attributes.count() != value) { \
        mErrorString = QString(QLatin1String("element '%1' contains more " \
                    "than %2 attributes")) \
            .arg(qName) \
            .arg(value); \
        return false; \
    }
#define CHECK_ELEMENT_HAS_ATTRIBUTE(attribute) \
    if (attributes.index(attribute) == -1) { \
        mErrorString = QString(QLatin1String("mandatory attribute '%1' " \
                    "missing on element '%2'")) \
            .arg(attribute) \
            .arg(qName); \
        return false; \
    }
#define CHECK_ELEMENT_ATTRIBUTES(allowedAttrs) \
    for (int i = 0; i < attributes.count(); ++i) { \
        bool valid = false; \
        QString attrName = attributes.qName(i); \
        foreach (const QString &allowedAttr, allowedAttrs) { \
            if (attrName == allowedAttr) { \
                valid = true; \
                break; \
            } \
        } \
        if (!valid) { \
            mErrorString = QString(QLatin1String("invalid attribute '%1' on " \
                        "element '%2'")) \
                .arg(attrName) \
                .arg(qName); \
            return false; \
        } \
    }

    if (qName == elemService) {
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrId);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrType);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrManager);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrProtocol);

        QStringList allowedAttrs = QStringList() <<
            elemAttrId << elemAttrType << elemAttrManager <<
            elemAttrProtocol << elemAttrProvider << elemAttrIcon;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        if (attributes.value(elemAttrId) != mServiceName) {
            mErrorString = QString(QLatin1String("the '%1' attribute of the "
                        "element '%2' does not match the file name"))
                .arg(elemAttrId)
                .arg(elemService);
            return false;
        }

        mMetServiceTag = true;
        mData->type = attributes.value(elemAttrType);
        if (mData->type != QLatin1String("IM") && !allowNonIMType) {
            mErrorString = QString(QLatin1String("unknown value of element "
                        "'type': %1"))
                .arg(mCurrentText);
            return false;
        }
        mData->provider = attributes.value(elemAttrProvider);
        mData->cmName = attributes.value(elemAttrManager);
        mData->protocolName = attributes.value(elemAttrProtocol);
        mData->iconName = attributes.value(elemAttrIcon);
    } else if (qName == elemParams) {
        CHECK_ELEMENT_IS_CHILD_OF(elemService);
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);
    } else if (qName == elemParam) {
        CHECK_ELEMENT_IS_CHILD_OF(elemParams);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrName);
        QStringList allowedAttrs = QStringList() << elemAttrName <<
            elemAttrType << elemAttrMandatory << elemAttrLabel;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        QString paramType = attributes.value(elemAttrType);
        if (paramType.isEmpty()) {
            paramType = QLatin1String("s");
        }
        mCurrentParameter.setName(attributes.value(elemAttrName));
        mCurrentParameter.setDBusSignature(QDBusSignature(paramType));
        mCurrentParameter.setLabel(attributes.value(elemAttrLabel));
        mCurrentParameter.setMandatory(attributeValueAsBoolean(attributes,
                    elemAttrMandatory));
    } else if (qName == elemPresences) {
        CHECK_ELEMENT_IS_CHILD_OF(elemService);
        QStringList allowedAttrs = QStringList() << elemAttrAllowOthers;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        mData->allowOtherPresences = attributeValueAsBoolean(attributes,
                elemAttrAllowOthers);
    } else if (qName == elemPresence) {
        CHECK_ELEMENT_IS_CHILD_OF(elemPresences);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrId);
        QStringList allowedAttrs = QStringList() << elemAttrId <<
            elemAttrLabel << elemAttrIcon << elemAttrMessage <<
            elemAttrDisabled;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        mData->presences.append(Profile::Presence(
                    attributes.value(elemAttrId),
                    attributes.value(elemAttrLabel),
                    attributes.value(elemAttrIcon),
                    attributes.value(elemAttrMessage),
                    attributeValueAsBoolean(attributes, elemAttrDisabled)));
    } else if (qName == elemUnsupportedCCs) {
        CHECK_ELEMENT_IS_CHILD_OF(elemService);
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);
    } else if (qName == elemCC) {
        CHECK_ELEMENT_IS_CHILD_OF(elemUnsupportedCCs);
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);
    } else if (qName == elemProperty) {
        CHECK_ELEMENT_IS_CHILD_OF(elemCC);
        CHECK_ELEMENT_ATTRIBUTES_COUNT(2);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrName);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrType);

        mCurrentPropertyName = attributes.value(elemAttrName);
        mCurrentPropertyType = attributes.value(elemAttrType);
    } else {
        if (qName != elemName) {
            Tp::warning() << "Ignoring unknown element" << qName;
        } else {
            // check if we are inside <service>
            CHECK_ELEMENT_IS_CHILD_OF(elemService);
            // no element here supports attributes
            CHECK_ELEMENT_ATTRIBUTES_COUNT(0);
        }
    }

#undef CHECK_ELEMENT_IS_CHILD_OF
#undef CHECK_ELEMENT_ATTRIBUTES_COUNT
#undef CHECK_ELEMENT_HAS_ATTRIBUTE
#undef CHECK_ELEMENT_ATTRIBUTES

    mElements.push(qName);
    mCurrentText.clear();
    return true;
}

bool Profile::Private::XmlHandler::endElement(const QString &namespaceURI,
        const QString &localName, const QString &qName)
{
    if (namespaceURI != xmlNs) {
        // ignore all elements with unknown xmlns
        debug() << "Ignoring unknown xmlns" << namespaceURI;
        return true;
    } else if (qName == elemName) {
        mData->name = mCurrentText;
    } else if (qName == elemParam) {
        mCurrentParameter.setValue(parseValueWithDBusSignature(mCurrentText,
                    mCurrentParameter.dbusSignature().signature()));
        mData->parameters.append(Profile::Parameter(mCurrentParameter));
    } else if (qName == elemCC) {
        mData->unsupportedChannelClassSpecs.append(RequestableChannelClassSpec(mCurrentCC));
        mCurrentCC.fixedProperties.clear();
    } else if (qName == elemProperty) {
        mCurrentCC.fixedProperties[mCurrentPropertyName] =
            parseValueWithDBusSignature(mCurrentText,
                    mCurrentPropertyType);
    }

    mElements.pop();
    return true;
}

bool Profile::Private::XmlHandler::characters(const QString &str)
{
    mCurrentText += str;
    return true;
}

bool Profile::Private::XmlHandler::fatalError(
        const QXmlParseException &exception)
{
    mErrorString = QString(QLatin1String("parse error at line %1, column %2: "
                "%3"))
        .arg(exception.lineNumber())
        .arg(exception.columnNumber())
        .arg(exception.message());
    return false;
}

QString Profile::Private::XmlHandler::errorString() const
{
    return mErrorString;
}

bool Profile::Private::XmlHandler::attributeValueAsBoolean(
        const QXmlAttributes &attributes, const QString &qName)
{
    QString tmpStr = attributes.value(qName);
    if (tmpStr == QLatin1String("1") ||
        tmpStr == QLatin1String("true")) {
        return true;
    } else {
        return false;
    }
}


Profile::Private::Private()
    : valid(false),
      fake(false),
      allowNonIMType(false)
{
}

void Profile::Private::setServiceName(const QString &serviceName_)
{
    invalidate();

    allowNonIMType = false;
    serviceName = serviceName_;
    lookupProfile();
}

void Profile::Private::setFileName(const QString &fileName)
{
    invalidate();

    allowNonIMType = true;
    QFileInfo fi(fileName);
    serviceName = fi.baseName();

    debug() << "Loading profile file" << fileName;

    QFile file(fileName);
    if (!file.exists()) {
        warning() << QString(QLatin1String("Error parsing profile file %1: file does not exist"))
            .arg(file.fileName());
        return;
    }

    if (!file.open(QFile::ReadOnly)) {
        warning() << QString(QLatin1String("Error parsing profile file %1: "
                    "cannot open file for readonly access"))
            .arg(file.fileName());
        return;
    }

    if (parse(&file)) {
        debug() << "Profile file" << fileName << "loaded successfully";
    }
}

void Profile::Private::lookupProfile()
{
    debug() << "Searching profile for service" << serviceName;

    QStringList searchDirs = Profile::searchDirs();
    bool found = false;
    foreach (const QString searchDir, searchDirs) {
        QString fileName = searchDir + serviceName + QLatin1String(".profile");

        QFile file(fileName);
        if (!file.exists()) {
            continue;
        }

        if (!file.open(QFile::ReadOnly)) {
            continue;
        }

        if (parse(&file)) {
            debug() << "Profile for service" << serviceName << "found:" << fileName;
            found = true;
            break;
        }
    }

    if (!found) {
        debug() << "Cannot find valid profile for service" << serviceName;
    }
}

bool Profile::Private::parse(QFile *file)
{
    invalidate();

    fake = false;
    QFileInfo fi(file->fileName());
    XmlHandler xmlHandler(serviceName, allowNonIMType, &data);

    QXmlSimpleReader xmlReader;
    xmlReader.setContentHandler(&xmlHandler);
    xmlReader.setErrorHandler(&xmlHandler);

    QXmlInputSource xmlInputSource(file);
    if (!xmlReader.parse(xmlInputSource)) {
        warning() << QString(QLatin1String("Error parsing profile file %1: %2"))
            .arg(file->fileName())
            .arg(xmlHandler.errorString());
        invalidate();
        return false;
    }

    valid = true;
    return true;
}

void Profile::Private::invalidate()
{
    valid = false;
    data.clear();
}

/**
 * \class Profile
 * \ingroup utils
 * \headerfile TelepathyQt/profile.h <TelepathyQt/Profile>
 *
 * \brief The Profile class provides an easy way to read Telepathy profile
 * files according to http://telepathy.freedesktop.org/wiki/service-profile-v1.
 *
 * Note that profiles with xml element \<type\> different than "IM" are considered
 * invalid.
 */

/**
 * Create a new Profile object used to read .profiles compliant files.
 *
 * \param serviceName The profile service name.
 * \return A ProfilePtr object pointing to the newly created Profile object.
 */
ProfilePtr Profile::createForServiceName(const QString &serviceName)
{
    ProfilePtr profile = ProfilePtr(new Profile());
    profile->setServiceName(serviceName);
    return profile;
}

/**
 * Create a new Profile object used to read .profiles compliant files.
 *
 * \param fileName The profile file name.
 * \return A ProfilePtr object pointing to the newly created Profile object.
 */
ProfilePtr Profile::createForFileName(const QString &fileName)
{
    ProfilePtr profile = ProfilePtr(new Profile());
    profile->setFileName(fileName);
    return profile;
}

/**
 * Construct a new Profile object used to read .profiles compliant files.
 *
 * \param serviceName The profile service name.
 */
Profile::Profile()
    : mPriv(new Private())
{
}

/**
 * Construct a fake profile using the given \a serviceName, \a cmName,
 * \a protocolName and \a protocolInfo.
 *
 *  - isFake() will return \c true
 *  - type() will return "IM"
 *  - provider() will return an empty string
 *  - serviceName() will return \a serviceName
 *  - name() and protocolName() will return \a protocolName
 *  - iconName() will return "im-\a protocolName"
 *  - cmName() will return \a cmName
 *  - parameters() will return a list matching CM default parameters
 *  - presences() will return an empty list and allowOtherPresences will return
 *    \c true, meaning that CM presences should be used
 *  - unsupportedChannelClassSpecs() will return an empty list
 *
 * \param serviceName The service name.
 * \param cmName The connection manager name.
 * \param protocolName The protocol name.
 * \param protocolInfo The protocol info for the protocol \a protocolName.
 */
Profile::Profile(const QString &serviceName, const QString &cmName,
        const QString &protocolName, const ProtocolInfo &protocolInfo)
    : mPriv(new Private())
{
    mPriv->serviceName = serviceName;

    mPriv->data.type = QString(QLatin1String("IM"));
    // provider is empty
    mPriv->data.name = protocolName;
    mPriv->data.iconName = QString(QLatin1String("im-%1")).arg(protocolName);
    mPriv->data.cmName = cmName;
    mPriv->data.protocolName = protocolName;

    foreach (const ProtocolParameter &protocolParam, protocolInfo.parameters()) {
        if (!protocolParam.defaultValue().isNull()) {
            mPriv->data.parameters.append(Profile::Parameter(
                        protocolParam.name(),
                        protocolParam.dbusSignature(),
                        protocolParam.defaultValue(),
                        QString(), // label
                        false));    // mandatory
        }
    }

    // parameters will be the same as CM parameters
    // set allow others to true meaning that the standard CM presences are
    // supported
    mPriv->data.allowOtherPresences = true;
    // presences will be the same as CM presences
    // unsupported channel classes is empty

    mPriv->valid = true;
    mPriv->fake = true;
}

/**
 * Class destructor.
 */
Profile::~Profile()
{
    delete mPriv;
}

/**
 * Return the unique name of the service to which this profile applies.
 *
 * \return The unique name of the service.
 */
QString Profile::serviceName() const
{
    return mPriv->serviceName;
}

/**
 * Return whether this profile is valid.
 *
 * \return \c true if valid, otherwise \c false.
 */
bool Profile::isValid() const
{
    return mPriv->valid;
}

/**
 * Return whether this profile is fake.
 *
 * Fake profiles are profiles created for services not providing a .profile
 * file.
 *
 * \return \c true if fake, otherwise \c false.
 */
bool Profile::isFake() const
{
    return mPriv->fake;
}

/**
 * Return the type of the service to which this profile applies.
 *
 * In general, services of interest of Telepathy should be of type 'IM'.
 * Other service types exist but are unlikely to affect Telepathy in any way.
 *
 * \return The type of the service.
 */
QString Profile::type() const
{
    return mPriv->data.type;
}

/**
 * Return the name of the vendor/organisation/provider who actually runs the
 * service to which this profile applies.
 *
 * \return The provider of the service.
 */
QString Profile::provider() const
{
    return mPriv->data.provider;
}

/**
 * Return the human-readable name for the service to which this profile applies.
 *
 * \return The Human-readable name of the service.
 */
QString Profile::name() const
{
    return mPriv->data.name;
}

/**
 * Return the base name of the icon for the service to which this profile
 * applies.
 *
 * \return The base name of the icon for the service.
 */
QString Profile::iconName() const
{
    return mPriv->data.iconName;
}

/**
 * Return the connection manager name for the service to which this profile
 * applies.
 *
 * \return The connection manager name for the service.
 */
QString Profile::cmName() const
{
    return mPriv->data.cmName;
}

/**
 * Return the protocol name for the service to which this profile applies.
 *
 * \return The protocol name for the service.
 */
QString Profile::protocolName() const
{
    return mPriv->data.protocolName;
}

/**
 * Return a list of parameters defined for the service to which this profile
 * applies.
 *
 * \return A list of Profile::Parameter.
 */
Profile::ParameterList Profile::parameters() const
{
    return mPriv->data.parameters;
}

/**
 * Return whether this profile defines the parameter named \a name.
 *
 * \return \c true if parameter is defined, otherwise \c false.
 */
bool Profile::hasParameter(const QString &name) const
{
    foreach (const Parameter &parameter, mPriv->data.parameters) {
        if (parameter.name() == name) {
            return true;
        }
    }
    return false;
}

/**
 * Return the parameter for a given \a name.
 *
 * \return A Profile::Parameter.
 */
Profile::Parameter Profile::parameter(const QString &name) const
{
    foreach (const Parameter &parameter, mPriv->data.parameters) {
        if (parameter.name() == name) {
            return parameter;
        }
    }
    return Profile::Parameter();
}

/**
 * Return whether the standard CM presences not defined in presences() are
 * supported.
 *
 * \return \c true if standard CM presences are supported, otherwise \c false.
 */
bool Profile::allowOtherPresences() const
{
    return mPriv->data.allowOtherPresences;
}

/**
 * Return a list of presences defined for the service to which this profile
 * applies.
 *
 * \return A list of Profile::Presence.
 */
Profile::PresenceList Profile::presences() const
{
    return mPriv->data.presences;
}

/**
 * Return whether this profile defines the presence with id \a id.
 *
 * \return \c true if presence is defined, otherwise \c false.
 */
bool Profile::hasPresence(const QString &id) const
{
    foreach (const Presence &presence, mPriv->data.presences) {
        if (presence.id() == id) {
            return true;
        }
    }
    return false;
}

/**
 * Return the presence for a given \a id.
 *
 * \return A Profile::Presence.
 */
Profile::Presence Profile::presence(const QString &id) const
{
    foreach (const Presence &presence, mPriv->data.presences) {
        if (presence.id() == id) {
            return presence;
        }
    }
    return Profile::Presence();
}

/**
 * A list of channel classes not supported by the service to which this profile
 * applies.
 *
 * \return A list of RequestableChannelClassSpec.
 */
RequestableChannelClassSpecList Profile::unsupportedChannelClassSpecs() const
{
    return mPriv->data.unsupportedChannelClassSpecs;
}

void Profile::setServiceName(const QString &serviceName)
{
    mPriv->setServiceName(serviceName);
}

void Profile::setFileName(const QString &fileName)
{
    mPriv->setFileName(fileName);
}

QStringList Profile::searchDirs()
{
    QStringList ret;

    QString xdgDataHome = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
    if (xdgDataHome.isEmpty()) {
        ret << QDir::homePath() + QLatin1String("/.local/share/data/telepathy/profiles/");
    } else {
        ret << xdgDataHome + QLatin1String("/telepathy/profiles/");
    }

    QString xdgDataDirsEnv = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS"));
    if (xdgDataDirsEnv.isEmpty()) {
        ret << QLatin1String("/usr/local/share/telepathy/profiles/");
        ret << QLatin1String("/usr/share/telepathy/profiles/");
    } else {
        QStringList xdgDataDirs = xdgDataDirsEnv.split(QLatin1Char(':'));
        foreach (const QString xdgDataDir, xdgDataDirs) {
            ret << xdgDataDir + QLatin1String("/telepathy/profiles/");
        }
    }

    return ret;
}


struct TP_QT_NO_EXPORT Profile::Parameter::Private
{
    QString name;
    QDBusSignature dbusSignature;
    QVariant value;
    QString label;
    bool mandatory;
};

/**
 * \class Profile::Parameter
 * \ingroup utils
 * \headerfile TelepathyQt/profile.h <TelepathyQt/Profile>
 *
 * \brief The Profile::Parameter class represents a parameter defined in
 * .profile files.
 */

/**
 * Construct a new Profile::Parameter object.
 */
Profile::Parameter::Parameter()
    : mPriv(new Private)
{
    mPriv->mandatory = false;
}

/**
 * Construct a new Profile::Parameter object that is a copy of \a other.
 */
Profile::Parameter::Parameter(const Parameter &other)
    : mPriv(new Private)
{
    mPriv->name = other.mPriv->name;
    mPriv->dbusSignature = other.mPriv->dbusSignature;
    mPriv->value = other.mPriv->value;
    mPriv->label = other.mPriv->label;
    mPriv->mandatory = other.mPriv->mandatory;
}

/**
 * Construct a new Profile::Parameter object.
 *
 * \param name The parameter name.
 * \param dbusSignature The parameter D-Bus signature.
 * \param value The parameter value.
 * \param label The parameter label.
 * \param mandatory Whether this parameter is mandatory.
 */
Profile::Parameter::Parameter(const QString &name,
        const QDBusSignature &dbusSignature,
        const QVariant &value,
        const QString &label,
        bool mandatory)
    : mPriv(new Private)
{
    mPriv->name = name;
    mPriv->dbusSignature = dbusSignature;
    mPriv->value = value;
    mPriv->label = label;
    mPriv->mandatory = mandatory;
}

/**
 * Class destructor.
 */
Profile::Parameter::~Parameter()
{
    delete mPriv;
}

/**
 * Return the name of this parameter.
 *
 * \return The name of this parameter.
 */
QString Profile::Parameter::name() const
{
    return mPriv->name;
}

void Profile::Parameter::setName(const QString &name)
{
    mPriv->name = name;
}

/**
 * Return the D-Bus signature of this parameter.
 *
 * \return The D-Bus signature of this parameter.
 */
QDBusSignature Profile::Parameter::dbusSignature() const
{
    return mPriv->dbusSignature;
}

void Profile::Parameter::setDBusSignature(const QDBusSignature &dbusSignature)
{
    mPriv->dbusSignature = dbusSignature;
}

/**
 * Return the QVariant::Type of this parameter, constructed using
 * dbusSignature().
 *
 * \return The QVariant::Type of this parameter.
 */
QVariant::Type Profile::Parameter::type() const
{
    return variantTypeFromDBusSignature(mPriv->dbusSignature.signature());
}

/**
 * Return the value of this parameter.
 *
 * If mandatory() returns \c true, the value must not be modified and should be
 * used as is when creating accounts for this profile.
 *
 * \return The value of this parameter.
 */
QVariant Profile::Parameter::value() const
{
    return mPriv->value;
}

void Profile::Parameter::setValue(const QVariant &value)
{
    mPriv->value = value;
}

/**
 * Return the human-readable label of this parameter.
 *
 * \return The human-readable label of this parameter.
 */
QString Profile::Parameter::label() const
{
    return mPriv->label;
}

void Profile::Parameter::setLabel(const QString &label)
{
    mPriv->label = label;
}

/**
 * Return whether this parameter is mandatory, or whether the value returned by
 * value() should be used as is when creating accounts for this profile.
 *
 * \return \c true if mandatory, otherwise \c false.
 */
bool Profile::Parameter::isMandatory() const
{
    return mPriv->mandatory;
}

void Profile::Parameter::setMandatory(bool mandatory)
{
    mPriv->mandatory = mandatory;
}

Profile::Parameter &Profile::Parameter::operator=(const Profile::Parameter &other)
{
    mPriv->name = other.mPriv->name;
    mPriv->dbusSignature = other.mPriv->dbusSignature;
    mPriv->value = other.mPriv->value;
    mPriv->label = other.mPriv->label;
    mPriv->mandatory = other.mPriv->mandatory;
    return *this;
}


struct TP_QT_NO_EXPORT Profile::Presence::Private
{
    QString id;
    QString label;
    QString iconName;
    QString message;
    bool disabled;
};

/**
 * \class Profile::Presence
 * \ingroup utils
 * \headerfile TelepathyQt/profile.h <TelepathyQt/Profile>
 *
 * \brief The Profile::Presence class represents a presence defined in
 * .profile files.
 */

/**
 * Construct a new Profile::Presence object.
 */
Profile::Presence::Presence()
    : mPriv(new Private)
{
    mPriv->disabled = false;
}

/**
 * Construct a new Profile::Presence object that is a copy of \a other.
 */
Profile::Presence::Presence(const Presence &other)
    : mPriv(new Private)
{
    mPriv->id = other.mPriv->id;
    mPriv->label = other.mPriv->label;
    mPriv->iconName = other.mPriv->iconName;
    mPriv->message = other.mPriv->message;
    mPriv->disabled = other.mPriv->disabled;
}

/**
 * Construct a new Profile::Presence object.
 *
 * \param id The presence id.
 * \param label The presence label.
 * \param iconName The presence icon name.
 * \param message The presence message.
 * \param disabled Whether this presence is supported.
 */
Profile::Presence::Presence(const QString &id,
        const QString &label,
        const QString &iconName,
        const QString &message,
        bool disabled)
    : mPriv(new Private)
{
    mPriv->id = id;
    mPriv->label = label;
    mPriv->iconName = iconName;
    mPriv->message = message;
    mPriv->disabled = disabled;
}

/**
 * Class destructor.
 */
Profile::Presence::~Presence()
{
    delete mPriv;
}

/**
 * Return the Telepathy presence id for this presence.
 *
 * \return The Telepathy presence id for this presence.
 */
QString Profile::Presence::id() const
{
    return mPriv->id;
}

void Profile::Presence::setId(const QString &id)
{
    mPriv->id = id;
}

/**
 * Return the label that should be used for this presence.
 *
 * \return The label for this presence.
 */
QString Profile::Presence::label() const
{
    return mPriv->label;
}

void Profile::Presence::setLabel(const QString &label)
{
    mPriv->label = label;
}

/**
 * Return the icon name of this presence.
 *
 * \return The icon name of this presence.
 */
QString Profile::Presence::iconName() const
{
    return mPriv->iconName;
}

void Profile::Presence::setIconName(const QString &iconName)
{
    mPriv->iconName = iconName;
}

/**
 * Return whether user-defined text-message can be attached to this presence.
 *
 * \return \c true if user-defined text-message can be attached to this presence, \c false
 *         otherwise.
 */
bool Profile::Presence::canHaveStatusMessage() const
{
    if (mPriv->message == QLatin1String("1") ||
        mPriv->message == QLatin1String("true")) {
        return true;
    }

    return false;
}

void Profile::Presence::setMessage(const QString &message)
{
    mPriv->message = message;
}

/**
 * Return whether this presence is supported for the service to which this
 * profile applies.
 *
 * \return \c true if supported, otherwise \c false.
 */
bool Profile::Presence::isDisabled() const
{
    return mPriv->disabled;
}

void Profile::Presence::setDisabled(bool disabled)
{
    mPriv->disabled = disabled;
}

Profile::Presence &Profile::Presence::operator=(const Profile::Presence &other)
{
    mPriv->id = other.mPriv->id;
    mPriv->label = other.mPriv->label;
    mPriv->iconName = other.mPriv->iconName;
    mPriv->message = other.mPriv->message;
    mPriv->disabled = other.mPriv->disabled;
    return *this;
}

} // Tp
