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

#include <TelepathyQt4/ContactSearchChannel>

#include "TelepathyQt4/_gen/contact-search-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingFailure>
#include <TelepathyQt4/Types>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT ContactSearchChannel::Private
{
    Private(ContactSearchChannel *parent,
            const QVariantMap &immutableProperties);
    ~Private();

    static void introspectMain(Private *self);

    void extractImmutableProperties(const QVariantMap &props);

    void processSearchResultQueue();

    // Public object
    ContactSearchChannel *parent;

    QVariantMap immutableProperties;

    Client::ChannelTypeContactSearchInterface *contactSearchInterface;
    Client::DBus::PropertiesInterface *properties;

    ReadinessHelper *readinessHelper;

    // Introspection
    uint searchState;
    uint limit;
    QStringList availableSearchKeys;
    QString server;

    QQueue<ContactSearchResultMap> searchResultQueue;
    bool buildingSearchResultContacts;
};

ContactSearchChannel::Private::Private(ContactSearchChannel *parent,
        const QVariantMap &immutableProperties)
    : parent(parent),
      immutableProperties(immutableProperties),
      contactSearchInterface(parent->contactSearchInterface(BypassInterfaceCheck)),
      properties(0),
      readinessHelper(parent->readinessHelper()),
      buildingSearchResultContacts(false)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                                      // makesSenseForStatuses
        Features() << Channel::FeatureCore,                                     // dependsOnFeatures (core)
        QStringList(),                                                          // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper->addIntrospectables(introspectables);
}

ContactSearchChannel::Private::~Private()
{
}

void ContactSearchChannel::Private::introspectMain(ContactSearchChannel::Private *self)
{
    if (!self->properties) {
        self->properties = self->parent->propertiesInterface();
        Q_ASSERT(self->properties != 0);
    }

    /* we need to at least introspect SearchState here as it's not immutable */
    self->parent->connect(self->contactSearchInterface,
            SIGNAL(SearchStateChanged(uint,QString,QVariantMap)),
            SLOT(onSearchStateChanged(uint,QString,QVariantMap)));
    self->parent->connect(self->contactSearchInterface,
            SIGNAL(SearchResultReceived(Tp::ContactSearchResultMap)),
            SLOT(onSearchResultReceived(Tp::ContactSearchResultMap)));

    QVariantMap props;
    bool needIntrospectMainProps = false;
    const unsigned numNames = 3;
    const static QString names[numNames] = {
        QLatin1String("Limit"),
        QLatin1String("AvailableSearchKeys"),
        QLatin1String("Server")
    };
    const static QString qualifiedNames[numNames] = {
        QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Limit"),
        QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".AvailableSearchKeys"),
        QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Server")
    };
    for (unsigned i = 0; i < numNames; ++i) {
        const QString &qualified = qualifiedNames[i];
        if (!self->immutableProperties.contains(qualified)) {
            needIntrospectMainProps = true;
            break;
        }
        props.insert(names[i], self->immutableProperties.value(qualified));
    }

    if (needIntrospectMainProps) {
        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    self->properties->GetAll(
                        QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH)),
                    self->parent);
        self->parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotProperties(QDBusPendingCallWatcher*)));
    } else {
        self->extractImmutableProperties(props);

        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    self->properties->Get(
                        QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_SEARCH),
                        QLatin1String("SearchState")),
                    self->parent);
        self->parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotSearchState(QDBusPendingCallWatcher*)));
    }
}

void ContactSearchChannel::Private::extractImmutableProperties(const QVariantMap &props)
{
    limit = qdbus_cast<uint>(props[QLatin1String("Limit")]);
    availableSearchKeys = qdbus_cast<QStringList>(props[QLatin1String("AvailableSearchKeys")]);
    server = qdbus_cast<QString>(props[QLatin1String("Server")]);
}

void ContactSearchChannel::Private::processSearchResultQueue()
{
    if (buildingSearchResultContacts || searchResultQueue.isEmpty()) {
        return;
    }

    const ContactSearchResultMap &result = searchResultQueue.first();
    buildingSearchResultContacts = true;

    if (!result.isEmpty()) {
        ContactManager *manager = parent->connection()->contactManager();
        PendingContacts *pendingContacts = manager->contactsForIdentifiers(
                result.keys());
        parent->connect(pendingContacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotSearchResultContacts(Tp::PendingOperation*)));
    } else {
        emit parent->searchResultReceived(SearchResult());

        buildingSearchResultContacts = false;
        processSearchResultQueue();
    }
}

struct TELEPATHY_QT4_NO_EXPORT ContactSearchChannel::SearchStateChangeDetails::Private : public QSharedData
{
    Private(const QVariantMap &details)
        : details(details) {}

