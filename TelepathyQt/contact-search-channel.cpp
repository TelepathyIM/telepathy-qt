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

#include <TelepathyQt/ContactSearchChannel>
#include "TelepathyQt/contact-search-channel-internal.h"

#include "TelepathyQt/_gen/contact-search-channel.moc.hpp"
#include "TelepathyQt/_gen/contact-search-channel-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/Types>

namespace Tp
{

struct TP_QT_NO_EXPORT ContactSearchChannel::Private
{
    Private(ContactSearchChannel *parent,
            const QVariantMap &immutableProperties);
    ~Private();

    static void introspectMain(Private *self);

    void extractImmutableProperties(const QVariantMap &props);

    void processSignalsQueue();
    void processSearchStateChangeQueue();
    void processSearchResultQueue();

    struct SearchStateChangeInfo
    {
        SearchStateChangeInfo(uint state, const QString &errorName,
                const Tp::ContactSearchChannel::SearchStateChangeDetails &details)
            : state(state), errorName(errorName), details(details)
        {
        }

        uint state;
        QString errorName;
        ContactSearchChannel::SearchStateChangeDetails details;
    };

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

    QQueue<void (Private::*)()> signalsQueue;
    QQueue<SearchStateChangeInfo> searchStateChangeQueue;
    QQueue<ContactSearchResultMap> searchResultQueue;
    bool processingSignalsQueue;
};

ContactSearchChannel::Private::Private(ContactSearchChannel *parent,
        const QVariantMap &immutableProperties)
    : parent(parent),
      immutableProperties(immutableProperties),
      contactSearchInterface(parent->interface<Client::ChannelTypeContactSearchInterface>()),
      properties(parent->interface<Client::DBus::PropertiesInterface>()),
      readinessHelper(parent->readinessHelper()),
      searchState(ChannelContactSearchStateNotStarted),
      limit(0),
      processingSignalsQueue(false)
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
        TP_QT_IFACE_CHANNEL + QLatin1String(".Limit"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".AvailableSearchKeys"),
        TP_QT_IFACE_CHANNEL + QLatin1String(".Server")
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
                        TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH),
                    self->parent);
        self->parent->connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(gotProperties(QDBusPendingCallWatcher*)));
    } else {
        self->extractImmutableProperties(props);

        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    self->properties->Get(
                        TP_QT_IFACE_CHANNEL_TYPE_CONTACT_SEARCH,
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

void ContactSearchChannel::Private::processSignalsQueue()
{
    if (processingSignalsQueue || signalsQueue.isEmpty()) {
        return;
    }

    processingSignalsQueue = true;
    (this->*(signalsQueue.dequeue()))();
}

void ContactSearchChannel::Private::processSearchStateChangeQueue()
{
    const SearchStateChangeInfo &info = searchStateChangeQueue.dequeue();

    searchState = info.state;
    emit parent->searchStateChanged(
            static_cast<ChannelContactSearchState>(info.state), info.errorName,
            SearchStateChangeDetails(info.details));

    processingSignalsQueue = false;
    processSignalsQueue();
}

void ContactSearchChannel::Private::processSearchResultQueue()
{
    const ContactSearchResultMap &result = searchResultQueue.first();
    if (!result.isEmpty()) {
        ContactManagerPtr manager = parent->connection()->contactManager();
        PendingContacts *pendingContacts = manager->contactsForIdentifiers(
                result.keys());
        parent->connect(pendingContacts,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotSearchResultContacts(Tp::PendingOperation*)));
    } else {
        searchResultQueue.dequeue();

        emit parent->searchResultReceived(SearchResult());

        processingSignalsQueue = false;
        processSignalsQueue();
    }
}

struct TP_QT_NO_EXPORT ContactSearchChannel::SearchStateChangeDetails::Private : public QSharedData
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

ContactSearchChannel::PendingSearch::PendingSearch(const ContactSearchChannelPtr &channel,
        QDBusPendingCall call)
    : PendingOperation(channel),
      mFinished(false)
{
    connect(channel.data(),
            SIGNAL(searchStateChanged(Tp::ChannelContactSearchState, const QString &,
                    const Tp::ContactSearchChannel::SearchStateChangeDetails &)),
            SLOT(onSearchStateChanged(Tp::ChannelContactSearchState, const QString &,
                    const Tp::ContactSearchChannel::SearchStateChangeDetails &)));
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

void ContactSearchChannel::PendingSearch::onSearchStateChanged(
        Tp::ChannelContactSearchState state, const QString &errorName,
        const Tp::ContactSearchChannel::SearchStateChangeDetails &details)
{
    if (state != ChannelContactSearchStateNotStarted) {
        if (mFinished) {
            if (mError.isValid()) {
                setFinishedWithError(mError);
            } else {
                setFinished();
            }
        }
        mFinished = true;
    }
}

void ContactSearchChannel::PendingSearch::watcherFinished(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        if (mFinished) {
            setFinishedWithError(watcher->error());
        } else {
            mError = watcher->error();
        }
    } else {
        if (mFinished) {
            setFinished();
        }
    }
    mFinished = true;

    watcher->deleteLater();
}

/**
 * \class ContactSearchChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/contact-search-channel.h <TelepathyQt/ContactSearchChannel>
 *
 * \brief The ContactSearchChannel class represents a Telepathy channel of type
 * ContactSearch.
 */

/**
 * \class ContactSearchChannel::SearchStateChangeDetails
 * \ingroup wrappers
 * \headerfile TelepathyQt/contact-search-channel.h <TelepathyQt/ContactSearchChannel>
 *
 * \brief The ContactSearchChannel::SearchStateChangeDetails class provides
 * a wrapper around the details for a search state change.
 *
 * \sa ContactSearchChannel
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
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A ContactSearchChannelPtr object pointing to the newly created
 *         ContactSearchChannel object.
 */
ContactSearchChannelPtr ContactSearchChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ContactSearchChannelPtr(new ContactSearchChannel(connection, objectPath,
                immutableProperties, ContactSearchChannel::FeatureCore));
}

