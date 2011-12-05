#include <QtTest/QtTest>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Feature>
#include <TelepathyQt/Types>

using namespace Tp;

namespace {

QList<Feature> reverse(const QList<Feature> &list)
{
    QList<Feature> ret(list);
    for (int k = 0; k < (list.size() / 2); k++) {
        ret.swap(k, list.size() - (1 + k));
    }
    return ret;
}

};

class TestFeatures : public QObject
{
    Q_OBJECT

public:
    TestFeatures(QObject *parent = 0);

private Q_SLOTS:
    void testFeaturesHash();
};

TestFeatures::TestFeatures(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

void TestFeatures::testFeaturesHash()
{
    QList<Feature> fs1;
    QList<Feature> fs2;
    for (int i = 0; i < 100; ++i) {
        fs1 << Feature(QString::number(i), i);
        fs2 << Feature(QString::number(i), i);
    }

    QCOMPARE(qHash(fs1.toSet()), qHash(fs2.toSet()));

    fs2.clear();
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 100; ++j) {
            fs2 << Feature(QString::number(j), j);
        }
    }

    QCOMPARE(qHash(fs1.toSet()), qHash(fs2.toSet()));

    fs1 = reverse(fs1);
    QCOMPARE(qHash(fs1.toSet()), qHash(fs2.toSet()));

    fs2 = reverse(fs2);
    QCOMPARE(qHash(fs1.toSet()), qHash(fs2.toSet()));

    fs2 << Feature(QLatin1String("100"), 100);
    QVERIFY(qHash(fs1.toSet()) != qHash(fs2.toSet()));
}

QTEST_MAIN(TestFeatures)

#include "_gen/features.cpp.moc.hpp"
