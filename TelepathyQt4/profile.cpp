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

#include <TelepathyQt4/Profile>

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Utils>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QXmlAttributes>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT Profile::Private
{
    Private();
    Private(const QString &serviceName);

    void init();
    void setFileName(const QString &fileName);
    bool parse(const QString &fileName);

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
        bool allowOthersPresences;
        Profile::PresenceList presences;
        RequestableChannelClassList unsupportedChannelClasses;
    };

    class XmlHandler;

    QString serviceName;
    bool valid;
    Data data;
};

Profile::Private::Data::Data()
    : allowOthersPresences(false)
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
    allowOthersPresences = false;
    presences = Profile::PresenceList();
    unsupportedChannelClasses = RequestableChannelClassList();
}


class TELEPATHY_QT4_NO_EXPORT Profile::Private::XmlHandler :
                public QXmlDefaultHandler
{
public:
    XmlHandler(const QString &baseFileName, Profile::Private::Data *outputData);

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

    QString mBaseFileName;
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
    static const QString elemType;
    static const QString elemProvider;
    static const QString elemName;
    static const QString elemIcon;
    static const QString elemManager;
    static const QString elemProtocol;
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
    static const QString elemAttrLabel;
    static const QString elemAttrMandatory;
    static const QString elemAttrAllowOthers;
    static const QString elemAttrIcon;
    static const QString elemAttrMessage;
    static const QString elemAttrDisabled;
};

const QString Profile::Private::XmlHandler::xmlNs = QLatin1String("http://telepathy.freedesktop.org/wiki/service-profile-v1");

const QString Profile::Private::XmlHandler::elemService = QLatin1String("service");
const QString Profile::Private::XmlHandler::elemType = QLatin1String("type");
const QString Profile::Private::XmlHandler::elemProvider = QLatin1String("provider");
const QString Profile::Private::XmlHandler::elemName = QLatin1String("name");
const QString Profile::Private::XmlHandler::elemIcon = QLatin1String("icon");
const QString Profile::Private::XmlHandler::elemManager = QLatin1String("manager");
const QString Profile::Private::XmlHandler::elemProtocol = QLatin1String("protocol");
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
const QString Profile::Private::XmlHandler::elemAttrLabel = QLatin1String("label");
const QString Profile::Private::XmlHandler::elemAttrMandatory = QLatin1String("mandatory");
const QString Profile::Private::XmlHandler::elemAttrAllowOthers = QLatin1String("allow-others");
const QString Profile::Private::XmlHandler::elemAttrIcon = QLatin1String("icon");
const QString Profile::Private::XmlHandler::elemAttrMessage = QLatin1String("message");
const QString Profile::Private::XmlHandler::elemAttrDisabled = QLatin1String("disabled");

