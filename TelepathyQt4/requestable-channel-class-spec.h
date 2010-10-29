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

#ifndef _TelepathyQt4_requestable_channel_class_spec_h_HEADER_GUARD_
#define _TelepathyQt4_requestable_channel_class_spec_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT RequestableChannelClassSpec
{
public:
    RequestableChannelClassSpec();
    RequestableChannelClassSpec(const RequestableChannelClass &rcc);
    RequestableChannelClassSpec(const RequestableChannelClassSpec &other);
    ~RequestableChannelClassSpec();

    static RequestableChannelClassSpec textChat();
    static RequestableChannelClassSpec textChatroom();

    static RequestableChannelClassSpec mediaCall();
    static RequestableChannelClassSpec audioCall();
    static RequestableChannelClassSpec videoCall();
    static RequestableChannelClassSpec videoCallWithAudio();

    static RequestableChannelClassSpec conferenceTextChat();
    static RequestableChannelClassSpec conferenceTextChatWithInvitees();
    static RequestableChannelClassSpec conferenceTextChatroom();
    static RequestableChannelClassSpec conferenceTextChatroomWithInvitees();
    static RequestableChannelClassSpec conferenceMediaCall();
    static RequestableChannelClassSpec conferenceMediaCallWithInvitees();

    static RequestableChannelClassSpec contactSearch();
    static RequestableChannelClassSpec contactSearchWithSpecificServer();
    static RequestableChannelClassSpec contactSearchWithLimit();
    static RequestableChannelClassSpec contactSearchWithSpecificServerAndLimit();

    bool isValid() const { return mPriv.constData() != 0; }

    RequestableChannelClassSpec &operator=(const RequestableChannelClassSpec &other);
    bool operator==(const RequestableChannelClassSpec &other) const;

    bool supports(const RequestableChannelClassSpec &spec) const;

    QString channelType() const;

    bool hasTargetHandleType() const;
    uint targetHandleType() const;

    bool hasFixedProperty(const QString &name) const;
    QVariant fixedProperty(const QString &name) const;
    QVariantMap fixedProperties() const;

    bool allowsProperty(const QString &name) const;
    QStringList allowedProperties() const;

    RequestableChannelClass bareClass() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TELEPATHY_QT4_EXPORT RequestableChannelClassSpecList :
                public QList<RequestableChannelClassSpec>
{
public:
    RequestableChannelClassSpecList() { }
    RequestableChannelClassSpecList(const RequestableChannelClass &rcc)
    {
        append(RequestableChannelClassSpec(rcc));
    }
    RequestableChannelClassSpecList(const RequestableChannelClassList &rccs)
    {
        Q_FOREACH (const RequestableChannelClass &rcc, rccs) {
            append(RequestableChannelClassSpec(rcc));
        }
    }
    RequestableChannelClassSpecList(const RequestableChannelClassSpec &rccSpec)
    {
        append(rccSpec);
    }
    RequestableChannelClassSpecList(const QList<RequestableChannelClassSpec> &other)
        : QList<RequestableChannelClassSpec>(other)
    {
    }
};

} // Tp

#endif
