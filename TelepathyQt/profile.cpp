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


class TP_QT_NO_EXPORT Profile::Private::XmlHandler
{
public:
    XmlHandler(const QString &serviceName, bool allowNonIMType, Profile::Private::Data *outputData);

    bool parse(QXmlStreamReader *reader);
    QString errorString() const;

private:
    bool attributeValueAsBoolean(const QXmlStreamAttributes &attributes,
            const QString &qName);

    QString mServiceName;
    bool allowNonIMType;
    Profile::Private::Data *mData;

    bool parseServiceElem(QXmlStreamReader *reader);
    bool parseParametersElem(QXmlStreamReader *reader);
    bool parseParamElem(QXmlStreamReader *reader);
    bool parsePresencesElem(QXmlStreamReader *reader);
    bool parsePresenceElem(QXmlStreamReader *reader);
    bool parseUnsupportedCCsElem(QXmlStreamReader *reader);
    bool parseCCElem(QXmlStreamReader *reader);
    bool parsePropElem(QXmlStreamReader *reader);

    Profile::Parameter mCurrentParameter;
    RequestableChannelClass mCurrentCC;
    QString mErrorString;

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
      mData(outputData)
{
}

#define CHECK_ELEMENT_ATTRIBUTES_COUNT(value) \
    if (attributes.count() != value) { \
        mErrorString = QString(QLatin1String("element '%1' contains more " \
                    "than %2 attributes")) \
            .arg(reader->name()) \
            .arg(value); \
        return false; \
    }
#define CHECK_ELEMENT_HAS_ATTRIBUTE(attribute) \
    if (!attributes.hasAttribute(attribute)) { \
        mErrorString = QString(QLatin1String("mandatory attribute '%1' " \
                    "missing on element '%2'")) \
            .arg(attribute) \
            .arg(reader->name()); \
        return false; \
    }
#define CHECK_ELEMENT_ATTRIBUTES(allowedAttrs) \
    for (int i = 0; i < attributes.count(); ++i) { \
        bool valid = false; \
        QString attrName; attrName.append(attributes.at(i).name()); \
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
                .arg(reader->name()); \
            return false; \
        } \
    }

bool Profile::Private::XmlHandler::parseServiceElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemService) {
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

        mData->type = attributes.value(elemAttrType).toString();
        if (mData->type != QLatin1String("IM") && !allowNonIMType) {
            mErrorString = QString(QLatin1String("unknown value of element "
                        "'type': %1"))
                .arg(mData->type);
            return false;
        }
        mData->provider = attributes.value(elemAttrProvider).toString();
        mData->cmName = attributes.value(elemAttrManager).toString();
        mData->protocolName = attributes.value(elemAttrProtocol).toString();
        mData->iconName = attributes.value(elemAttrIcon).toString();
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parseParametersElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemParams) {
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);

        do {
            while (reader->readNextStartElement()) {
                if (!parseParamElem(reader))
                    return false;
            }

            // try again if we hit the end element of a child node
        } while (!reader->hasError() && reader->name() != elemParams);
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parseParamElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemParam) {
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrName);
        QStringList allowedAttrs = QStringList() << elemAttrName <<
            elemAttrType << elemAttrMandatory << elemAttrLabel;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        QString paramType = attributes.value(elemAttrType).toString();
        if (paramType.isEmpty()) {
            paramType = QLatin1String("s");
        }
        mCurrentParameter.setName(attributes.value(elemAttrName).toString());
        mCurrentParameter.setDBusSignature(QDBusSignature(paramType));
        mCurrentParameter.setLabel(attributes.value(elemAttrLabel).toString());
        mCurrentParameter.setMandatory(attributeValueAsBoolean(attributes,
                    elemAttrMandatory));
        const QString &currentText =
                reader->readElementText(QXmlStreamReader::SkipChildElements);
        mCurrentParameter.setValue(parseValueWithDBusSignature(currentText,
                    mCurrentParameter.dbusSignature().signature()));
        mData->parameters.append(Profile::Parameter(mCurrentParameter));
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parsePresencesElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemPresences) {
        QStringList allowedAttrs = QStringList() << elemAttrAllowOthers;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);

        mData->allowOtherPresences = attributeValueAsBoolean(attributes,
                elemAttrAllowOthers);

        do {
            while (reader->readNextStartElement()) {
                if (!parsePresenceElem(reader))
                    return false;
            }

            // try again if we hit the end element of a child node
        } while (!reader->hasError() && reader->name() != elemPresences);
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parsePresenceElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemPresence) {
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrId);
        QStringList allowedAttrs = QStringList() << elemAttrId <<
            elemAttrLabel << elemAttrIcon << elemAttrMessage <<
            elemAttrDisabled;
        CHECK_ELEMENT_ATTRIBUTES(allowedAttrs);
        mData->presences.append(Profile::Presence(
                    attributes.value(elemAttrId).toString(),
                    attributes.value(elemAttrLabel).toString(),
                    attributes.value(elemAttrIcon).toString(),
                    attributes.value(elemAttrMessage).toString(),
                    attributeValueAsBoolean(attributes, elemAttrDisabled)));
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parseUnsupportedCCsElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemUnsupportedCCs) {
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);

        do {
            while (reader->readNextStartElement()) {
                if (!parseCCElem(reader))
                    return false;
            }

            // try again if we hit the end element of a child node
        } while (!reader->hasError() && reader->name() != elemUnsupportedCCs);
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parseCCElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemCC) {
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);

        do {
            while (reader->readNextStartElement()) {
                if (!parsePropElem(reader))
                    return false;
            }
            // try again if we hit the end element of a child node
        } while (!reader->hasError() && reader->name() != elemCC);

        mData->unsupportedChannelClassSpecs.append(RequestableChannelClassSpec(mCurrentCC));
        mCurrentCC.fixedProperties.clear();
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

