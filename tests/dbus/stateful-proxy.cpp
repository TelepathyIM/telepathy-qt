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

#include <QEventLoop>
#include <QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/StatefulDBusProxy>

using namespace Telepathy;
using namespace Telepathy::Client;

// expose protected functions for testing
class MyStatefulDBusProxy : public StatefulDBusProxy
{
public:
    MyStatefulDBusProxy(const QDBusConnection &dbusConnection,
            const QString &busName, const QString &objectPath,
            QObject *parent = 0)
        : StatefulDBusProxy(dbusConnection, busName, objectPath, parent)
    {
    }

    using StatefulDBusProxy::invalidate;
};

class TestStatefulProxy : public QObject
{
    Q_OBJECT

public:
    TestStatefulProxy(QObject *parent = 0);
    ~TestStatefulProxy();

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();
    void testNameOwnerChanged();

    void cleanup();
    void cleanupTestCase();

protected Q_SLOTS:
    // these would be public, but then QtTest would think they were tests
    void expectInvalidated(Telepathy::Client::DBusProxy *,
            QString, QString);

private:
    QEventLoop *mLoop;
    MyStatefulDBusProxy *mProxy;

    int mInvalidated;
    QString mSignalledInvalidationReason;
    QString mSignalledInvalidationMessage;

    static QString wellKnownName();
    static QString objectPath();
    static QString uniqueName();
};

QString TestStatefulProxy::wellKnownName()
{
    return "org.freedesktop.Telepathy.Qt4.TestStatefulProxy";
}

QString TestStatefulProxy::objectPath()
{
    return "/org/freedesktop/Telepathy/Qt4/TestStatefulProxy/Object";
}

TestStatefulProxy::TestStatefulProxy(QObject *parent)
    : QObject(parent),
      mLoop(new QEventLoop(this)),
      mProxy(0)
{
}

TestStatefulProxy::~TestStatefulProxy()
{
    delete mLoop;
}

QString TestStatefulProxy::uniqueName()
{
    return QDBusConnection::sessionBus().baseService();
}

void TestStatefulProxy::initTestCase()
{
    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    QVERIFY(QDBusConnection::sessionBus().isConnected());
    QVERIFY(QDBusConnection::sessionBus().registerService(wellKnownName()));
}

void TestStatefulProxy::init()
{
    mInvalidated = 0;
}

void TestStatefulProxy::testBasics()
{
    mProxy = new MyStatefulDBusProxy(QDBusConnection::sessionBus(),
            wellKnownName(), objectPath());

    QVERIFY(mProxy);
    QCOMPARE(mProxy->dbusConnection().baseService(), uniqueName());
    QCOMPARE(mProxy->busName(), uniqueName());
    QCOMPARE(mProxy->objectPath(), objectPath());

    QVERIFY(mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(), QString());
    QCOMPARE(mProxy->invalidationMessage(), QString());


    QVERIFY(connect(mProxy, SIGNAL(invalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString)),
                this, SLOT(expectInvalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString))));
    mProxy->invalidate("com.example.DomainSpecificError",
            "Because I said so");

    QVERIFY(!mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(),
            QString::fromAscii("com.example.DomainSpecificError"));
    QCOMPARE(mProxy->invalidationMessage(),
            QString::fromAscii("Because I said so"));

    // the signal doesn't arrive instantly
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mProxy, SIGNAL(invalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString)),
                this, SLOT(expectInvalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString))));

    QCOMPARE(mInvalidated, 1);
    QVERIFY(!mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(),
            QString::fromAscii("com.example.DomainSpecificError"));
    QCOMPARE(mSignalledInvalidationReason,
            QString::fromAscii("com.example.DomainSpecificError"));
    QCOMPARE(mProxy->invalidationMessage(),
            QString::fromAscii("Because I said so"));
    QCOMPARE(mSignalledInvalidationMessage,
            QString::fromAscii("Because I said so"));
}

void TestStatefulProxy::expectInvalidated(DBusProxy *proxy,
        QString reason, QString message)
{
    mInvalidated++;
    mSignalledInvalidationReason = reason;
    mSignalledInvalidationMessage = message;
    mLoop->exit(0);
}

void TestStatefulProxy::testNameOwnerChanged()
{
    QString otherUniqueName = QDBusConnection::connectToBus(
            QDBusConnection::SessionBus,
            "another unique name").baseService();

    mProxy = new MyStatefulDBusProxy(QDBusConnection::sessionBus(),
            otherUniqueName, objectPath());

    QVERIFY(mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(), QString());
    QCOMPARE(mProxy->invalidationMessage(), QString());

    QVERIFY(connect(mProxy, SIGNAL(invalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString)),
                this, SLOT(expectInvalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString))));
    QDBusConnection::disconnectFromBus("another unique name");
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(disconnect(mProxy, SIGNAL(invalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString)),
                this, SLOT(expectInvalidated(
                        Telepathy::Client::DBusProxy *,
                        QString, QString))));

    QCOMPARE(mInvalidated, 1);
    QVERIFY(!mProxy->isValid());
    QCOMPARE(mProxy->invalidationReason(),
            QString::fromAscii(TELEPATHY_DBUS_ERROR_NAME_HAS_NO_OWNER));
    QCOMPARE(mSignalledInvalidationReason,
            QString::fromAscii(TELEPATHY_DBUS_ERROR_NAME_HAS_NO_OWNER));
    QVERIFY(!mProxy->invalidationMessage().isEmpty());
    QCOMPARE(mProxy->invalidationMessage(), mSignalledInvalidationMessage);
}

void TestStatefulProxy::cleanup()
{
    if (mProxy) {
        delete mProxy;
        mProxy = 0;
    }
}

void TestStatefulProxy::cleanupTestCase()
{
}

QTEST_MAIN(TestStatefulProxy)

#include "_gen/stateful-proxy.cpp.moc.hpp"
