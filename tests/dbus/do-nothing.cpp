#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
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
  QVERIFY(QDBusConnection::sessionBus().isConnected());
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
}


QTEST_MAIN(TestDoNothing)

#include "_gen/do-nothing.cpp.moc.hpp"