/**
 * Construct a new ContactSearchChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on ContactSearchChannel::FeatureCore.
 */
ContactSearchChannel::ContactSearchChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
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
 * Change notification is via the searchStateChanged() signal.
 *
 * This method requires ContactSearchChannel::FeatureCore to be ready.
 *
 * \return The current search state as #ChannelContactSearchState.
 * \sa searchStateChanged()
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
 * This method requires ContactSearchChannel::FeatureCore to be ready.
 *
 * \return The maximum number of results, or 0 if there is no limit.
 * \sa availableSearchKeys(), search()
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
 * This method requires ContactSearchChannel::FeatureCore to be ready.
 *
 * \return The supported search keys.
 * \sa limit(), search()
 */
QStringList ContactSearchChannel::availableSearchKeys() const
{
    return mPriv->availableSearchKeys;
}

/**
 * Return the DNS name of the server being searched by this channel.
 *
 * This method requires ContactSearchChannel::FeatureCore to be ready.
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
 * This may only be called while the searchState() is #ChannelContactSearchStateNotStarted;
 * a valid search request will cause the searchStateChanged() signal to be emitted with the
 * state #ChannelContactSearchStateInProgress.
 *
 * Search results are signalled by searchResultReceived().
 *
 * This method requires ContactSearchChannel::FeatureCore to be ready.
 *
 * This is an overloaded method for search(const ContactSearchMap &searchTerms).
 *
 * \param searchKey The search key.
 * \param searchTerm The search term.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the search has started.
 * \sa searchState(), searchStateChanged(), searchResultReceived()
 */
PendingOperation *ContactSearchChannel::search(const QString &searchKey, const QString &searchTerm)
{
    ContactSearchMap searchTerms;
    searchTerms.insert(searchKey, searchTerm);
    return search(searchTerms);
}