Profile::Private::XmlHandler::XmlHandler(const QString &baseFileName,
        Profile::Private::Data *outputData)
    : mBaseFileName(baseFileName),
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
        CHECK_ELEMENT_ATTRIBUTES_COUNT(1);
        CHECK_ELEMENT_HAS_ATTRIBUTE(elemAttrId);

        if (attributes.value(elemAttrId) != mBaseFileName) {
            mErrorString = QString(QLatin1String("the '%1' attribute of the "
                        "element '%2' does not match the file name"))
                .arg(elemAttrId)
                .arg(elemService);
            return false;
        }
        mMetServiceTag = true;
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

        mData->allowOthersPresences = attributeValueAsBoolean(attributes,
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
        if (qName != elemType && qName != elemProvider &&
            qName != elemName && qName != elemIcon &&
            qName != elemManager && qName != elemProtocol) {
            mErrorString = QString(QLatin1String("unknown element '%1'"))
                .arg(qName);
            return false;
        }

        // check if we are inside <service>
        CHECK_ELEMENT_IS_CHILD_OF(elemService);
        // no element here supports attributes
        CHECK_ELEMENT_ATTRIBUTES_COUNT(0);
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
    }

    if (qName == elemType) {
        mData->type = mCurrentText;
    } else if (qName == elemProvider) {
        mData->provider = mCurrentText;
    } else if (qName == elemName) {
        mData->name = mCurrentText;
    } else if (qName == elemIcon) {
        mData->iconName = mCurrentText;
    } else if (qName == elemManager) {
        mData->cmName = mCurrentText;
    } else if (qName == elemProtocol) {
        mData->protocolName = mCurrentText;
    } else if (qName == elemParam) {
        mCurrentParameter.setValue(parseValueWithSignature(mCurrentText,
                    mCurrentParameter.dbusSignature()));
        mData->parameters.append(Profile::Parameter(mCurrentParameter));
    } else if (qName == elemCC) {
        mData->unsupportedChannelClasses.append(mCurrentCC);
        mCurrentCC.fixedProperties.clear();
    } else if (qName == elemProperty) {
        mCurrentCC.fixedProperties[mCurrentPropertyName] =
            parseValueWithSignature(mCurrentText,
                    QDBusSignature(mCurrentPropertyType));
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
    : valid(false)
{
}

Profile::Private::Private(const QString &serviceName)
    : serviceName(serviceName),
      valid(false)
{
    init();
}

void Profile::Private::init()
{
    QStringList searchDirs = Profile::searchDirs();

    foreach (const QString searchDir, searchDirs) {
        QString fileName = searchDir + serviceName + QLatin1String(".profile");
        if (QFile::exists(fileName)) {
            debug() << "Parsing profile file" << fileName;
            data.clear();
            if (!parse(fileName)) {
                data.clear();
                continue;
            }
            valid = true;
            return;
        }
    }
}

void Profile::Private::setFileName(const QString &fileName)
{
    QFileInfo fi(fileName);
    serviceName = fi.baseName();
    data.clear();
    valid = false;
    if (parse(fileName)) {
        valid = true;
    } else {
        serviceName = QString();
        data.clear();
    }
}

bool Profile::Private::parse(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists()) {
        warning().nospace() << "Error parsing profile file " << fileName <<
            ": " << "file does not exist";
        return false;
    }

    if (!file.open(QFile::ReadOnly)) {
        warning().nospace() << "Error parsing profile file " << fileName <<
            ": " << "cannot open file for readonly access";
        return false;
    }

    QFileInfo fi(fileName);
    XmlHandler xmlHandler(fi.baseName(), &data);

    QXmlSimpleReader xmlReader;
    xmlReader.setContentHandler(&xmlHandler);
    xmlReader.setErrorHandler(&xmlHandler);

    QXmlInputSource xmlInputSource(&file);
    if (!xmlReader.parse(xmlInputSource)) {
        warning().nospace() << "Error parsing profile file " << fileName <<
            ": " << xmlHandler.errorString();
        return false;
    }

    return true;
}


/**
 * \class Profile
 * \headerfile TelepathyQt4/profile.h <TelepathyQt4/Profile>
 *
 * \brief The Profile class provides an easy way to read telepathy profile
 * files according to http://telepathy.freedesktop.org/spec.html.
 */

/**
 * Create a new Profile object used to read .profiles compliant files.
 *
 * \param serviceName The profile service name.
 * \return A ProfilePtr object pointing to the newly created Profile object.
 */