    QVariantMap details;
};

ContactSearchChannel::SearchStateChangeDetails::SearchStateChangeDetails(const QVariantMap &details)
    : mPriv(new Private(details))
{
}

ContactSearchChannel::SearchStateChangeDetails::SearchStateChangeDetails()
{
}

ContactSearchChannel::SearchStateChangeDetails::SearchStateChangeDetails(
        const ContactSearchChannel::SearchStateChangeDetails &other)
    : mPriv(other.mPriv)
{
}

ContactSearchChannel::SearchStateChangeDetails::~SearchStateChangeDetails()
{
}

ContactSearchChannel::SearchStateChangeDetails &ContactSearchChannel::SearchStateChangeDetails::operator=(
        const ContactSearchChannel::SearchStateChangeDetails &other)
{
    this->mPriv = other.mPriv;
    return *this;
}

QVariantMap ContactSearchChannel::SearchStateChangeDetails::allDetails() const
{
    return isValid() ? mPriv->details : QVariantMap();
}

/**
 * \class ContactSearchChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/contact-search-channel.h <TelepathyQt4/ContactSearchChannel>
 *
 * \brief The ContactSearchChannel class provides an object representing a
 * Telepathy channel of type ContactSearch.
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ContactSearchChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * ContactSearchChannel methods.
 * See specific methods documentation for more details.
 *
 * When calling isReady(), becomeReady(), this feature is implicitly added
 * to the requested features.
 */
const Feature ContactSearchChannel::FeatureCore = Feature(QLatin1String(ContactSearchChannel::staticMetaObject.className()), 0);

/**
 * Create a new ContactSearchChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The object path of this channel.
 * \param immutableProperties The immutable properties of this channel.
 * \return A ContactSearchChannelPtr object pointing to the newly created
 *         ContactSearchChannel object.
 */
ContactSearchChannelPtr ContactSearchChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ContactSearchChannelPtr(new ContactSearchChannel(connection, objectPath,
                immutableProperties));
}

/**
 * Construct a new contact search channel associated with the given \a objectPath
 * on the same service as the given \a connection.
 *
 * \param connection Connection owning this channel, and specifying the service.
 * \param objectPath Path to the object on the service.
 * \param immutableProperties The immutable properties of the channel.
 */
ContactSearchChannel::ContactSearchChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties)
    : Channel(connection, objectPath, immutableProperties),
      mPriv(new Private(this, immutableProperties))
{
}

/**
 * Class destructor.
 */
ContactSearchChannel::~ContactSearchChannel()
{
    delete mPriv;
}

/**
 * Return the current search state of this channel.
 *
 * Change notification is via searchStateChanged().
 *
 * \return The current search state of this channel.
 */
ChannelContactSearchState ContactSearchChannel::searchState() const
{
    return static_cast<ChannelContactSearchState>(mPriv->searchState);
}

/**
 * Return the maximum number of results that should be returned by calling search(), where
 * 0 represents no limit.
 *
 * For example, if the terms passed to search() match Antonius, Bridget and Charles and
 * this property is 2, the search service will only return Antonius and Bridget.
 *
 * This method requires ContactSearchChannel::FeatureCore to be enabled.
 *
 * \return The maximum number of results that should be returned by calling search().
 */
uint ContactSearchChannel::limit() const
{
    return mPriv->limit;
}

/**
 * Return the set of search keys supported by this channel.
 *
 * Example values include [""] (for protocols where several address fields are implicitly searched)
 * or ["x-n-given", "x-n-family", "nickname", "email"] (for XMPP XEP-0055, without extensibility via
 * Data Forms).
 *
 * This method requires ContactSearchChannel::FeatureCore to be enabled.
 *
 * \return The search keys supported by this channel.
 */
QStringList ContactSearchChannel::availableSearchKeys() const
{
    return mPriv->availableSearchKeys;
}

/**
 * Return the DNS name of the server being searched by this channel.
 *
 * This method requires ContactSearchChannel::FeatureCore to be enabled.
 *
 * \return For protocols which support searching for contacts on multiple servers with different DNS
 *         names (like XMPP), the DNS name of the server being searched by this channel, e.g.
 *         "characters.shakespeare.lit". Otherwise, an empty string.
 */
QString ContactSearchChannel::server() const
{
    return mPriv->server;
}

/**
 * Send a request to start a search for contacts on this connection.
 *
 * This may only be called while the searchState() is ChannelContactSearchStateNotStarted;
 * a valid search request will cause the searchStateChanged() signal to be emitted with the
 * state ChannelContactSearchStateInProgress.
 *
 * This method requires ContactSearchChannel::FeatureCore to be enabled.
 */