/**
 * Send a request to start a search for contacts on this connection.
 *
 * This may only be called while the searchState() is #ChannelContactSearchStateNotStarted;
 * a valid search request will cause the searchStateChanged() signal to be emitted with the
 * state #ChannelContactSearchStateInProgress.
 *
 * Search results are signalled by searchResultReceived().
 *
 * This method requires ContactSearchChannel::FeatureCore to be ready.
 *
 * \param terms The search terms.
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the search has started.
 * \sa searchState(), searchStateChanged(), searchResultReceived()
 */
PendingOperation *ContactSearchChannel::search(const ContactSearchMap &terms)
{
    if (!isReady(FeatureCore)) {
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                ContactSearchChannelPtr(this));
    }

    if (searchState() != ChannelContactSearchStateNotStarted) {
        warning() << "ContactSearchChannel::search called with "
            "searchState() != ChannelContactSearchStateNotStarted. Doing nothing";
        return new PendingFailure(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Search already started"),
                ContactSearchChannelPtr(this));
    }

    return new PendingSearch(ContactSearchChannelPtr(this),
            mPriv->contactSearchInterface->Search(terms));
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

    (void) new PendingVoid(mPriv->contactSearchInterface->More(),
            ContactSearchChannelPtr(this));
}

/**
 * Stop the current search.
 *
 * This may not be called while the searchState() is #ChannelContactSearchStateNotStarted.
 * If called while the searchState() is #ChannelContactSearchStateInProgress,
 * searchStateChanged() will be emitted, with the state #ChannelContactSearchStateFailed and
 * the error #TP_QT_ERROR_CANCELLED.
 *
 * \return A PendingOperation which will emit PendingOperation::finished
 *         when the call has finished.
 * \sa searchState(), searchStateChanged()
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

    (void) new PendingVoid(mPriv->contactSearchInterface->Stop(),
            ContactSearchChannelPtr(this));
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
    mPriv->searchStateChangeQueue.enqueue(Private::SearchStateChangeInfo(state, error, details));
    mPriv->signalsQueue.enqueue(&Private::processSearchStateChangeQueue);
    mPriv->processSignalsQueue();
}

void ContactSearchChannel::onSearchResultReceived(const ContactSearchResultMap &result)
{
    mPriv->searchResultQueue.enqueue(result);
    mPriv->signalsQueue.enqueue(&Private::processSearchResultQueue);
    mPriv->processSignalsQueue();
}

void ContactSearchChannel::gotSearchResultContacts(PendingOperation *op)
{
    PendingContacts *pc = qobject_cast<PendingContacts *>(op);

    const ContactSearchResultMap &result = mPriv->searchResultQueue.dequeue();

    if (!pc->isValid()) {
        warning().nospace() << "Getting search result contacts "
            "failed with " << pc->errorName() << ":" <<
            pc->errorMessage() << ". Ignoring search result";
        mPriv->processingSignalsQueue = false;
        mPriv->processSignalsQueue();
        return;
    }

    const QList<ContactPtr> &contacts = pc->contacts();
    Q_ASSERT(result.count() == contacts.count());

    SearchResult ret;
    uint i = 0;
    for (ContactSearchResultMap::const_iterator it = result.constBegin();
                                                it != result.constEnd();
                                                ++it, ++i) {
        ret.insert(contacts.at(i), Contact::InfoFields(it.value()));
    }
    emit searchResultReceived(ret);

    mPriv->processingSignalsQueue = false;
    mPriv->processSignalsQueue();
}

/**
 * \fn void ContactSearchChannel::searchStateChanged(Tp::ChannelContactSearchState state,
 *          const QString &errorName,
 *          const Tp::ContactSearchChannel::SearchStateChangeDetails &details)
 *
 * Emitted when the value of searchState() changes.
 *
 * \param state The new state.
 * \param errorName The name of the error if any.
 * \param details The details for the state change.
 * \sa searchState()
 */

/**
 * \fn void ContactSearchChannel::searchResultReceived(
 *          const Tp::ContactSearchChannel::SearchResult &result)
 *
 * Emitted when a result for a search is received. It can be emitted multiple times
 * until the searchState() goes to #ChannelContactSearchStateCompleted or
 * #ChannelContactSearchStateFailed.
 *
 * \param result The search result.
 * \sa searchState()
 */

} // Tp
