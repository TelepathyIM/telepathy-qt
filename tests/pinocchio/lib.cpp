#include "tests/pinocchio/lib.h"

#include <cstdlib>

#include <QtCore/QTimer>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Client/DBus>

using namespace Telepathy::Client::DBus;

PinocchioTest::~PinocchioTest()
{
    delete mLoop;
}

void PinocchioTest::initTestCaseImpl()
{
    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    mPinocchioPath = QString::fromLocal8Bit(::getenv("PINOCCHIO"));
    mPinocchioCtlPath = QString::fromLocal8Bit(::getenv("PINOCCHIO_CTL"));

    QVERIFY2(!mPinocchioPath.isEmpty(), "Put $PINOCCHIO in your environment");
    QVERIFY2(!mPinocchioCtlPath.isEmpty(),
        "Put $PINOCCHIO_CTL in your environment");
    QVERIFY(QDBusConnection::sessionBus().isConnected());

    mPinocchio.setProcessChannelMode(QProcess::ForwardedChannels);
    mPinocchio.start(mPinocchioPath, QStringList());

    QVERIFY(mPinocchio.waitForStarted(5000));
    mPinocchio.closeWriteChannel();

    qDebug() << "Started Pinocchio";
}


void PinocchioTest::initImpl()
{
}


void PinocchioTest::cleanupImpl()
{
}


void PinocchioTest::gotNameOwner(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QString> reply = *watcher;

    if (reply.isError()) {
        return;
    }

    if (reply.value() != "") {
        // it has an owner now
        mLoop->exit(1);
    }
}


void PinocchioTest::onNameOwnerChanged(const QString& name,
    const QString& old, const QString& owner)
{
    if (name != pinocchioBusName()) {
        return;
    }

    if (owner != "") {
        // it has an owner now
        mLoop->exit(1);
    }
}


bool PinocchioTest::waitForPinocchio(uint timeoutMs)
{
    QTimer timer;

    connect(&timer, SIGNAL(timeout()), mLoop, SLOT(quit()));
    timer.setSingleShot(true);
    timer.start(timeoutMs);
    QTimer::singleShot(timeoutMs, mLoop, SLOT(quit()));

    DBusDaemonInterface busDaemon("org.freedesktop.DBus", "/org/freedesktop/DBus");
    connect(&busDaemon,
        SIGNAL(NameOwnerChanged(const QString&, const QString&, const QString&)),
        this,
        SLOT(onNameOwnerChanged(const QString&, const QString&, const QString&)));
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
        busDaemon.GetNameOwner(pinocchioBusName()));
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
        this, SLOT(gotNameOwner(QDBusPendingCallWatcher*)));

    bool ret = (mLoop->exec() == 1);

    timer.stop();

    // signals will automatically be disconnected as timer and busDaemon
    // go out of scope

    return ret;
}


void PinocchioTest::cleanupTestCaseImpl()
{
    qDebug() << "Terminating Pinocchio";

    mPinocchio.terminate();

    if (!mPinocchio.waitForFinished(1000)) {
        mPinocchio.kill();
    }
}

#include "_gen/lib.h.moc.hpp"
