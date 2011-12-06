/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt_key_file_h_HEADER_GUARD_
#define _TelepathyQt_key_file_h_HEADER_GUARD_

#include <TelepathyQt/Global>

#include <QMetaType>
#include <QtGlobal>

class QString;
class QStringList;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace Tp
{

class TP_QT_NO_EXPORT KeyFile
{
public:
    enum Status {
        None = 0,
        NoError,
        NotFoundError,
        AccessError,
        FormatError,
    };

    KeyFile();
    KeyFile(const KeyFile &other);
    KeyFile(const QString &fileName);
    ~KeyFile();

    KeyFile &operator=(const KeyFile &other);

    void setFileName(const QString &fileName);
    QString fileName() const;

    Status status() const;

    void setGroup(const QString &group);
    QString group() const;

    QStringList allGroups() const;
    QStringList allKeys() const;
    QStringList keys() const;
    bool contains(const QString &key) const;

    QString rawValue(const QString &key) const;
    QString value(const QString &key) const;
    QStringList valueAsStringList(const QString &key) const;

    static bool unescapeString(const QByteArray &data, int from, int to,
        QString &result);
    static bool unescapeStringList(const QByteArray &data, int from, int to,
        QStringList &result);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

Q_DECLARE_METATYPE(Tp::KeyFile);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