void ContactSearchChannel::search(const ContactSearchMap &terms)
{
    if (!isReady(FeatureCore)) {
        return;
    }

    if (searchState() != ChannelContactSearchStateNotStarted) {
        warning() << "ContactSearchChannel::search called with "
            "searchState() != ChannelContactSearchStateNotStarted. Doing nothing";
        return;
    }

    (void) new QDBusPendingCallWatcher(mPriv->contactSearchInterface->Search(terms), this);
}

/**
 * Request that a search which searchState() is ChannelContactSearchStateMoreAvailable
 * move back to state ChannelContactSearchStateInProgress and continue listing up to limit()
 * more results.
 */
void ContactSearchChannel::continueSearch()
{
    if (!isReady(FeatureCore)) {
        return;
    }

    if (searchState() != ChannelContactSearchStateMoreAvailable) {
        warning() << "ContactSearchChannel::continueSearch called with "
            "searchState() != ChannelContactSearchStateMoreAvailable. Doing nothing";
        return;
    }

    (void) new QDBusPendingCallWatcher(mPriv->contactSearchInterface->More(), this);
}

/**
 * Stop the current search.
 *
 * This may not be called while the searchState() is ChannelContactSearchStateNotStarted.
 * If called while the searchState() is ChannelContactSearchStateInProgress,
 * searchStateChanged() will be emitted, with the state ChannelContactSearchStateFailed and
 * the error #TELEPATHY_ERROR_CANCELLED.
 *
 * \return A PendingOperation, which will emit PendingOperation::finished
 *         when the call has finished.
 */
void ContactSearchChannel::stopSearch()
{
    if (!isReady(FeatureCore)) {
        return;
    }

    if (searchState() != ChannelContactSearchStateInProgress &&
        searchState() != ChannelContactSearchStateMoreAvailable) {
        warning() << "ContactSearchChannel::stopSearch called with "
            "searchState() != ChannelContactSearchStateInProgress or "
            "ChannelContactSearchStateMoreAvailable. Doing nothing";
        return;
    }

    (void) new QDBusPendingCallWatcher(mPriv->contactSearchInterface->Stop(), this);
}

void ContactSearchChannel::gotProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;

    if (!reply.isError()) {
        QVariantMap props = reply.value();
        mPriv->extractImmutableProperties(props);

        mPriv->searchState = qdbus_cast<uint>(props[QLatin1String("SearchState")]);

        debug() << "Got reply to Properties::GetAll(ContactSearchChannel)";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        warning().nospace() << "Properties::GetAll(ContactSearchChannel) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                reply.error());
    }

    watcher->deleteLater();
}

void ContactSearchChannel::gotSearchState(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariant> reply = *watcher;

    if (!reply.isError()) {
        mPriv->searchState = qdbus_cast<uint>(reply.value());

        debug() << "Got reply to Properties::Get(SearchState)";
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);
    } else {
        warning().nospace() << "Properties::Get(SearchState) failed "
            "with " << reply.error().name() << ": " << reply.error().message();
        mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, false,
                reply.error());
    }

    watcher->deleteLater();
}

void ContactSearchChannel::onSearchStateChanged(uint state, const QString &error,
        const QVariantMap &details)
{
    mPriv->searchState = state;
    emit searchStateChanged(static_cast<ChannelContactSearchState>(state), error,
            SearchStateChangeDetails(details));
}

void ContactSearchChannel::onSearchResultReceived(const ContactSearchResultMap &result)
{
    mPriv->searchResultQueue.enqueue(result);
    mPriv->processSearchResultQueue();
}

void ContactSearchChannel::gotSearchResultContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);

    const ContactSearchResultMap &result = mPriv->searchResultQueue.dequeue();

    if (!pc->isValid()) {
        warning().nospace() << "Getting search result contacts "
            "failed with " << pc->errorName() << ":" <<
            pc->errorMessage();
        mPriv->buildingSearchResultContacts = false;
        mPriv->processSearchResultQueue();
        return;
    }

    const QList<ContactPtr> &contacts = pc->contacts();
    Q_ASSERT(result.count() == contacts.count());

    SearchResult ret;
    uint i = 0;
    for (ContactSearchResultMap::const_iterator it = result.constBegin();
                                                it != result.constEnd();
                                                ++it, ++i) {
        ret.insert(contacts.at(i), it.value());
    }
    emit searchResultReceived(ret);

    mPriv->buildingSearchResultContacts = false;
    mPriv->processSearchResultQueue();
}

} // Tp
