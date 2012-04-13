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

#ifndef _TelepathyQt_presence_h_HEADER_GUARD_
#define _TelepathyQt_presence_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

namespace Tp
{

class TP_QT_EXPORT Presence
{
public:
    Presence();
    Presence(const SimplePresence &sp);
    Presence(ConnectionPresenceType type, const QString &status, const QString &statusMessage);
    Presence(const Presence &other);
    ~Presence();

    static Presence available(const QString &statusMessage = QString());
    static Presence chat(const QString &statusMessage = QString());
    static Presence away(const QString &statusMessage = QString());
    static Presence brb(const QString &statusMessage = QString());
    static Presence busy(const QString &statusMessage = QString());
    static Presence dnd(const QString &statusMessage = QString());
    static Presence xa(const QString &statusMessage = QString());
    static Presence hidden(const QString &statusMessage = QString());
    static Presence offline(const QString &statusMessage = QString());

    bool isValid() const { return mPriv.constData() != 0; }

    Presence &operator=(const Presence &other);
    bool operator==(const Presence &other) const;
    bool operator!=(const Presence &other) const;

    ConnectionPresenceType type() const;
    QString status() const;
    QString statusMessage() const;
    void setStatus(const SimplePresence &value);
    void setStatus(ConnectionPresenceType type, const QString &status,
            const QString &statusMessage);
    void setStatusMessage(const QString &statusMessage);

    SimplePresence barePresence() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TP_QT_EXPORT PresenceSpec
{
public:
    enum SimpleStatusFlag {
        NoFlags = 0,
        MaySetOnSelf = 0x1,
        CanHaveStatusMessage = 0x2,
        AllFlags = MaySetOnSelf | CanHaveStatusMessage
    };
    Q_DECLARE_FLAGS(SimpleStatusFlags, SimpleStatusFlag);

    PresenceSpec();
    PresenceSpec(const QString &status, const SimpleStatusSpec &spec);
    PresenceSpec(const PresenceSpec &other);
    ~PresenceSpec();

    static PresenceSpec available(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec chat(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec pstn(SimpleStatusFlags flags = CanHaveStatusMessage);
    static PresenceSpec away(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec brb(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec dnd(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec busy(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec xa(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec hidden(SimpleStatusFlags flags = AllFlags);
    static PresenceSpec offline(SimpleStatusFlags flags = CanHaveStatusMessage);
    static PresenceSpec unknown(SimpleStatusFlags flags = CanHaveStatusMessage);
    static PresenceSpec error(SimpleStatusFlags flags = CanHaveStatusMessage);

    bool isValid() const { return mPriv.constData() != 0; }

    PresenceSpec &operator=(const PresenceSpec &other);
    bool operator==(const PresenceSpec &other) const;
    bool operator!=(const PresenceSpec &other) const;
    bool operator<(const PresenceSpec &other) const;

    Presence presence(const QString &statusMessage = QString()) const;
    bool maySetOnSelf() const;
    bool canHaveStatusMessage() const;

    SimpleStatusSpec bareSpec() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TP_QT_EXPORT PresenceSpecList : public QList<PresenceSpec>
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

    SimpleStatusSpecMap bareSpecs() const
    {
        SimpleStatusSpecMap ret;
        Q_FOREACH (const PresenceSpec &spec, *this) {
            ret.insert(spec.presence().status(), spec.bareSpec());
        }
        return ret;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PresenceSpec::SimpleStatusFlags)

} // Tp

Q_DECLARE_METATYPE(Tp::Presence);
Q_DECLARE_METATYPE(Tp::PresenceSpec);
Q_DECLARE_METATYPE(Tp::PresenceSpecList);

#endif
