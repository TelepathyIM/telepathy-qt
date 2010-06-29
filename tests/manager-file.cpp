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
    ManagerFile notFoundManagerFile(QLatin1String("test-manager-file-not-found"));
    QCOMPARE(notFoundManagerFile.isValid(), false);

    ManagerFile invalidManagerFile(QLatin1String("test-manager-file-malformed-keyfile"));
    QCOMPARE(invalidManagerFile.isValid(), false);

    ManagerFile invalidManagerFile2(QLatin1String("test-manager-file-invalid-signature"));
    QCOMPARE(invalidManagerFile2.isValid(), false);

    ManagerFile managerFile(QLatin1String("test-manager-file"));
    QCOMPARE(managerFile.isValid(), true);

    QStringList protocols = managerFile.protocols();
    protocols.sort();
    QCOMPARE(protocols,
             QStringList() << QLatin1String("bar") << QLatin1String("foo") << QLatin1String("somewhat-pathological"));

    ParamSpecList params = managerFile.parameters(QLatin1String("foo"));
    QCOMPARE(containsParam(params, "account"), true);
    QCOMPARE(containsParam(params, "encryption-key"), true);
    QCOMPARE(containsParam(params, "password"), true);
    QCOMPARE(containsParam(params, "port"), true);
    QCOMPARE(containsParam(params, "register"), true);
    QCOMPARE(containsParam(params, "server-list"), true);
    QCOMPARE(containsParam(params, "non-existant"), false);

    QCOMPARE(QLatin1String("x-foo"), managerFile.vcardField(QLatin1String("foo")));
    QCOMPARE(QLatin1String("Foo"), managerFile.englishName(QLatin1String("foo")));
    QCOMPARE(QLatin1String("im-foo"), managerFile.iconName(QLatin1String("foo")));

    RequestableChannelClass expectedRcc;
    expectedRcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
    expectedRcc.fixedProperties.insert(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
            HandleTypeContact);
    expectedRcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"));
    expectedRcc.allowedProperties.append(
            QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"));
    RequestableChannelClassList expectedRccs;
    expectedRccs.append(expectedRcc);
    QCOMPARE(expectedRccs, managerFile.requestableChannelClasses(QLatin1String("foo")));

    const ParamSpec *param;
    param = getParam(params, QLatin1String("account"));
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString(QLatin1String("s")));
    param = getParam(params, QLatin1String("password"));
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagSecret);
    QCOMPARE(param->signature, QString(QLatin1String("s")));
    param = getParam(params, QLatin1String("encryption-key"));
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagSecret);
    QCOMPARE(param->signature, QString(QLatin1String("s")));

    params = managerFile.parameters(QLatin1String("somewhat-pathological"));
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

    param = getParam(params, QLatin1String("foo"));
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagRequired | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString(QLatin1String("s")));
    param = getParam(params, QLatin1String("semicolons"));
    QCOMPARE(param->flags, (uint) ConnMgrParamFlagSecret | ConnMgrParamFlagHasDefault);
    QCOMPARE(param->signature, QString(QLatin1String("s")));
    param = getParam(params, QLatin1String("list"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QLatin1String("list") << QLatin1String("of") << QLatin1String("misc"));
    param = getParam(params, QLatin1String("escaped-semicolon-in-list"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QLatin1String("list;of") << QLatin1String("misc"));
    param = getParam(params, QLatin1String("doubly-escaped-semicolon-in-list"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QLatin1String("list\\") << QLatin1String("of") << QLatin1String("misc"));
    param = getParam(params, QLatin1String("triply-escaped-semicolon-in-list"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QLatin1String("list\\;of") << QLatin1String("misc"));
    param = getParam(params, QLatin1String("empty-list"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList());
    param = getParam(params, QLatin1String("list-of-empty-string"));
    QCOMPARE(param->signature, QString(QLatin1String("as")));
    QCOMPARE(param->defaultValue.variant().toStringList(),
             QStringList() << QString());
}

bool containsParam(const ParamSpecList &params, const char *name)
{
    Q_FOREACH (const ParamSpec &param, params) {
        if (param.name == QLatin1String(name)) {
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
