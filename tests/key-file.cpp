#include <QtTest/QtTest>

#include <TelepathyQt4/KeyFile>

using namespace Telepathy;

class TestKeyFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testKeyFile();
};

void TestKeyFile::testKeyFile()
{
    KeyFile defaultKeyFile;
    QCOMPARE(defaultKeyFile.status(), KeyFile::None);

    KeyFile notFoundkeyFile("test-key-file-not-found.ini");
    QCOMPARE(notFoundkeyFile.status(), KeyFile::NotFoundError);

    KeyFile formatErrorkeyFile("test-key-file-format-error.ini");
    QCOMPARE(formatErrorkeyFile.status(), KeyFile::FormatError);

    KeyFile keyFile("test-key-file.ini");
    QCOMPARE(keyFile.status(), KeyFile::NoError);

    QCOMPARE(keyFile.allGroups(),
             QStringList() << QString() <<
                              "test group 1" <<
                              "test group 2");

    QStringList allKeys = keyFile.allKeys();
    allKeys.sort();
    QCOMPARE(allKeys,
             QStringList() << "a" << "b" << "c" << "d" << "e");

    keyFile.setGroup("test group 1");
    QCOMPARE(keyFile.contains("f"), false);
    QCOMPARE(keyFile.value("c").length(), 5);

    keyFile.setGroup("test group 2");
    QCOMPARE(keyFile.contains("e"), true);
    QCOMPARE(keyFile.value("e"), QString("space"));
}

QTEST_MAIN(TestKeyFile)
#include "key-file.moc.hpp"
