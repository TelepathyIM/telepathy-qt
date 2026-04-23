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
#if QT_VERSION > QT_VERSION_CHECK(5, 13, 0)
        ret.swapItemsAt(k, list.size() - (1 + k));
#else
        ret.swap(k, list.size() - (1 + k));
#endif
    }
    return ret;
}

QSet<Feature> makeSet(const QList<Feature> &list)
{
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 0)
    return QSet<Feature>(list.begin(), list.end());
#else
    return list.toSet();
#endif
}

};

class TestFeatures : public QObject
{
    Q_OBJECT

public:
    TestFeatures(QObject *parent = nullptr);

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

    QCOMPARE(qHash(makeSet(fs1)), qHash(makeSet(fs2)));

    fs2.clear();
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 100; ++j) {
            fs2 << Feature(QString::number(j), j);
        }
    }

    QCOMPARE(qHash(makeSet(fs1)), qHash(makeSet(fs2)));

    fs1 = reverse(fs1);
    QCOMPARE(qHash(makeSet(fs1)), qHash(makeSet(fs2)));

    fs2 = reverse(fs2);
    QCOMPARE(qHash(makeSet(fs1)), qHash(makeSet(fs2)));

    fs2 << Feature(QLatin1String("100"), 100);
    QVERIFY(qHash(makeSet(fs1)) != qHash(makeSet(fs2)));
}

QTEST_MAIN(TestFeatures)

#include "_gen/features.cpp.moc.hpp"