bool Profile::Private::XmlHandler::parsePropElem(QXmlStreamReader *reader)
{
    const QXmlStreamAttributes &attributes = reader->attributes();

    if (reader->name() == elemProperty) {
        CHECK_ELEMENT_ATTRIBUTES_COUNT(2);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrName);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrType);

        QString propertyName = attributes.value(elemAttrName).toString();
        QString propertyType = attributes.value(elemAttrType).toString();
        const QString &propertyText =
                reader->readElementText(QXmlStreamReader::SkipChildElements);
        mCurrentCC.fixedProperties[propertyName] =
            parseValueWithDBusSignature(propertyText, propertyType);
    } else {
        Tp::warning() << "Ignoring unknown element" << reader->name();
        reader->skipCurrentElement();
    }

    return true;
}

#undef CHECK_ELEMENT_ATTRIBUTES_COUNT
#undef CHECK_ELEMENT_HAS_ATTRIBUTE
#undef CHECK_ELEMENT_ATTRIBUTES

QString Profile::Private::XmlHandler::errorString() const
{
    return mErrorString;
}

bool Profile::Private::XmlHandler::attributeValueAsBoolean(
        const QXmlStreamAttributes &attributes, const QString &qName)
{
    QString tmpStr = attributes.value(qName).toString();
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

bool Profile::Private::XmlHandler::parse(QXmlStreamReader *reader)
{
    if (!reader->readNextStartElement() || !reader->isStartElement() ||
            reader->namespaceUri() != xmlNs || reader->name() != elemService) {
        mErrorString = QString(QLatin1String("the file is not a profile file"));
        return false;
    }

    // top level element should be <service id=[...] >
    if (!parseServiceElem(reader))
        return false;

    while (reader->readNextStartElement()) {
        if (reader->namespaceUri() != xmlNs) {
            // ignore all elements with unknown xmlns
            debug() << "Ignoring unknown xmlns" << reader->namespaceUri();
            continue;
        }

        if (reader->name() == elemName) {
            // <name> foo </name>
            mData->name = reader->readElementText(QXmlStreamReader::SkipChildElements);
        } else if (reader->name() == elemParams) {
            // <parameters>
            //   <parameter name=[...] > foo </parameter>
            //   <parameter name=[...] />
            // </parameters>
            if (!parseParametersElem(reader))
                return false;
        } else if (reader->name() == elemPresences) {
            // <presences allow-others="1">
            //   <presence id=[...] />
            // </presences>
            if (!parsePresencesElem(reader))
                return false;
        } else if (reader->name() == elemUnsupportedCCs) {
            // <unsupported-channel-classes>
            //   <channel-class>
            //     <property name=[...]>1</property>
            //     <property name=[...]>Channel.Type.Text</property>
            //   </channel-class>
            //   <channel-class>
            //     [...]
            //   </channel-class>
            // </unsupported-channel-classes>
            if (!parseUnsupportedCCsElem(reader))
                return false;
        } else {
            Tp::warning() << "Ignoring unknown element" << reader->name() << " in main loop";
            reader->skipCurrentElement();
        }
    }
    if (reader->hasError()) {
        mErrorString = QString(QLatin1String(
                    "parse error at line %1, column %2: %3"))
            .arg(reader->lineNumber())
            .arg(reader->columnNumber())
            .arg(reader->errorString());
        return false;
    }

    return true;
}

bool Profile::Private::parse(QFile *file)
{
    invalidate();

    fake = false;
    XmlHandler xmlHandler(serviceName, allowNonIMType, &data);
    QXmlStreamReader reader(file);

    if (!xmlHandler.parse(&reader)) {
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
