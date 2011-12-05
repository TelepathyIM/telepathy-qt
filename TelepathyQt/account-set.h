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

#ifndef _TelepathyQt_account_set_h_HEADER_GUARD_
#define _TelepathyQt_account_set_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Filter>
#include <TelepathyQt/Object>
#include <TelepathyQt/Types>

#include <QList>
#include <QString>
#include <QVariantMap>

namespace Tp
{

class TP_QT_EXPORT AccountSet : public Object
{
    Q_OBJECT
    Q_DISABLE_COPY(AccountSet)
    Q_PROPERTY(AccountManagerPtr accountManager READ accountManager)
    Q_PROPERTY(AccountFilterConstPtr filter READ filter)
    Q_PROPERTY(QList<AccountPtr> accounts READ accounts)

public:
    AccountSet(const AccountManagerPtr &accountManager,
            const AccountFilterConstPtr &filter);
    AccountSet(const AccountManagerPtr &accountManager,
            const QVariantMap &filter);
    virtual ~AccountSet();

    AccountManagerPtr accountManager() const;

    AccountFilterConstPtr filter() const;

    QList<AccountPtr> accounts() const;

Q_SIGNALS:
    void accountAdded(const Tp::AccountPtr &account);
    void accountRemoved(const Tp::AccountPtr &account);

private Q_SLOTS:
    TP_QT_NO_EXPORT void onNewAccount(const Tp::AccountPtr &account);
    TP_QT_NO_EXPORT void onAccountRemoved(const Tp::AccountPtr &account);
    TP_QT_NO_EXPORT void onAccountChanged(const Tp::AccountPtr &account);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
