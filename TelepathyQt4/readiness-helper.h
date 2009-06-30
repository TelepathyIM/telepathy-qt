/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/Feature>

#include <QMap>
#include <QSet>
#include <QSharedDataPointer>
#include <QStringList>

class QDBusError;

namespace Tp
{

class DBusProxy;
class PendingOperation;
class PendingReady;

class ReadinessHelper : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ReadinessHelper)

public:
    typedef void (*IntrospectFunc)(void *data);

    struct Introspectable {
    public:
        Introspectable();
        Introspectable(const QSet<uint> &makesSenseForStatuses,
                const Features &dependsOnFeatures,
                const QStringList &dependsOnInterfaces,
                IntrospectFunc introspectFunc,
                void *introspectFuncData,
                bool critical = false);
        Introspectable(const Introspectable &other);
        ~Introspectable();

        Introspectable &operator=(const Introspectable &other);

    private:
        friend class ReadinessHelper;

        struct Private;
        friend struct Private;
        QSharedDataPointer<Private> mPriv;
    };
    typedef QMap<Feature, Introspectable> Introspectables;

    ReadinessHelper(QObject *object,
            uint currentStatus = 0,
            const Introspectables &introspectables = Introspectables(),
            QObject *parent = 0);
    ~ReadinessHelper();

    void addIntrospectables(const Introspectables &introspectables);

    uint currentStatus() const;
    void setCurrentStatus(uint currentStatus);

    QStringList interfaces() const;
    void setInterfaces(const QStringList &interfaces);

    Features requestedFeatures() const;
    Features actualFeatures() const;
    Features missingFeatures() const;

    bool isReady(const Feature &feature,
            QString *errorName = 0, QString *errorMessage = 0) const;
    bool isReady(const Features &features,
            QString *errorName = 0, QString *errorMessage = 0) const;
    PendingReady *becomeReady(const Features &requestedFeatures);

    void setIntrospectCompleted(const Feature &feature, bool success,
            const QString &errorName = QString(),
            const QString &errorMessage = QString());
    void setIntrospectCompleted(const Feature &feature, bool success,
            const QDBusError &error);

Q_SIGNALS:
    void statusReady(uint status);

private Q_SLOTS:
    void iterateIntrospection();

    void onProxyInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage);
    void onOperationFinished(Tp::PendingOperation *op);
    void onOperationDestroyed(QObject *obj);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
