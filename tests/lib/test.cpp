#include "tests/lib/test.h"

#include <cstdlib>

#include <QtCore/QTimer>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/DBus>
#include <TelepathyQt4/PendingVoid>

using Tp::PendingOperation;
using Tp::PendingVoid;
using Tp::Client::DBus::PeerInterface;

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
    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

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

void Test::expectFailure(PendingOperation *op)
{
    if (!op->isError()) {
        qWarning() << "expectFailure(): should have been an error, but wasn't";
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}  

void Test::processDBusQueue(Tp::DBusProxy *proxy)
{
    // Call method Ping on the D-Bus Peer interface
    PeerInterface peer(proxy);
    PendingVoid *call = new PendingVoid(peer.Ping(), this);

    // Wait for the reply to the Ping call
    QVERIFY(connect(call,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
}

#include "_gen/test.h.moc.hpp"
