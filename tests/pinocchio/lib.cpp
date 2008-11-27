#include "tests/pinocchio/lib.h"

#include <cstdlib>

void PinocchioTest::initTestCaseImpl()
{
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


void PinocchioTest::cleanupTestCaseImpl()
{
    qDebug() << "Terminating Pinocchio";

    mPinocchio.terminate();

    if (!mPinocchio.waitForFinished(1000)) {
        mPinocchio.kill();
    }
}

#include "_gen/lib.h.moc.hpp"
