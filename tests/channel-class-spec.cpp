#include <QtTest/QtTest>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/Types>

using namespace Tp;

namespace {

ChannelClassSpecList reverse(const ChannelClassSpecList &list)
{
    ChannelClassSpecList ret(list);
    for (int k = 0; k < (list.size() / 2); k++) {
        ret.swap(k, list.size() - (1 + k));
    }
    return ret;
}

};

class TestChannelClassSpec : public QObject
{
    Q_OBJECT

public:
    TestChannelClassSpec(QObject *parent = 0);

private Q_SLOTS:
    void testChannelClassSpecHash();
    void testServiceLeaks();
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

    sl1.clear();
    sl2.clear();

    for (int i = 0; i < 100; ++i) {
        sl1 << ChannelClassSpec::textChat() <<
            ChannelClassSpec::streamedMediaCall() <<
            ChannelClassSpec::unnamedTextChat();
    }

    ChannelClassSpec specs[3] = {
            ChannelClassSpec::textChat(),
            ChannelClassSpec::streamedMediaCall(),
            ChannelClassSpec::unnamedTextChat()
    };
    for (int i = 0; i < 3; ++i) {
        ChannelClassSpec spec = specs[i];
        for (int j = 0; j < 100; ++j) {
            sl2 << spec;
        }
    }

    QCOMPARE(qHash(sl1), qHash(ChannelClassSpecList() <<
                ChannelClassSpec::unnamedTextChat() <<
                ChannelClassSpec::streamedMediaCall() <<
                ChannelClassSpec::textChat()));

    for (int i = 0; i < 1000; ++i) {
        ChannelClassSpec spec = ChannelClassSpec::outgoingStreamTube(QString::number(i));
        sl1 << spec;
        sl2.prepend(spec);
    }

    QCOMPARE(qHash(sl1), qHash(sl2));

    sl1 = reverse(sl1);
    sl2 = reverse(sl2);
    QCOMPARE(qHash(sl1), qHash(sl2));

    sl2 << ChannelClassSpec::outgoingFileTransfer();
    QVERIFY(qHash(sl1) != qHash(sl2));
}

void TestChannelClassSpec::testServiceLeaks()
{
    ChannelClassSpec bareTube = ChannelClassSpec::outgoingStreamTube();
    QVERIFY(!bareTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));

    ChannelClassSpec ftpTube = ChannelClassSpec::outgoingStreamTube(QLatin1String("ftp"));
    QVERIFY(ftpTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));
    QCOMPARE(ftpTube.allProperties().value(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")).toString(), QString::fromLatin1("ftp"));
    QVERIFY(!bareTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));

    ChannelClassSpec httpTube = ChannelClassSpec::outgoingStreamTube(QLatin1String("http"));
    QVERIFY(httpTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));
    QVERIFY(ftpTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));
    QCOMPARE(httpTube.allProperties().value(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")).toString(), QString::fromLatin1("http"));
    QCOMPARE(ftpTube.allProperties().value(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")).toString(), QString::fromLatin1("ftp"));
    QVERIFY(!bareTube.allProperties().contains(TP_QT_IFACE_CHANNEL_TYPE_STREAM_TUBE +
                QString::fromLatin1(".Service")));
}

QTEST_MAIN(TestChannelClassSpec)

#include "_gen/channel-class-spec.cpp.moc.hpp"
