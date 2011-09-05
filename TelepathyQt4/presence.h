/**
 * This file is part of TelepathyQt4
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

#ifndef _TelepathyQt4_presence_h_HEADER_GUARD_
#define _TelepathyQt4_presence_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT Presence
{
public:
    Presence();
    Presence(const SimplePresence &sp);
    Presence(ConnectionPresenceType type, const QString &status, const QString &statusMessage);
    Presence(const Presence &other);
    ~Presence();

    static Presence available(const QString &statusMessage = QString());
    static Presence away(const QString &statusMessage = QString());
    static Presence brb(const QString &statusMessage = QString());
    static Presence busy(const QString &statusMessage = QString());
    static Presence xa(const QString &statusMessage = QString());
    static Presence hidden(const QString &statusMessage = QString());
    static Presence offline(const QString &statusMessage = QString());

    bool isValid() const { return mPriv.constData() != 0; }

    Presence &operator=(const Presence &other);
    bool operator==(const Presence &other) const;

    ConnectionPresenceType type() const;
    QString status() const;
    QString statusMessage() const;
    void setStatus(const SimplePresence &value);
    void setStatus(ConnectionPresenceType type, const QString &status,
            const QString &statusMessage);

    SimplePresence barePresence() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TELEPATHY_QT4_EXPORT PresenceSpec
{
public:
    PresenceSpec();
    PresenceSpec(const QString &status, const SimpleStatusSpec &spec);
    PresenceSpec(const PresenceSpec &other);
    ~PresenceSpec();

    bool isValid() const { return mPriv.constData() != 0; }

    PresenceSpec &operator=(const PresenceSpec &other);

    Presence presence(const QString &statusMessage = QString()) const;
    bool maySetOnSelf() const;
    bool canHaveStatusMessage() const;

    SimpleStatusSpec bareSpec() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TELEPATHY_QT4_EXPORT PresenceSpecList : public QList<PresenceSpec>
{
public:
    PresenceSpecList() { }
    PresenceSpecList(const SimpleStatusSpecMap &specMap)
    {
        SimpleStatusSpecMap::const_iterator i = specMap.constBegin();
        SimpleStatusSpecMap::const_iterator end = specMap.end();
        for (; i != end; ++i) {
            QString status = i.key();
            SimpleStatusSpec spec = i.value();
            append(PresenceSpec(status, spec));
        }
    }
    PresenceSpecList(const QList<PresenceSpec> &other)
        : QList<PresenceSpec>(other)
    {
    }

    QMap<QString, PresenceSpec> toMap() const
    {
        QMap<QString, PresenceSpec> ret;
        Q_FOREACH (const PresenceSpec &spec, *this) {
            ret.insert(spec.presence().status(), spec);
        }
        return ret;
    }
};

} // Tp

Q_DECLARE_METATYPE(Tp::Presence);
Q_DECLARE_METATYPE(Tp::PresenceSpec);

#endif
