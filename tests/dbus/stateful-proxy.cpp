/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#include <QEventLoop>
#include <QtTest>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>
#include <TelepathyQt/DBus>
#include <TelepathyQt/StatefulDBusProxy>

#include "tests/lib/test.h"

using namespace Tp;
using namespace Tp;
using Tp::Client::DBus::IntrospectableInterface;

// expose protected functions for testing
class MyStatefulDBusProxy : public StatefulDBusProxy
{
public:
    MyStatefulDBusProxy(const QDBusConnection &dbusConnection,
            const QString &busName, const QString &objectPath)
        : StatefulDBusProxy(dbusConnection, busName, objectPath, Feature())
    {
    }

    using StatefulDBusProxy::invalidate;
};

class ObjectAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.example.Foo")

public:
    ObjectAdaptor(Test *test)
        : QDBusAbstractAdaptor(test)
    {
    }
};

class TestStatefulProxy : public Test
{
    Q_OBJECT

public:
    TestStatefulProxy(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();
    void testNameOwnerChanged();

    void cleanup();
    void cleanupTestCase();

protected Q_SLOTS:
    // these would be public, but then QtTest would think they were tests
    void expectInvalidated(Tp::DBusProxy *,
            const QString &, const QString &);

    // anything other than 0 or 1 is OK
#   define EXPECT_INVALIDATED_SUCCESS 111

private:
    MyStatefulDBusProxy *mProxy;
    ObjectAdaptor *mAdaptor;

    int mInvalidated;
    QString mSignalledInvalidationReason;
    QString mSignalledInvalidationMessage;

    static QString wellKnownName();
    static QString objectPath();
    static QString uniqueName();
};

QString TestStatefulProxy::wellKnownName()
{
    return QLatin1String("org.freedesktop.Telepathy.Qt.TestStatefulProxy");
}

QString TestStatefulProxy::objectPath()
{
    return QLatin1String("/org/freedesktop/Telepathy/Qt/TestStatefulProxy/Object");
}

TestStatefulProxy::TestStatefulProxy(QObject *parent)
    : Test(parent),
      mProxy(0),
      mAdaptor(new ObjectAdaptor(this))
{
}

QString TestStatefulProxy::uniqueName()
{
    return QDBusConnection::sessionBus().baseService();
}

void TestStatefulProxy::initTestCase()
{
    initTestCaseImpl();

    QVERIFY(QDBusConnection::sessionBus().registerService(wellKnownName()));
    QDBusConnection::sessionBus().registerObject(objectPath(), this);
}

void TestStatefulProxy::init()
{
    initImpl();

    mInvalidated = 0;
}

void TestStatefulProxy::testBasics()
{
    mProxy = new MyStatefulDBusProxy(QDBusConnection::sessionBus(),
            wellKnownName(), objectPath());
    IntrospectableInterface ifaceFromProxy(mProxy);
    IntrospectableInterface ifaceFromWellKnown(wellKnownName(), objectPath());
    IntrospectableInterface ifaceFromUnique(uniqueName(), objectPath());

    QVERIFY(mProxy);
    QCOMPARE(mProxy->dbusConnection().baseService(), uniqueName());
    QCOMPARE(mProxy->busName(), uniqueName());
    QCOMPARE(mProxy->objectPath(), objectPath());

    QVERIFY(mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(), QString());
    QCOMPARE(mProxy->invalidationMessage(), QString());

    QDBusPendingReply<QString> reply;
    QDBusPendingCallWatcher *watcher;

    reply = ifaceFromUnique.Introspect();
    if (!reply.isValid()) {
        watcher = new QDBusPendingCallWatcher(reply);
        QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
        QCOMPARE(mLoop->exec(), 0);
        delete watcher;
    }

    reply = ifaceFromWellKnown.Introspect();
    if (!reply.isValid()) {
        watcher = new QDBusPendingCallWatcher(reply);
        QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
        QCOMPARE(mLoop->exec(), 0);
        delete watcher;
    }

    reply = ifaceFromProxy.Introspect();
    if (!reply.isValid()) {
        watcher = new QDBusPendingCallWatcher(reply);
        QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
        QCOMPARE(mLoop->exec(), 0);
        delete watcher;
    }

    QVERIFY(connect(mProxy, SIGNAL(invalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &)),
                this, SLOT(expectInvalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &))));
    mProxy->invalidate(QLatin1String("com.example.DomainSpecificError"),
            QLatin1String("Because I said so"));

    QVERIFY(!mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(),
            QLatin1String("com.example.DomainSpecificError"));
    QCOMPARE(mProxy->invalidationMessage(),
            QLatin1String("Because I said so"));

    // FIXME: ideally, the method call would already fail synchronously at
    // this point - after all, the proxy already knows it's dead
    reply = ifaceFromProxy.Introspect();
    if (reply.isValid()) {
        qWarning() << "reply is valid";
    } else if (reply.isError()) {
        qDebug() << "reply is error" << reply.error().name()
            << reply.error().message();
    } else {
        qWarning() << "no reply yet";
    }

    // the signal doesn't arrive instantly
    QCOMPARE(mLoop->exec(), EXPECT_INVALIDATED_SUCCESS);
    QVERIFY(disconnect(mProxy, SIGNAL(invalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &)),
                this, SLOT(expectInvalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &))));

    QCOMPARE(mInvalidated, 1);
    QCOMPARE(mSignalledInvalidationReason,
            QLatin1String("com.example.DomainSpecificError"));
    QCOMPARE(mSignalledInvalidationMessage,
            QLatin1String("Because I said so"));

    // low-level proxies with no knowledge of the high-level DBusProxy are
    // unaffected

    reply = ifaceFromUnique.Introspect();
    if (!reply.isValid()) {
        watcher = new QDBusPendingCallWatcher(reply);
        QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
        QCOMPARE(mLoop->exec(), 0);
        delete watcher;
    }

    reply = ifaceFromWellKnown.Introspect();
    if (!reply.isValid()) {
        watcher = new QDBusPendingCallWatcher(reply);
        QVERIFY(connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                this, SLOT(expectSuccessfulCall(QDBusPendingCallWatcher *))));
        QCOMPARE(mLoop->exec(), 0);
        delete watcher;
    }
}

