#include <QtTest/QtTest>

#include <TelepathyQt/Utils>

using namespace Tp;

class TestUtils : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testUtils();
};

void TestUtils::testUtils()
{
  QString res;

  res = escapeAsIdentifier(QString::fromLatin1(""));
  QCOMPARE(res, QString::fromLatin1("_"));

  res = escapeAsIdentifier(QString::fromLatin1("badger"));
  QCOMPARE(res, QString::fromLatin1("badger"));

  res = escapeAsIdentifier(QString::fromLatin1("0123abc_xyz"));
  QCOMPARE(res, QString::fromLatin1("_30123abc_5fxyz"));

  res = escapeAsIdentifier(QString::fromUtf8("©"));
  QCOMPARE(res, QString::fromLatin1("_c2_a9"));
}

QTEST_MAIN(TestUtils)

#include "_gen/utils.cpp.moc.hpp"
