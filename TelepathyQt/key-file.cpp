/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008-2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008-2009 Nokia Corporation
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

#include "TelepathyQt/key-file.h"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Utils>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Tp
{

struct TP_QT_NO_EXPORT KeyFile::Private
{
    Private();
    Private(const QString &fName);

    void setFileName(const QString &fName);
    void setError(KeyFile::Status status, const QString &reason);
    bool read();

    bool validateKey(const QByteArray &data, int from, int to, QString &result);

    QStringList allGroups() const;
    QStringList allKeys() const;
    QStringList keys() const;
    bool contains(const QString &key) const;
    QString rawValue(const QString &key) const;
    QString value(const QString &key) const;
    QStringList valueAsStringList(const QString &key) const;

    QString fileName;
    KeyFile::Status status;
    QHash<QString, QHash<QString, QByteArray> > groups;
    QString currentGroup;
};

KeyFile::Private::Private()
    : status(KeyFile::None)
{
}

KeyFile::Private::Private(const QString &fName)
    : fileName(fName),
      status(KeyFile::NoError)
{
    read();
}

void KeyFile::Private::setFileName(const QString &fName)
{
    fileName = fName;
    status = KeyFile::NoError;
    currentGroup = QString();
    groups.clear();
    read();
}

void KeyFile::Private::setError(KeyFile::Status st, const QString &reason)
{
    warning() << QString(QLatin1String("ERROR: filename(%1) reason(%2)"))
                         .arg(fileName).arg(reason);
    status = st;
    groups.clear();
}

bool KeyFile::Private::read()
{
    QFile file(fileName);
    if (!file.exists()) {
        setError(KeyFile::NotFoundError,
                 QLatin1String("file does not exist"));
        return false;
    }

    if (!file.open(QFile::ReadOnly)) {
        setError(KeyFile::AccessError,
                 QLatin1String("cannot open file for readonly access"));
        return false;
    }

    QByteArray data;
    QByteArray group;
    QString currentGroup;
    QHash<QString, QByteArray> groupMap;
    QByteArray rawValue;
    int line = 0;
    int idx;
    while (!file.atEnd()) {
        data = file.readLine().trimmed();
        line++;

        if (data.size() == 0) {
            // skip empty lines
            continue;
        }

        char ch = data.at(0);
        if (ch == '#') {
            // skip comments
            continue;
        }
        else if (ch == '[') {
            if (groupMap.size()) {
                groups[currentGroup] = groupMap;
                groupMap.clear();
            }

            idx = data.indexOf(']');
            if (idx == -1) {
                // line starts with [ and it's not a group
                setError(KeyFile::FormatError,
                         QString(QLatin1String("invalid group at line %2 - missing ']'"))
                                 .arg(line));
                return false;
            }

            group = data.mid(1, idx - 1).trimmed();
            if (groups.contains(QLatin1String(group))) {
                setError(KeyFile::FormatError,
                         QString(QLatin1String("duplicated group '%1' at line %2"))
                                 .arg(QLatin1String(group)).arg(line));
                return false;
            }

            currentGroup = QLatin1String("");
            if (!unescapeString(group, 0, group.size(), currentGroup)) {
                setError(KeyFile::FormatError,
                         QString(QLatin1String("invalid group '%1' at line %2"))
                                 .arg(currentGroup).arg(line));
                return false;
            }
        }
        else {
            idx = data.indexOf('=');
            if (idx == -1) {
                setError(KeyFile::FormatError,
                         QString(QLatin1String("format error at line %1 - missing '='"))
                                 .arg(line));
                return false;
            }

            // remove trailing spaces
            char ch;
            int idxKeyEnd = idx;
            while ((ch = data.at(idxKeyEnd - 1)) == ' ' || ch == '\t') {
                --idxKeyEnd;
            }

            QString key;
            if (!validateKey(data, 0, idxKeyEnd, key)) {
                setError(KeyFile::FormatError,
                         QString(QLatin1String("invalid key '%1' at line %2"))
                                 .arg(key).arg(line));
                return false;
            }

            if (groupMap.contains(key)) {
                setError(KeyFile::FormatError,
                         QString(QLatin1String("duplicated key '%1' on group '%2' at line %3"))
                                 .arg(key).arg(currentGroup).arg(line));
                return false;
            }

            data = data.mid(idx + 1).trimmed();
            rawValue = data.mid(0, data.size());
            groupMap[key] = rawValue;
        }
    }

    if (groupMap.size()) {
        groups[currentGroup] = groupMap;
        groupMap.clear();
    }

    return true;
}

bool KeyFile::Private::validateKey(const QByteArray &data, int from, int to, QString &result)
{
    int i = from;
    bool ret = true;
    while (i < to) {
        uint ch = data.at(i++);
        // as an extension to the Desktop Entry spec, we allow " ", "_", "." and "@"
        // as valid key characters - "_" and "." are needed for keys that are
        // D-Bus property names, and GKeyFile and KConfigIniBackend also accept
        // all four of those characters.
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              (ch == ' ') ||
              (ch == '-') || (ch == '_') ||
              (ch == '.') || (ch == '@'))) {
            ret = false;
        }
        result += ch;
    }
    return ret;
}

