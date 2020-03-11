/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2020 Alexandr Akulich <akulichalexander@gmail.com>
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

#include <TelepathyQt/ChatManager>

#include "TelepathyQt/_gen/chat-manager.moc.hpp"

#include "TelepathyQt/channel-requests-properties-internal.h"
#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/pending-chat-read.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/Utils>

#include <QMap>

namespace Tp
{

struct TP_QT_NO_EXPORT ChatManager::Private
{
    Private(ChatManager *parent, Connection *connection);

    Features realFeatures(const Features &features);
    QSet<QString> interfacesForFeatures(const Features &features);

    ChatManager *parent;
    WeakPtr<Connection> connection;

    QHash<Feature, bool> tracking;
    Features supportedFeatures;
};

ChatManager::Private::Private(ChatManager *parent, Connection *connection)
    : parent(parent),
      connection(connection)
{
}

Features ChatManager::Private::realFeatures(const Features &features)
{
    Features ret(features);
    ret.unite(parent->connection()->actualFeatures());
    return ret;
}

QSet<QString> ChatManager::Private::interfacesForFeatures(const Features &features)
{
    Features supported = parent->supportedFeatures();
    QSet<QString> ret;
    foreach (const Feature &feature, features) {
        parent->ensureTracking(feature);

        if (supported.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            ret.insert(parent->featureToInterface(feature));
        }
    }
    return ret;
}

/**
 * \class ChatManager
 * \ingroup clientconn
 * \headerfile TelepathyQt/contact-manager.h <TelepathyQt/ChatManager>
 *
 * \brief The ChatManager class is responsible for managing chats.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Construct a new ChatManager object.
 *
 * \param connection The connection owning this ChatManager.
 */
ChatManager::ChatManager(Connection *connection)
    : Object(),
      mPriv(new Private(this, connection))
{
}

/**
 * Class destructor.
 */
ChatManager::~ChatManager()
{
    delete mPriv;
}

/**
 * Return the connection owning this ChatManager.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr ChatManager::connection() const
{
    return ConnectionPtr(mPriv->connection);
}

/**
 * Return the features that are expected to work on chats on this ChatManager connection.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The supported features as a set of Feature objects.
 */
Features ChatManager::supportedFeatures() const
{
    if (mPriv->supportedFeatures.isEmpty()) {
        Features allFeatures = Features()
                << Connection::FeatureChatRead
                ;
        const QStringList interfaces = connection()->interfaces();
        for (const Feature &feature : allFeatures) {
            if (interfaces.contains(featureToInterface(feature))) {
                mPriv->supportedFeatures.insert(feature);
            }
        }

        debug() << mPriv->supportedFeatures.size() << "chat features supported using" << this;
    }

    return mPriv->supportedFeatures;
}

PendingOperation *Tp::ChatManager::markTextChatRead(const QString &contactIdentifier, const QString &messageToken)
{
    QVariantMap request = textChatRequest(contactIdentifier);
    return markChatRead(request, messageToken);
}

PendingOperation *Tp::ChatManager::markTextChatroomRead(const QString &roomIdentifier, const QString &messageToken)
{
    QVariantMap request = textChatroomRequest(roomIdentifier);
    return markChatRead(request, messageToken);
}

PendingOperation *Tp::ChatManager::markChatRead(const QVariantMap &channelRequest, const QString &messageToken)
{
    return new PendingChatReadOperation(connection(), channelRequest, messageToken);
}

QString ChatManager::featureToInterface(const Feature &feature)
{
    if (feature == Connection::FeatureChatRead) {
        return TP_QT_IFACE_CONNECTION_INTERFACE_CHAT_READ;
    }

    warning() << "ChatManager doesn't know which interface corresponds to feature"
              << feature;
    return QString();
}

void ChatManager::ensureTracking(const Feature &feature)
{
    if (mPriv->tracking[feature]) {
        return;
    }

    if (feature == Connection::FeatureChatRead) {
        // nothing to do here, but we don't want to warn
        ;
    } else {
        warning() << " Unknown feature" << feature
                  << "when trying to figure out how to connect change notification!";
    }

    mPriv->tracking[feature] = true;
}

} // Tp
