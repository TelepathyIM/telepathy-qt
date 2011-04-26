#include <QtTest/QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/Types>

using namespace Tp;

class TestChannelClassSpec : public QObject
{
    Q_OBJECT

public:
    TestChannelClassSpec(QObject *parent = 0);

private Q_SLOTS:
    void testChannelClassSpecHash();
};

TestChannelClassSpec::TestChannelClassSpec(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

void TestChannelClassSpec::testChannelClassSpecHash()
{
    ChannelClassSpec st1 = ChannelClassSpec::textChat();
    ChannelClassSpec st2 = ChannelClassSpec::textChat();
    ChannelClassSpec ssm1 = ChannelClassSpec::streamedMediaCall();
    ChannelClassSpec ssm2 = ChannelClassSpec::streamedMediaCall();

    QCOMPARE(qHash(st1), qHash(st2));
    QCOMPARE(qHash(ssm1), qHash(ssm2));
    QVERIFY(qHash(st1) != qHash(ssm1));

    // hash of list with duplicated elements should be the same as hash of the list of same items
    // but with no duplicates
    ChannelClassSpecList sl1;
    sl1 << st1 << st2;
    ChannelClassSpecList sl2;
    sl2 << st1;
    QCOMPARE(qHash(sl1), qHash(sl2));

    // hash of list with same elements but different order should be the same
    sl1.clear();
    sl2.clear();
    sl1 << st1 << ssm1;
    sl2 << ssm1 << st1;
    QCOMPARE(qHash(sl1), qHash(sl2));

    // still the same but with duplicated elements
    sl2 << ssm2 << st2;
    QCOMPARE(qHash(sl1), qHash(sl2));
    sl1 << st2;
    QCOMPARE(qHash(sl1), qHash(sl2));

    // now sl2 is different from sl1, hash should be different
    sl2 << ChannelClassSpec::unnamedTextChat();
    QVERIFY(qHash(sl1) != qHash(sl2));

    // same again
    sl1.prepend(ChannelClassSpec::unnamedTextChat());
    QCOMPARE(qHash(sl1), qHash(sl2));
}

QTEST_MAIN(TestChannelClassSpec)

#include "_gen/channel-class-spec.cpp.moc.hpp"