QStringList KeyFile::Private::allGroups() const
{
    return groups.keys();
}

QStringList KeyFile::Private::allKeys() const
{
    QStringList keys;
    QHash<QString, QHash<QString, QByteArray> >::const_iterator itrGroups = groups.begin();
    while (itrGroups != groups.end()) {
        keys << itrGroups.value().keys();
        ++itrGroups;
    }
    return keys;
}

QStringList KeyFile::Private::keys() const
{
    QHash<QString, QByteArray> groupMap = groups[currentGroup];
    return groupMap.keys();
}

bool KeyFile::Private::contains(const QString &key) const
{
    QHash<QString, QByteArray> groupMap = groups[currentGroup];
    return groupMap.contains(key);
}

QString KeyFile::Private::rawValue(const QString &key) const
{
    QHash<QString, QByteArray> groupMap = groups[currentGroup];
    QByteArray rawValue = groupMap.value(key);
    return QLatin1String(rawValue);
}

QString KeyFile::Private::value(const QString &key) const
{
    QHash<QString, QByteArray> groupMap = groups[currentGroup];
    QString result;
    QByteArray rawValue = groupMap.value(key);
    if (unescapeString(rawValue, 0, rawValue.size(), result)) {
        return result;
    }
    return QString();
}

QStringList KeyFile::Private::valueAsStringList(const QString &key) const
{
    QHash<QString, QByteArray> groupMap = groups[currentGroup];
    QStringList result;
    QByteArray rawValue = groupMap.value(key);
    if (unescapeStringList(rawValue, 0, rawValue.size(), result)) {
        return result;
    }
    return QStringList();
}

/**
 * \class KeyFile
 * \ingroup utils
 * \headerfile TelepathyQt/key-file.h <TelepathyQt/KeyFile>
 *
 * \brief The KeyFile class provides an easy way to read key-pair files such as
 * INI style files and .desktop files.
 *
 * It follows the rules regarding string escaping as defined in
 * http://standards.freedesktop.org/desktop-entry-spec/latest/index.html
 */

/**
 * Create a KeyFile object used to read (key-pair) compliant files.
 *
 * The status will be KeyFile::None
 * \sa setFileName()
 */
KeyFile::KeyFile()
    : mPriv(new Private())
{
}

/**
 * Create a KeyFile object used to read (key-pair) compliant files.
 *
 * \param fileName Name of the file to be read.
 */
KeyFile::KeyFile(const QString &fileName)
    : mPriv(new Private(fileName))
{
}

/**
 * Create a KeyFile object used to read (key-pair) compliant files.
 */
KeyFile::KeyFile(const KeyFile &other)
    : mPriv(new Private())
{
    mPriv->fileName = other.mPriv->fileName;
    mPriv->status = other.mPriv->status;
    mPriv->groups = other.mPriv->groups;
    mPriv->currentGroup = other.mPriv->currentGroup;
}

/**
 * Class destructor.
 */
KeyFile::~KeyFile()
{
    delete mPriv;
}

KeyFile &KeyFile::operator=(const KeyFile &other)
{
    mPriv->fileName = other.mPriv->fileName;
    mPriv->status = other.mPriv->status;
    mPriv->groups = other.mPriv->groups;
    mPriv->currentGroup = other.mPriv->currentGroup;
    return *this;
}

/**
 * Set the name of the file to be read.
 *
 * \param fileName Name of the file to be read.
 */
void KeyFile::setFileName(const QString &fileName)
{
    mPriv->setFileName(fileName);
}

/**
 * Return the name of the file associated with this object.
 *
 * \return Name of the file associated with this object.
 */
QString KeyFile::fileName() const
{
    return mPriv->fileName;
}

