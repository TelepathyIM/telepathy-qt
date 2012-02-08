#include <QtTest/QtTest>

#include <TelepathyQt/FileTransferChannelCreationProperties>

using namespace Tp;

class TestFileTransferCreationProperties : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFileTransferCreationPropertiesDefaultConstructor();
    void testFileTransferCreationPropertiesDefaultByMandatoryProperties();
    void testFileTransferCreationPropertiesDefaultByPath();
    void testFileTransferCreationPropertiesDefaultByPathFail();
};

void TestFileTransferCreationProperties::testFileTransferCreationPropertiesDefaultConstructor()
{
    FileTransferChannelCreationProperties ftprops;
    QVERIFY(!ftprops.isValid());
    QVERIFY(ftprops.suggestedFileName().isEmpty());
    QVERIFY(ftprops.contentType().isEmpty());
    QCOMPARE(ftprops.size(), (qulonglong)0);

    QVERIFY(!ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeNone);
    QVERIFY(ftprops.contentHash().isEmpty());
    ftprops.setContentHash(FileHashTypeMD5, QLatin1String("ffffffffffffffff"));
    QVERIFY(!ftprops.isValid());
    QVERIFY(!ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeNone);
    QVERIFY(ftprops.contentHash().isEmpty());

    QVERIFY(!ftprops.hasDescription());
    QVERIFY(ftprops.description().isEmpty());
    ftprops.setDescription(QLatin1String("description"));
    QVERIFY(!ftprops.isValid());
    QVERIFY(!ftprops.hasDescription());
    QVERIFY(ftprops.description().isEmpty());

    QVERIFY(!ftprops.hasLastModificationTime());
    QVERIFY(!ftprops.lastModificationTime().isValid());
    ftprops.setLastModificationTime(QDateTime::currentDateTime());
    QVERIFY(!ftprops.isValid());
    QVERIFY(!ftprops.hasLastModificationTime());
    QVERIFY(!ftprops.lastModificationTime().isValid());

    QVERIFY(!ftprops.hasUri());
    QVERIFY(ftprops.uri().isEmpty());
    ftprops.setUri(QLatin1String("file:///path/filename"));
    QVERIFY(!ftprops.isValid());
    QVERIFY(!ftprops.hasUri());
    QVERIFY(ftprops.uri().isEmpty());
}

void TestFileTransferCreationProperties::testFileTransferCreationPropertiesDefaultByMandatoryProperties()
{
    FileTransferChannelCreationProperties ftprops(QLatin1String("suggestedFileName"),
            QLatin1String("application/octet-stream"), (qulonglong)10000);
    QCOMPARE(ftprops.isValid(), true);
    QCOMPARE(ftprops.suggestedFileName(), QLatin1String("suggestedFileName"));
    QCOMPARE(ftprops.contentType(), QLatin1String("application/octet-stream"));
    QCOMPARE(ftprops.size(), (qulonglong)10000);

    QVERIFY(!ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeNone);
    QVERIFY(ftprops.contentHash().isEmpty());
    ftprops.setContentHash(FileHashTypeMD5, QLatin1String("ffffffffffffffff"));
    QVERIFY(ftprops.isValid());
    QVERIFY(ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeMD5);
    QCOMPARE(ftprops.contentHash(), QLatin1String("ffffffffffffffff"));

    QVERIFY(!ftprops.hasDescription());
    QVERIFY(ftprops.description().isEmpty());
    ftprops.setDescription(QLatin1String("description"));
    QVERIFY(ftprops.isValid());
    QVERIFY(ftprops.hasDescription());
    QCOMPARE(ftprops.description(), QLatin1String("description"));

    QVERIFY(!ftprops.hasLastModificationTime());
    QVERIFY(!ftprops.lastModificationTime().isValid());
    QDateTime now = QDateTime::currentDateTime();
    ftprops.setLastModificationTime(now);
    QVERIFY(ftprops.hasLastModificationTime());
    QCOMPARE(ftprops.lastModificationTime(), now);

    QVERIFY(!ftprops.hasUri());
    QVERIFY(ftprops.uri().isEmpty());
    ftprops.setUri(QLatin1String("file:///path/filename"));
    QVERIFY(ftprops.isValid());
    QVERIFY(ftprops.hasUri());
    QCOMPARE(ftprops.uri(), QLatin1String("file:///path/filename"));
}

void TestFileTransferCreationProperties::testFileTransferCreationPropertiesDefaultByPath()
{
    // Test constructor by local file path with existing file
    QString filePath = QCoreApplication::applicationFilePath();
    QFileInfo fileInfo(filePath);
    QUrl fileUri = QUrl::fromLocalFile(filePath);

    FileTransferChannelCreationProperties ftprops(filePath,
            QLatin1String("application/octet-stream"));
    QVERIFY(ftprops.isValid());
    QCOMPARE(ftprops.suggestedFileName(), fileInfo.fileName());
    QCOMPARE(ftprops.contentType(), QLatin1String("application/octet-stream"));
    QCOMPARE(ftprops.size(), (quint64)fileInfo.size());

    QVERIFY(!ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeNone);
    QVERIFY(ftprops.contentHash().isEmpty());
    QVERIFY(!ftprops.hasDescription());
    QVERIFY(ftprops.description().isEmpty());
    QVERIFY(ftprops.hasLastModificationTime());
    QCOMPARE(ftprops.lastModificationTime(), fileInfo.lastModified());
    QVERIFY(ftprops.hasUri());
    QCOMPARE(ftprops.uri(), fileUri.toString());
}

void TestFileTransferCreationProperties::testFileTransferCreationPropertiesDefaultByPathFail()
{
    // Test constructor by local file path with non-existing file
    FileTransferChannelCreationProperties ftprops(QLatin1String("/non-existent-path/non-existent-filename"),
            QLatin1String("application/octet-stream"));
    QVERIFY(!ftprops.isValid());
    QVERIFY(ftprops.suggestedFileName().isEmpty());
    QVERIFY(ftprops.contentType().isEmpty());
    QCOMPARE(ftprops.size(), (qulonglong)0);

    QVERIFY(!ftprops.hasContentHash());
    QCOMPARE(ftprops.contentHashType(), FileHashTypeNone);
    QVERIFY(ftprops.contentHash().isEmpty());
    QVERIFY(!ftprops.hasDescription());
    QVERIFY(ftprops.description().isEmpty());
    QVERIFY(!ftprops.hasLastModificationTime());
    QVERIFY(!ftprops.lastModificationTime().isValid());
    QVERIFY(!ftprops.hasUri());
    QVERIFY(ftprops.uri().isEmpty());
}

QTEST_MAIN(TestFileTransferCreationProperties)

#include "_gen/file-transfer-channel-creation-properties.cpp.moc.hpp"
