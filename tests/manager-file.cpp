#include <QtTest/QtTest>

#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ManagerFile>

using namespace Tp;

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

    QStringList protocols = managerFile.protocols();
    protocols.sort();
    QCOMPARE(protocols,
             QStringList() << "bar" << "foo" << "somewhat-pathological");

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

    params = managerFile.parameters("somewhat-pathological");
    QCOMPARE(containsParam(params, "foo"), true);
    QCOMPARE(containsParam(params, "semicolons"), true);
    QCOMPARE(containsParam(params, "list"), true);
    QCOMPARE(containsParam(params, "unterminated-list"), true);
    QCOMPARE(containsParam(params, "spaces-in-list"), true);
    QCOMPARE(containsParam(params, "escaped-semicolon-in-list"), true);
    QCOMPARE(containsParam(params, "doubly-escaped-semicolon-in-list"), true);
    QCOMPARE(containsParam(params, "triply-escaped-semicolon-in-list"), true);
    QCOMPARE(containsParam(params, "empty-list"), true);
    QCOMPARE(containsParam(params, "escaped-semicolon"), true);
    QCOMPARE(containsParam(params, "object"), true);
    QCOMPARE(containsParam(params, "non-existant"), false);

    param = getParam(params, "foo");
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString("s"));
    param = getParam(params, "semicolons");
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagSecret | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString("s"));
    param = getParam(params, "list");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << "list" << "of" << "misc");
    param = getParam(params, "escaped-semicolon-in-list");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << "list;of" << "misc");
    param = getParam(params, "doubly-escaped-semicolon-in-list");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << "list\\" << "of" << "misc");
    param = getParam(params, "triply-escaped-semicolon-in-list");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << "list\\;of" << "misc");
    param = getParam(params, "empty-list");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList());
    param = getParam(params, "list-of-empty-string");
    QCOMPARE(param->signature, QString("as"));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QString());
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

#include "_gen/manager-file.cpp.moc.hpp"
