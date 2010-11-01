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

#ifndef _TelepathyQt4_account_set_h_HEADER_GUARD_
#define _TelepathyQt4_account_set_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Filter>
#include <TelepathyQt4/Types>

#include <QList>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class TELEPATHY_QT4_EXPORT AccountSet : public QObject,
                      public RefCounted
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountSet)
    Q_PROPERTY(AccountManagerPtr accountManager READ accountManager)
    // FIXME: (API/ABI break) Remove filterValid
    Q_PROPERTY(bool filterValid READ _deprecated_isFilterValid)
    // FIXME: (API/ABI break) Remove filter
    Q_PROPERTY(QVariantMap filter READ _deprecated_filter)
    Q_PROPERTY(QList<AccountFilterConstPtr> filters READ filters)
    Q_PROPERTY(QList<AccountPtr> accounts READ accounts)

public:
    AccountSet(const AccountManagerPtr &accountManager,
            const QList<AccountFilterConstPtr > &filters);
    AccountSet(const AccountManagerPtr &accountManager,
            const QVariantMap &filter);
    virtual ~AccountSet();

    AccountManagerPtr accountManager() const;

    TELEPATHY_QT4_DEPRECATED bool isFilterValid() const;

    TELEPATHY_QT4_DEPRECATED QVariantMap filter() const;
    QList<AccountFilterConstPtr> filters() const;

    QList<AccountPtr> accounts() const;

Q_SIGNALS:
    void accountAdded(const Tp::AccountPtr &account);
    void accountRemoved(const Tp::AccountPtr &account);

private Q_SLOTS:
    void onNewAccount(const Tp::AccountPtr &account);
    void onAccountRemoved(const Tp::AccountPtr &account);
    void onAccountChanged(const Tp::AccountPtr &account);

private:
    struct Private;
    friend struct Private;

    bool _deprecated_isFilterValid() const;
    QVariantMap _deprecated_filter() const;

    Private *mPriv;
};

} // Tp

#endif
