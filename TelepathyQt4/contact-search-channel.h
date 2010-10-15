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

#ifndef _TelepathyQt4_contact_search_channel_h_HEADER_GUARD_
#define _TelepathyQt4_contact_search_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Channel>

namespace Tp
{

class TELEPATHY_QT4_EXPORT ContactSearchChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(ContactSearchChannel)

public:
    static const Feature FeatureCore;

    class SearchStateChangeDetails
    {
    public:
        SearchStateChangeDetails();
        SearchStateChangeDetails(const SearchStateChangeDetails &other);
        ~SearchStateChangeDetails();

        bool isValid() const { return mPriv.constData() != 0; }

        SearchStateChangeDetails &operator=(const SearchStateChangeDetails &other);

        bool hasDebugMessage() const { return allDetails().contains(QLatin1String("debug-message")); }
        QString debugMessage() const { return qdbus_cast<QString>(allDetails().value(QLatin1String("debug-message"))); }

        QVariantMap allDetails() const;

    private:
        friend class ContactSearchChannel;

        SearchStateChangeDetails(const QVariantMap &details);

        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };

    typedef QHash<ContactPtr, Contact::InfoFields> SearchResult;

    static ContactSearchChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~ContactSearchChannel();

    ChannelContactSearchState searchState() const;
    uint limit() const;
    QStringList availableSearchKeys() const;
    QString server() const;

    PendingOperation *search(const QString &searchKey, const QString &searchTerm);
    PendingOperation *search(const ContactSearchMap &searchTerms);
    void continueSearch();
    void stopSearch();

Q_SIGNALS:
    void searchStateChanged(Tp::ChannelContactSearchState state, const QString &errorName,
            const Tp::ContactSearchChannel::SearchStateChangeDetails &details);
    void searchResultReceived(const Tp::ContactSearchChannel::SearchResult &result);

protected:
    ContactSearchChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties);

private Q_SLOTS:
    void gotProperties(QDBusPendingCallWatcher *watcher);
    void gotSearchState(QDBusPendingCallWatcher *watcher);
    void onSearchStateChanged(uint state, const QString &error, const QVariantMap &details);
    void onSearchResultReceived(const Tp::ContactSearchResultMap &result);
    void gotSearchResultContacts(Tp::PendingOperation *op);

private:
    class PendingSearch;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