void TestStatefulProxy::expectInvalidated(DBusProxy *proxy,
        const QString &reason, const QString &message)
{
    mInvalidated++;
    mSignalledInvalidationReason = reason;
    mSignalledInvalidationMessage = message;
    mLoop->exit(EXPECT_INVALIDATED_SUCCESS);
}

void TestStatefulProxy::testNameOwnerChanged()
{
    QString otherUniqueName = QDBusConnection::connectToBus(
            QDBusConnection::SessionBus,
            QLatin1String("another unique name")).baseService();

    mProxy = new MyStatefulDBusProxy(QDBusConnection::sessionBus(),
            otherUniqueName, objectPath());

    QVERIFY(mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(), QString());
    QCOMPARE(mProxy->invalidationMessage(), QString());

    QVERIFY(connect(mProxy, SIGNAL(invalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &)),
                this, SLOT(expectInvalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &))));
    QDBusConnection::disconnectFromBus(QLatin1String("another unique name"));
    QCOMPARE(mLoop->exec(), EXPECT_INVALIDATED_SUCCESS);
    QVERIFY(disconnect(mProxy, SIGNAL(invalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &)),
                this, SLOT(expectInvalidated(
                        Tp::DBusProxy *,
                        const QString &, const QString &))));

    QCOMPARE(mInvalidated, 1);
    QVERIFY(!mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(),
            TP_QT_DBUS_ERROR_NAME_HAS_NO_OWNER);
    QCOMPARE(mSignalledInvalidationReason,
            TP_QT_DBUS_ERROR_NAME_HAS_NO_OWNER);
    QVERIFY(!mProxy->invalidationMessage().isEmpty());
    QCOMPARE(mProxy->invalidationMessage(), mSignalledInvalidationMessage);
}

void TestStatefulProxy::cleanup()
{
    if (mProxy) {
        delete mProxy;
        mProxy = 0;
    }

    cleanupImpl();
}

void TestStatefulProxy::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestStatefulProxy)

#include "_gen/stateful-proxy.cpp.moc.hpp"
