#include <TelepathyQt/Room>

#include "TelepathyQt/_gen/room.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/future-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/Channel>
#include <TelepathyQt/Constants>

namespace Tp
{

struct TP_QT_NO_EXPORT Room::Private
{
    Private(Room *parent, const Tp::ConnectionPtr &connection, uint handle);
    ~Private();

    Room *m_parent;
    Tp::ConnectionPtr m_connection;

    uint m_handle;
    QString m_id;

    Features m_requestedFeatures;
    Features m_actualFeatures;
    Features m_supportedFeatures;

    // RoomConfig
    bool m_moderated;
    QString m_title;
};

Room::Private::Private(Room *parent, const ConnectionPtr &connection, uint handle) :
      m_parent(parent),
      m_connection(connection),
      m_handle(handle)
{
}

Room::Private::~Private()
{
}

/**
 * Feature used in order to access room configuration data info.
 * *
 */
const Feature Room::FeatureRoomConfig = Feature(QLatin1String(Room::staticMetaObject.className()), 0, false);

Room::~Room()
{
}

/**
 * Return the features that are actually enabled on this room.
 *
 * \return The actual features as a set of Feature objects.
 */
Features Room::actualFeatures() const
{
    return mPriv->m_actualFeatures;
}

bool Room::moderated() const
{
    if (!mPriv->m_actualFeatures.contains(FeatureRoomConfig)) {
        warning() << "Room::moderated() used on" << this
            << "for which FeatureRoomConfig is not available (yet)";
        return false;
    }

    return mPriv->m_moderated;
}

QString Room::title() const
{
    if (!mPriv->m_actualFeatures.contains(FeatureRoomConfig)) {
        warning() << "Room::title() used on" << this
            << "for which FeatureRoomConfig is not available (yet)";
        return QString();
    }

    return mPriv->m_title;
}

Room::Room(const Tp::ConnectionPtr &connection, uint handle)
    : Object(),
      mPriv(new Private(this, connection, handle))
{
}

void Room::receiveRoomConfig(const QVariantMap &props)
{
    mPriv->m_moderated = qdbus_cast<bool>(props[QLatin1String("Moderated")]);
    mPriv->m_title = qdbus_cast<QString>(props[QLatin1String("Title")]);
    mPriv->m_actualFeatures.insert(FeatureRoomConfig);
}

void Room::updateRoomConfigProperties(const QVariantMap &changedProperties)
{
    foreach (const QString &key, changedProperties.keys()) {
        if (key == QLatin1String("Title")) {
            const QString newTitle = qdbus_cast<QString>(changedProperties[QLatin1String("Title")]);
            if (mPriv->m_title != newTitle) {
                mPriv->m_title = newTitle;
                emit titleChanged(newTitle);
            }
        } else {
            warning() << "Room::updateRoomConfigProperties(): Unhandled key" << key << "with value" << changedProperties.value(key);
        }
    }
}

} // Tp