/**
 * Return a status code indicating the first error that was met by #KeyFile,
 * or KeyFile::NoError if no error occurred.
 *
 * Make sure to use this method if you set the filename to be read using
 * setFileName().
 *
 * \return Status code.
 * \sa setFileName()
 */
KeyFile::Status KeyFile::status() const
{
    return mPriv->status;
}

/**
 * Set the current group to be used while reading keys.
 *
 * Query functions such as keys(), contains() and value() are based on this
 * group.
 *
 * By default a empty group is used as the group for global
 * keys and is used as the default group if none is set.
 *
 * \param group Name of the group to be used.
 * \sa group()
 */
void KeyFile::setGroup(const QString &group)
{
    mPriv->currentGroup = group;
}

/**
 * Return the current group.
 *
 * \return Name of the current group.
 * \sa setGroup()
 */
QString KeyFile::group() const
{
    return mPriv->currentGroup;
}

/**
 * Return all groups in the desktop file.
 *
 * Global keys will be added to a empty group.
 *
 * \return List of all groups in the desktop file.
 */
QStringList KeyFile::allGroups() const
{
    return mPriv->allGroups();
}

/**
 * Return all keys described in the desktop file.
 *
 * \return List of all keys in the desktop file.
 */
QStringList KeyFile::allKeys() const
{
    return mPriv->allKeys();
}

/**
 * Return a list of keys in the current group.
 *
 * \return List of all keys in the current group.
 * \sa group(), setGroup()
 */
QStringList KeyFile::keys() const
{
    return mPriv->keys();
}

/**
 * Check if the current group contains a key named \a key.
 *
 * \return true if \a key exists, false otherwise.
 * \sa group(), setGroup()
 */
bool KeyFile::contains(const QString &key) const
{
    return mPriv->contains(key);
}

/**
 * Get the raw value for the key in the current group named \a key.
 *
 * The raw value is the value as is in the key file.
 *
 * \return Value of \a key, empty string if not found.
 * \sa group(), setGroup()
 */
QString KeyFile::rawValue(const QString &key) const
{
    return mPriv->rawValue(key);
}

/**
 * Get the value for the key in the current group named \a key.
 *
 * Escape sequences in the value are interpreted as defined in:
 * http://standards.freedesktop.org/desktop-entry-spec/latest/
 *
 * \return Value of \a key, empty string if not found or an error occurred.
 * \sa group(), setGroup()
 */
QString KeyFile::value(const QString &key) const
{
    return mPriv->value(key);
}

/**
 * Get the value for the key in the current group named \a key as a list.
 *
 * Return a list containing all strings on this key separated by ';'.
 * Escape sequences in the value are interpreted as defined in:
 * http://standards.freedesktop.org/desktop-entry-spec/latest/
 *
 * \return Value of \a key as a list, empty string list if not found or an error occurred.
 * \sa group(), setGroup()
 */
QStringList KeyFile::valueAsStringList(const QString &key) const
{
    return mPriv->valueAsStringList(key);
}

bool KeyFile::unescapeString(const QByteArray &data, int from, int to, QString &result)
{
    int i = from;
    while (i < to) {
        uint ch = data.at(i++);

        if (ch == '\\') {
            if (i == to) {
                result += QLatin1String("\\");
                return true;
            }

            char nextCh = data.at(i++);
            switch (nextCh) {
                case 's':
                    result += QLatin1String(" ");
                    break;
                case 'n':
                    result += QLatin1String("\n");
                    break;
                case 't':
                    result += QLatin1String("\t");
                    break;
                case 'r':
                    result += QLatin1String("\r");
                    break;
                case ';':
                    result += QLatin1String(";");
                    break;
                case '\\':
                    result += QLatin1String("\\");
                    break;
                default:
                    return false;
            }
        } else {
            result += ch;
        }
    }

    return true;
}

bool KeyFile::unescapeStringList(const QByteArray &data, int from, int to, QStringList &result)
{
    QByteArray value;
    QList<QByteArray> valueList;
    int i = from;
    char ch;
    while (i < to) {
        ch = data.at(i++);

        if (ch == '\\') {
            value += ch;
            if (i < to) {
                value += data.at(i++);
                continue;
            } else {
                valueList << value;
                break;
            }
        } else if (ch == ';') {
            valueList << value;
            value = "";
        } else {
            value += ch;
            if (i == to) {
                valueList << value;
            }
        }
    }

    foreach (value, valueList) {
        QString str;
        if (!unescapeString(value, 0, value.size(), str)) {
            return false;
        }
        result << str;
    }

    return true;
}

} // Tp
