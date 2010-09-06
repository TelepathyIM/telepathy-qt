#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <tests/lib/test.h>

class TestDoNothing : public Test
{
    Q_OBJECT

public:
    TestDoNothing(QObject *parent = 0)
        : Test(parent)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void doNothing();
    void doNothing2();

    void cleanup();
    void cleanupTestCase();
};

void TestDoNothing::initTestCase()
{
    initTestCaseImpl();
}

void TestDoNothing::init()
{
    initImpl();
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
    cleanupImpl();
}

void TestDoNothing::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestDoNothing)
#include "_gen/do-nothing.cpp.moc.hpp"
