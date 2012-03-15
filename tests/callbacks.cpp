#include <QtTest/QtTest>

#include <TelepathyQt/Callbacks>

using namespace Tp;

class TestCallbacks : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testMemFun();
    void testPtrFun();
};

struct MyCallbacks
{
    MyCallbacks()
    {
        reset();
    }

    void testVV() { mVVCalled++; }
    void testVI1(int a1)
    {
        QCOMPARE(a1, 1);
        mVI1Called++;
    }
    void testVI2(int a1, int a2)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2);
        mVI2Called++;
    }
    void testVI3(int a1, int a2, int a3)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3);
        mVI3Called++;
    }
    void testVI4(int a1, int a2, int a3, int a4)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4);
        mVI4Called++;
    }
    void testVI5(int a1, int a2, int a3, int a4, int a5)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5);
        mVI5Called++;
    }
    void testVI6(int a1, int a2, int a3, int a4, int a5, int a6)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5); QCOMPARE(a6, 6);
        mVI6Called++;
    }
    void testVI7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5); QCOMPARE(a6, 6); QCOMPARE(a7, 7);
        mVI7Called++;
    }

    void reset()
    {
        mVVCalled = false;
        mVI1Called = false;
        mVI2Called = false;
        mVI3Called = false;
        mVI4Called = false;
        mVI5Called = false;
        mVI6Called = false;
        mVI7Called = false;
    }

    void verifyCalled(bool vv, bool vi1, bool vi2, bool vi3,
                      bool vi4, bool vi5, bool vi6, bool vi7)
    {
        QCOMPARE(mVVCalled, vv);
        QCOMPARE(mVI1Called, vi1);
        QCOMPARE(mVI2Called, vi2);
        QCOMPARE(mVI3Called, vi3);
        QCOMPARE(mVI4Called, vi4);
        QCOMPARE(mVI5Called, vi5);
        QCOMPARE(mVI6Called, vi6);
        QCOMPARE(mVI7Called, vi7);
    }

    bool mVVCalled;
    bool mVI1Called;
    bool mVI2Called;
    bool mVI3Called;
    bool mVI4Called;
    bool mVI5Called;
    bool mVI6Called;
    bool mVI7Called;
};

namespace
{
    bool mVVCalled;
    bool mVI1Called;
    bool mVI2Called;
    bool mVI3Called;
    bool mVI4Called;
    bool mVI5Called;
    bool mVI6Called;
    bool mVI7Called;

    void testVV() { mVVCalled++; }
    void testVI1(int a1)
    {
        QCOMPARE(a1, 1);
        mVI1Called++;
    }
    void testVI2(int a1, int a2)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2);
        mVI2Called++;
    }
    void testVI3(int a1, int a2, int a3)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3);
        mVI3Called++;
    }
    void testVI4(int a1, int a2, int a3, int a4)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4);
        mVI4Called++;
    }
    void testVI5(int a1, int a2, int a3, int a4, int a5)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5);
        mVI5Called++;
    }
    void testVI6(int a1, int a2, int a3, int a4, int a5, int a6)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5); QCOMPARE(a6, 6);
        mVI6Called++;
    }
    void testVI7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
    {
        QCOMPARE(a1, 1); QCOMPARE(a2, 2); QCOMPARE(a3, 3); QCOMPARE(a4, 4); QCOMPARE(a5, 5); QCOMPARE(a6, 6); QCOMPARE(a7, 7);
        mVI7Called++;
    }

    void reset()
    {
        mVVCalled = false;
        mVI1Called = false;
        mVI2Called = false;
        mVI3Called = false;
        mVI4Called = false;
        mVI5Called = false;
        mVI6Called = false;
        mVI7Called = false;
    }

    void verifyCalled(bool vv, bool vi1, bool vi2, bool vi3,
                      bool vi4, bool vi5, bool vi6, bool vi7)
    {
        QCOMPARE(mVVCalled, vv);
        QCOMPARE(mVI1Called, vi1);
        QCOMPARE(mVI2Called, vi2);
        QCOMPARE(mVI3Called, vi3);
        QCOMPARE(mVI4Called, vi4);
        QCOMPARE(mVI5Called, vi5);
        QCOMPARE(mVI6Called, vi6);
        QCOMPARE(mVI7Called, vi7);
    }
}

