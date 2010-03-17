#include <QtTest/QtTest>

#include <TelepathyQt4/KeyFile>

using namespace Tp;

class TestKeyFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testKeyFile();
};

void TestKeyFile::testKeyFile()
{
    QString top_srcdir = QString::fromLocal8Bit(::getenv("abs_top_srcdir"));
    if (!top_srcdir.isEmpty()) {
        QDir::setCurrent(top_srcdir + QLatin1String("/tests"));
    }

    KeyFile defaultKeyFile;
    QCOMPARE(defaultKeyFile.status(), KeyFile::None);

    KeyFile notFoundkeyFile(QLatin1String("test-key-file-not-found.ini"));
    QCOMPARE(notFoundkeyFile.status(), KeyFile::NotFoundError);

    KeyFile formatErrorkeyFile(QLatin1String("test-key-file-format-error.ini"));
    QCOMPARE(formatErrorkeyFile.status(), KeyFile::FormatError);

    KeyFile keyFile(QLatin1String("test-key-file.ini"));
    QCOMPARE(keyFile.status(), KeyFile::NoError);

    QCOMPARE(keyFile.allGroups(),
             QStringList() << QString() <<
                              QLatin1String("test group 1") <<
                              QLatin1String("test group 2"));

    QStringList allKeys = keyFile.allKeys();
    allKeys.sort();
    QCOMPARE(allKeys,
             QStringList() <<
             QLatin1String("a") <<
             QLatin1String("b") <<
             QLatin1String("c") <<
             QLatin1String("d") <<
             QLatin1String("e"));

    keyFile.setGroup(QLatin1String("test group 1"));
    QCOMPARE(keyFile.contains(QLatin1String("f")), false);
    QCOMPARE(keyFile.value(QLatin1String("c")).length(), 5);

    keyFile.setGroup(QLatin1String("test group 2"));
    QCOMPARE(keyFile.contains(QLatin1String("e")), true);
    QCOMPARE(keyFile.value(QLatin1String("e")), QString(QLatin1String("space")));
}

QTEST_MAIN(TestKeyFile)

#include "_gen/key-file.cpp.moc.hpp"