ProfilePtr Profile::create(const QString &serviceName)
{
    return ProfilePtr(new Profile(serviceName));
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
 * Construct a new Profile object used to read .profiles compliant files.
 *
 * \param serviceName The profile service name.
 */
Profile::Profile(const QString &serviceName)
    : mPriv(new Private(serviceName))
{
}

/**
 * Class destructor.
 */
Profile::~Profile()
{
    delete mPriv;
}

QString Profile::serviceName() const
{
    return mPriv->serviceName;
}

bool Profile::isValid() const
{
    return mPriv->valid;
}

QString Profile::type() const
{
    return mPriv->data.type;
}

QString Profile::provider() const
{
    return mPriv->data.provider;
}

QString Profile::name() const
{
    return mPriv->data.name;
}

QString Profile::iconName() const
{
    return mPriv->data.iconName;
}

QString Profile::cmName() const
{
    return mPriv->data.cmName;
}

QString Profile::protocolName() const
{
    return mPriv->data.protocolName;
}

Profile::ParameterList Profile::parameters() const
{
    return mPriv->data.parameters;
}

bool Profile::hasParameter(const QString &name) const
{
    foreach (const Parameter &parameter, mPriv->data.parameters) {
        if (parameter.name() == name) {
            return true;
        }
    }
    return false;
}

Profile::Parameter Profile::parameter(const QString &name) const
{
    foreach (const Parameter &parameter, mPriv->data.parameters) {
        if (parameter.name() == name) {
            return parameter;
        }
    }
    return Profile::Parameter();
}

bool Profile::allowOthersPresences() const
{
    return mPriv->data.allowOthersPresences;
}

Profile::PresenceList Profile::presences() const
{
    return mPriv->data.presences;
}

bool Profile::hasPresence(const QString &id) const
{
    foreach (const Presence &presence, mPriv->data.presences) {
        if (presence.id() == id) {
            return true;
        }
    }
    return false;
}

Profile::Presence Profile::presence(const QString &id) const
{
    foreach (const Presence &presence, mPriv->data.presences) {
        if (presence.id() == id) {
            return presence;
        }
    }
    return Profile::Presence();
}

RequestableChannelClassList Profile::unsupportedChannelClasses() const
{
    return mPriv->data.unsupportedChannelClasses;
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


struct TELEPATHY_QT4_NO_EXPORT Profile::Parameter::Private
{
    QString name;
    QDBusSignature dbusSignature;
    QVariant value;
    QString label;
    bool mandatory;
};

Profile::Parameter::Parameter()
    : mPriv(new Private)
{
    mPriv->mandatory = false;
}

Profile::Parameter::Parameter(const Parameter &other)
    : mPriv(new Private)
{
    mPriv->name = other.mPriv->name;
    mPriv->dbusSignature = other.mPriv->dbusSignature;
    mPriv->value = other.mPriv->value;
    mPriv->label = other.mPriv->label;
    mPriv->mandatory = other.mPriv->mandatory;
}

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

Profile::Parameter::~Parameter()
{
    delete mPriv;
}

QString Profile::Parameter::name() const
{
    return mPriv->name;
}

void Profile::Parameter::setName(const QString &name)
{
    mPriv->name = name;
}

QDBusSignature Profile::Parameter::dbusSignature() const
{
    return mPriv->dbusSignature;
}

void Profile::Parameter::setDBusSignature(const QDBusSignature &dbusSignature)
{
    mPriv->dbusSignature = dbusSignature;
}

QVariant::Type Profile::Parameter::type() const
{
    return variantTypeForSignature(mPriv->dbusSignature);
}

QVariant Profile::Parameter::value() const
{
    return mPriv->value;
}

void Profile::Parameter::setValue(const QVariant &value)
{
    mPriv->value = value;
}

QString Profile::Parameter::label() const
{
    return mPriv->label;
}

void Profile::Parameter::setLabel(const QString &label)
{
    mPriv->label = label;
}

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


struct TELEPATHY_QT4_NO_EXPORT Profile::Presence::Private
{
    QString id;
    QString label;
    QString iconName;
    QString message;
    bool disabled;
};

Profile::Presence::Presence()
    : mPriv(new Private)
{
    mPriv->disabled = false;
}

Profile::Presence::Presence(const Presence &other)
    : mPriv(new Private)
{
    mPriv->id = other.mPriv->id;
    mPriv->label = other.mPriv->label;
    mPriv->iconName = other.mPriv->iconName;
    mPriv->message = other.mPriv->message;
    mPriv->disabled = other.mPriv->disabled;
}

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

Profile::Presence::~Presence()
{
    delete mPriv;
}

QString Profile::Presence::id() const
{
    return mPriv->id;
}

void Profile::Presence::setId(const QString &id)
{
    mPriv->id = id;
}

QString Profile::Presence::label() const
{
    return mPriv->label;
}

void Profile::Presence::setLabel(const QString &label)
{
    mPriv->label = label;
}

QString Profile::Presence::iconName() const
{
    return mPriv->iconName;
}

void Profile::Presence::setIconName(const QString &iconName)
{
    mPriv->iconName = iconName;
}

QString Profile::Presence::message() const
{
    return mPriv->message;
}

void Profile::Presence::setMessage(const QString &message)
{
    mPriv->message = message;
}

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