void TestCallbacks::testMemFun()
{
    MyCallbacks cbs;

    Callback0<void> cbVV = memFun(&cbs, &MyCallbacks::testVV);
    cbVV();
    cbs.verifyCalled(true, false, false, false, false, false, false, false);
    cbs.reset();

    Callback1<void, int> cbVI1 = memFun(&cbs, &MyCallbacks::testVI1);
    cbVI1(1);
    cbs.verifyCalled(false, true, false, false, false, false, false, false);
    cbs.reset();

    Callback2<void, int, int> cbVI2 = memFun(&cbs, &MyCallbacks::testVI2);
    cbVI2(1, 2);
    cbs.verifyCalled(false, false, true, false, false, false, false, false);
    cbs.reset();

    Callback3<void, int, int, int> cbVI3 = memFun(&cbs, &MyCallbacks::testVI3);
    cbVI3(1, 2, 3);
    cbs.verifyCalled(false, false, false, true, false, false, false, false);
    cbs.reset();

    Callback4<void, int, int, int, int> cbVI4 = memFun(&cbs, &MyCallbacks::testVI4);
    cbVI4(1, 2, 3, 4);
    cbs.verifyCalled(false, false, false, false, true, false, false, false);
    cbs.reset();

    Callback5<void, int, int, int, int, int> cbVI5 = memFun(&cbs, &MyCallbacks::testVI5);
    cbVI5(1, 2, 3, 4, 5);
    cbs.verifyCalled(false, false, false, false, false, true, false, false);
    cbs.reset();

    Callback6<void, int, int, int, int, int, int> cbVI6 = memFun(&cbs, &MyCallbacks::testVI6);
    cbVI6(1, 2, 3, 4, 5, 6);
    cbs.verifyCalled(false, false, false, false, false, false, true, false);
    cbs.reset();

    Callback7<void, int, int, int, int, int, int, int> cbVI7 = memFun(&cbs, &MyCallbacks::testVI7);
    cbVI7(1, 2, 3, 4, 5, 6, 7);
    cbs.verifyCalled(false, false, false, false, false, false, false, true);
    cbs.reset();
}

void TestCallbacks::testPtrFun()
{
    reset();

    Callback0<void> cbVV = ptrFun(&testVV);
    cbVV();
    verifyCalled(true, false, false, false, false, false, false, false);
    reset();

    Callback1<void, int> cbVI1 = ptrFun(&testVI1);
    cbVI1(1);
    verifyCalled(false, true, false, false, false, false, false, false);
    reset();

    Callback2<void, int, int> cbVI2 = ptrFun(&testVI2);
    cbVI2(1, 2);
    verifyCalled(false, false, true, false, false, false, false, false);
    reset();

    Callback3<void, int, int, int> cbVI3 = ptrFun(&testVI3);
    cbVI3(1, 2, 3);
    verifyCalled(false, false, false, true, false, false, false, false);
    reset();

    Callback4<void, int, int, int, int> cbVI4 = ptrFun(&testVI4);
    cbVI4(1, 2, 3, 4);
    verifyCalled(false, false, false, false, true, false, false, false);
    reset();

    Callback5<void, int, int, int, int, int> cbVI5 = ptrFun(&testVI5);
    cbVI5(1, 2, 3, 4, 5);
    verifyCalled(false, false, false, false, false, true, false, false);
    reset();

    Callback6<void, int, int, int, int, int, int> cbVI6 = ptrFun(&testVI6);
    cbVI6(1, 2, 3, 4, 5, 6);
    verifyCalled(false, false, false, false, false, false, true, false);
    reset();

    Callback7<void, int, int, int, int, int, int, int> cbVI7 = ptrFun(&testVI7);
    cbVI7(1, 2, 3, 4, 5, 6, 7);
    verifyCalled(false, false, false, false, false, false, false, true);
    reset();
}

QTEST_MAIN(TestCallbacks)

#include "_gen/callbacks.cpp.moc.hpp"
