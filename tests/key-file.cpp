#include <QtTest/QtTest>

#include "TelepathyQt/key-file.h"

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

    QStringList allGroups = keyFile.allGroups();
    allGroups.sort();
    QCOMPARE(allGroups,
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

    keyFile.setFileName(QLatin1String("telepathy/managers/test-manager-file.manager"));
    QCOMPARE(keyFile.status(), KeyFile::NoError);

    keyFile.setGroup(QLatin1String("Protocol somewhat-pathological"));
    QCOMPARE(keyFile.value(QLatin1String("param-foo")), QString(QLatin1String("s required")));
    QCOMPARE(keyFile.value(QLatin1String("default-foo")), QString(QLatin1String("hello world")));

    QCOMPARE(keyFile.value(QLatin1String("param-semicolons")), QString(QLatin1String("s secret")));
    QCOMPARE(keyFile.value(QLatin1String("default-semicolons")), QString(QLatin1String("list;of;misc;")));

    QCOMPARE(keyFile.value(QLatin1String("param-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-list")), QString(QLatin1String("list;of;misc;")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-list")),
             QStringList() << QLatin1String("list") << QLatin1String("of") << QLatin1String("misc"));

    QCOMPARE(keyFile.value(QLatin1String("param-unterminated-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-unterminated-list")), QString(QLatin1String("list;of;misc")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-unterminated-list")),
             QStringList() << QLatin1String("list") << QLatin1String("of") << QLatin1String("misc"));

    QCOMPARE(keyFile.value(QLatin1String("param-spaces-in-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-spaces-in-list")), QString(QLatin1String("list; of; misc ;")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-spaces-in-list")),
             QStringList() << QLatin1String("list") << QLatin1String(" of") << QLatin1String(" misc "));

    QCOMPARE(keyFile.value(QLatin1String("param-escaped-semicolon-in-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-escaped-semicolon-in-list")), QString(QLatin1String("list;of;misc;")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-escaped-semicolon-in-list")),
             QStringList() << QLatin1String("list;of") << QLatin1String("misc"));

    QCOMPARE(keyFile.value(QLatin1String("param-doubly-escaped-semicolon-in-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-doubly-escaped-semicolon-in-list")), QString(QLatin1String("list\\;of;misc;")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-doubly-escaped-semicolon-in-list")),
             QStringList() << QLatin1String("list\\") << QLatin1String("of") << QLatin1String("misc"));

    QCOMPARE(keyFile.value(QLatin1String("param-triply-escaped-semicolon-in-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-triply-escaped-semicolon-in-list")), QString(QLatin1String("list\\;of;misc;")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-triply-escaped-semicolon-in-list")),
             QStringList() << QLatin1String("list\\;of") << QLatin1String("misc"));

    QCOMPARE(keyFile.value(QLatin1String("param-empty-list")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-empty-list")), QString(QLatin1String("")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-empty-list")), QStringList());

    QCOMPARE(keyFile.value(QLatin1String("param-list-of-empty-string")), QString(QLatin1String("as")));
    QCOMPARE(keyFile.value(QLatin1String("default-list-of-empty-string")), QString(QLatin1String(";")));
    QCOMPARE(keyFile.valueAsStringList(QLatin1String("default-list-of-empty-string")), QStringList() << QString());

    QCOMPARE(keyFile.value(QLatin1String("param-escaped-semicolon")), QString(QLatin1String("s")));
    QCOMPARE(keyFile.value(QLatin1String("default-escaped-semicolon")), QString(QLatin1String("foo;bar")));
}

QTEST_MAIN(TestKeyFile)

#include "_gen/key-file.cpp.moc.hpp"
