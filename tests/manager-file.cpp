#include <QtTest/QtTest>

#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include "TelepathyQt/manager-file.h"

using namespace Tp;

namespace
{

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

PresenceSpec getPresenceSpec(const PresenceSpecList &specs, const QString &status)
{
    foreach (const PresenceSpec &spec, specs) {
        if (spec.presence().status() == status) {
            return spec;
        }
    }
    return PresenceSpec();
}

}

class TestManagerFile : public QObject
{
    Q_OBJECT

public:
    TestManagerFile(QObject *parent = 0);

private Q_SLOTS:
    void testManagerFile();
};

TestManagerFile::TestManagerFile(QObject *parent)
    : QObject(parent)
{
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
}

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

    QStringList addressableVCardFields = managerFile.addressableVCardFields(QLatin1String("foo"));
    QCOMPARE(addressableVCardFields, QStringList() << QLatin1String("x-foo"));
    QStringList addressableUriSchemes = managerFile.addressableUriSchemes(QLatin1String("foo"));
    QCOMPARE(addressableUriSchemes, QStringList() << QLatin1String("foo"));

    RequestableChannelClass ftRcc;
    ftRcc.fixedProperties.insert(
            TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
            TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    ftRcc.fixedProperties.insert(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
            HandleTypeContact);
    ftRcc.fixedProperties.insert(
            TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHashType"),
            FileHashTypeMD5);
    ftRcc.allowedProperties.append(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    ftRcc.allowedProperties.append(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));

    RequestableChannelClass fooTextRcc;
    fooTextRcc.fixedProperties.insert(
            TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType"),
            TP_QT_IFACE_CHANNEL_TYPE_TEXT);
    fooTextRcc.fixedProperties.insert(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"),
            HandleTypeContact);
    fooTextRcc.allowedProperties.append(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    fooTextRcc.allowedProperties.append(
            TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));

    RequestableChannelClassList expectedRccs;

    expectedRccs.append(ftRcc);
    expectedRccs.append(fooTextRcc);

    QCOMPARE(expectedRccs.size(), managerFile.requestableChannelClasses(QLatin1String("foo")).size());

    qDebug() << managerFile.requestableChannelClasses(QLatin1String("foo"))[0].fixedProperties;
    qDebug() << managerFile.requestableChannelClasses(QLatin1String("foo"))[1].fixedProperties;

    QCOMPARE(ftRcc, managerFile.requestableChannelClasses(QLatin1String("foo"))[0]);
    QCOMPARE(fooTextRcc, managerFile.requestableChannelClasses(QLatin1String("foo"))[1]);

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

    PresenceSpecList statuses = managerFile.allowedPresenceStatuses(QLatin1String("foo"));
    QCOMPARE(statuses.size(), 3);

    PresenceSpec spec;
    spec = getPresenceSpec(statuses, QLatin1String("offline"));
    QCOMPARE(spec.isValid(), true);
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeOffline);
    QCOMPARE(spec.maySetOnSelf(), false);
    QCOMPARE(spec.canHaveStatusMessage(), false);
    spec = getPresenceSpec(statuses, QLatin1String("dnd"));
    QCOMPARE(spec.isValid(), true);
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeBusy);
    QCOMPARE(spec.maySetOnSelf(), true);
    QCOMPARE(spec.canHaveStatusMessage(), false);
    spec = getPresenceSpec(statuses, QLatin1String("available"));
    QCOMPARE(spec.isValid(), true);
    QVERIFY(spec.presence().type() == ConnectionPresenceTypeAvailable);
    QCOMPARE(spec.maySetOnSelf(), true);
    QCOMPARE(spec.canHaveStatusMessage(), true);

    AvatarSpec avatarReqs = managerFile.avatarRequirements(QLatin1String("foo"));
    QStringList supportedMimeTypes = avatarReqs.supportedMimeTypes();
    supportedMimeTypes.sort();
    QCOMPARE(supportedMimeTypes,
             QStringList() << QLatin1String("image/gif") << QLatin1String("image/jpeg") <<
                              QLatin1String("image/png"));
    QCOMPARE(avatarReqs.minimumHeight(), (uint) 32);
    QCOMPARE(avatarReqs.maximumHeight(), (uint) 96);
    QCOMPARE(avatarReqs.recommendedHeight(), (uint) 64);
    QCOMPARE(avatarReqs.minimumWidth(), (uint) 32);
    QCOMPARE(avatarReqs.maximumWidth(), (uint) 96);
    QCOMPARE(avatarReqs.recommendedWidth(), (uint) 64);
    QCOMPARE(avatarReqs.maximumBytes(), (uint) 8192);

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

QTEST_MAIN(TestManagerFile)

#include "_gen/manager-file.cpp.moc.hpp"
