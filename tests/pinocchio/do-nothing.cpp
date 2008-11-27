#include <cstdlib>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

class TestDoNothing : public QObject
{
    Q_OBJECT

public:
    TestDoNothing(QObject *parent = 0)
        : QObject(parent), mLoop(new QEventLoop(this))
    { }

private:
    QString mPinocchioPath;
    QString mPinocchioCtlPath;
    QProcess mPinocchio;
    QEventLoop *mLoop;

private slots:
    void initTestCase();
    void init();

    void doNothing();
    void doNothing2();

    void cleanup();
    void cleanupTestCase();
};


void TestDoNothing::initTestCase()
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


void TestDoNothing::init()
{
}


void TestDoNothing::doNothing()
{
    QTimer::singleShot(0, mLoop, SLOT(quit()));
    QCOMPARE(mLoop->exec(), 0);
}


void TestDoNothing::doNothing2()
{
    QTimer::singleShot(0, mLoop, SLOT(quit()));
    QCOMPARE(mLoop->exec(), 0);
}


void TestDoNothing::cleanup()
{
}


void TestDoNothing::cleanupTestCase()
{
    qDebug() << "Terminating Pinocchio";

    mPinocchio.terminate();

    if (!mPinocchio.waitForFinished(1000)) {
        mPinocchio.kill();
    }
}


QTEST_MAIN(TestDoNothing)
#include "_gen/do-nothing.moc.hpp"
