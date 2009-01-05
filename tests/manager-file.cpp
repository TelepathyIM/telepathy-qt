#include <QtTest/QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ManagerFile>

using namespace Telepathy;

bool containsParam(const ParamSpecList &params, const char *name);
const ParamSpec *getParam(const ParamSpecList &params, const QString &name);

class TestManagerFile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testManagerFile();
};

void TestManagerFile::testManagerFile()
{
    ManagerFile notFoundManagerFile("test-manager-file-not-found");
    QCOMPARE(notFoundManagerFile.isValid(), false);

    ManagerFile invalidManagerFile("test-manager-file-invalid");
    QCOMPARE(invalidManagerFile.isValid(), false);

    ManagerFile managerFile("test-manager-file");
    QCOMPARE(managerFile.isValid(), true);

    QCOMPARE(managerFile.protocols(),
             QStringList() << "foo" << "bar");

    ParamSpecList params = managerFile.parameters("foo");
    QCOMPARE(containsParam(params, "account"), true);
    QCOMPARE(containsParam(params, "encryption-key"), true);
    QCOMPARE(containsParam(params, "password"), true);
    QCOMPARE(containsParam(params, "port"), true);
    QCOMPARE(containsParam(params, "register"), true);
    QCOMPARE(containsParam(params, "server-list"), true);
    QCOMPARE(containsParam(params, "non-existant"), false);

    const ParamSpec *param;
    param = getParam(params, "account");
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString("s"));
    param = getParam(params, "password");
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagSecret);
    QCOMPARE(param->signature, QString("s"));
    param = getParam(params, "encryption-key");
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagSecret);
    QCOMPARE(param->signature, QString("s"));
}

bool containsParam(const ParamSpecList &params, const char *name)
{
    Q_FOREACH (const ParamSpec &param, params) {
        if (param.name == name) {
            return true;
        }
    }
    return false;
}

const ParamSpec *getParam(const ParamSpecList &params, const QString &name)
{
    Q_FOREACH (const ParamSpec &param, params) {
        if (param.name == name) {
            return &param;
        }
    }
    return NULL;
}

QTEST_MAIN(TestManagerFile)
#include "manager-file.moc.hpp"
