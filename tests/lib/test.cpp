#include "tests/lib/test.h"

#include <cstdlib>

#include <QtCore/QTimer>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Client/DBus>

using Telepathy::Client::PendingOperation;

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

#include "_gen/test.h.moc.hpp"
