/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#ifndef _TelepathyQt_message_content_part_h_HEADER_GUARD_
#define _TelepathyQt_message_content_part_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

namespace Tp
{

class TP_QT_EXPORT MessageContentPart
{
public:
    MessageContentPart();
    MessageContentPart(const MessagePart &mp);
    MessageContentPart(const MessageContentPart &other);
    ~MessageContentPart();

    bool isValid() const { return mPriv.constData() != 0; }

    MessageContentPart &operator=(const MessageContentPart &other);
    bool operator==(const MessageContentPart &other) const;

    MessagePart barePart() const;

private:
    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

class TP_QT_EXPORT MessageContentPartList :
                public QList<MessageContentPart>
{
public:
    MessageContentPartList() { }
    MessageContentPartList(const MessagePart &mp)
    {
        append(MessageContentPart(mp));
    }
    MessageContentPartList(const MessagePartList &mps)
    {
        Q_FOREACH (const MessagePart &mp, mps) {
            append(MessageContentPart(mp));
        }
    }
    MessageContentPartList(const MessageContentPart &mcp)
    {
        append(mcp);
    }
    MessageContentPartList(const QList<MessageContentPart> &other)
        : QList<MessageContentPart>(other)
    {
    }

    MessagePartList bareParts() const
    {
        MessagePartList list;
        Q_FOREACH (const MessageContentPart &mcp, *this) {
            list.append(mcp.barePart());
        }
        return list;
    }
};

} // Tp

Q_DECLARE_METATYPE(Tp::MessageContentPart);
Q_DECLARE_METATYPE(Tp::MessageContentPartList);

#endif
