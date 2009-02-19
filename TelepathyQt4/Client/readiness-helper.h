/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#ifndef _TelepathyQt4_cli_readiness_helper_h_HEADER_GUARD_
#define _TelepathyQt4_cli_readiness_helper_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Client/DBusProxy>

#include <QMap>
#include <QSet>
#include <QStringList>

namespace Telepathy
{
namespace Client
{

class PendingReady;

class ReadinessHelper : public QObject
{
    Q_OBJECT

public:
    typedef void (*IntrospectFunc)(void *data);

    struct Introspectable {
    public:
        Introspectable()
            : introspectFunc(0),
              introspectFuncData(0)
        {
        }

        Introspectable(const QSet<uint> &makesSenseForStatuses,
                const QSet<uint> &dependsOnFeatures,
                const QStringList &dependsOnInterfaces,
                IntrospectFunc introspectFunc,
                void *introspectFuncData)
            : makesSenseForStatuses(makesSenseForStatuses),
              dependsOnFeatures(dependsOnFeatures),
              dependsOnInterfaces(dependsOnInterfaces),
              introspectFunc(introspectFunc),
              introspectFuncData(introspectFuncData)
        {
        }

    private:
        friend class ReadinessHelper;

        QSet<uint> makesSenseForStatuses;
        QSet<uint> dependsOnFeatures;
        QStringList dependsOnInterfaces;
        IntrospectFunc introspectFunc;
        void *introspectFuncData;
    };

    ReadinessHelper(DBusProxy *proxy,
            uint currentStatus,
            const QMap<uint, Introspectable> &introspectables,
            QObject *parent = 0);
    ~ReadinessHelper();

    uint currentStatus() const;
    void setCurrentStatus(uint currentStatus);

    QStringList interfaces() const;
    void setInterfaces(const QStringList &interfaces);

    QSet<uint> requestedFeatures() const;
    QSet<uint> actualFeatures() const;
    QSet<uint> missingFeatures() const;

    bool isReady(QSet<uint> features) const;
    PendingReady *becomeReady(QSet<uint> requestedFeatures);

    void setIntrospectCompleted(uint feature, bool success);

Q_SIGNALS:
    void statusReady(uint status);

private Q_SLOTS:
    void iterateIntrospection();

    void onProxyInvalidated(Telepathy::Client::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Telepathy::Client
} // Telepathy

#endif
