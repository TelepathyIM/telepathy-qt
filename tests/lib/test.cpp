#include "tests/lib/test.h"

#include <cstdlib>

#include <QtCore/QTimer>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/PendingVoidMethodCall>

using Telepathy::PendingOperation;
using Telepathy::PendingVoidMethodCall;
using Telepathy::Client::DBus::PeerInterface;

Test::Test(QObject *parent)
    : QObject(parent), mLoop(new QEventLoop(this))
{
}

Test::~Test()
{
    delete mLoop;
}

void Test::initTestCaseImpl()
{
    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    QVERIFY(QDBusConnection::sessionBus().isConnected());
}

void Test::initImpl()
{
}

void Test::cleanupImpl()
{
}

void Test::cleanupTestCaseImpl()
{
}

void Test::expectSuccessfulCall(PendingOperation *op)
{
    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}

void Test::expectSuccessfulCall(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        qWarning().nospace() << watcher->error().name()
            << ": " << watcher->error().message();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}

void Test::processDBusQueue(Telepathy::DBusProxy *proxy)
{
    // Call method Ping on the D-Bus Peer interface
    PeerInterface peer(proxy);
    PendingVoidMethodCall *call = new PendingVoidMethodCall(this, peer.Ping());

    // Wait for the reply to the Ping call
    QVERIFY(connect(call,
                SIGNAL(finished(Telepathy::PendingOperation*)),
                SLOT(expectSuccessfulCall(Telepathy::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

#include "_gen/test.h.moc.hpp"
