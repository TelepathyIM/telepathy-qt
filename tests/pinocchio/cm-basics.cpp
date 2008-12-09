#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <TelepathyQt4/Client/ConnectionManager>

#include <tests/pinocchio/lib.h>

using namespace Telepathy::Client;

class TestCmBasics : public PinocchioTest
{
    Q_OBJECT

private:
    Telepathy::Client::ConnectionManager* mCM;

protected Q_SLOTS:
    void onCmReady(ConnectionManager*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();
};


void TestCmBasics::initTestCase()
{
    initTestCaseImpl();

    // Wait for the CM to start up
    QVERIFY(waitForPinocchio(5000));
}


void TestCmBasics::init()
{
    initImpl();
}


void TestCmBasics::onCmReady(ConnectionManager* it)
{
    if (mCM != it) {
        qWarning() << "Got the wrong CM pointer";
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}


void TestCmBasics::testBasics()
{
    mCM = new ConnectionManager("pinocchio");
    QCOMPARE(mCM->isReady(), false);

    connect(mCM, SIGNAL(ready(ConnectionManager*)),
            SLOT(onCmReady(ConnectionManager*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);
    disconnect(mCM, SIGNAL(ready(ConnectionManager*)),
            this, SLOT(onCmReady(ConnectionManager*)));

    QCOMPARE(mCM->interfaces(), QStringList());
    QCOMPARE(mCM->supportedProtocols(), QStringList() << "dummy");

    const ProtocolInfo* info = mCM->protocolInfo("not-there");
    QVERIFY(info == 0);
    info = mCM->protocolInfo("dummy");
    QVERIFY(info != 0);
    QVERIFY(info->cmName() == "pinocchio");
    QVERIFY(info->protocolName() == "dummy");
    QSet<QString> paramSet = QSet<QString>();
    Q_FOREACH (QString param, info->parameters()) {
        paramSet << param;
    }
    QCOMPARE(paramSet, QSet<QString>() << "account" << "password");

    QCOMPARE(info->hasParameter("account"), true);
    QCOMPARE(info->hasParameter("not-there"), false);

    QCOMPARE(info->parameterDBusSignature("account").signature(),
            QLatin1String("s"));
    QCOMPARE(info->parameterDBusSignature("not-there").signature(), QString());

    QCOMPARE(info->parameterIsRequired("account"), true);
    QCOMPARE(info->parameterIsRequired("password"), false);
    QCOMPARE(info->parameterIsRequired("not-there"), false);

    QCOMPARE(info->parameterIsSecret("account"), false);
    QCOMPARE(info->parameterIsSecret("password"), true);
    QCOMPARE(info->parameterIsSecret("not-there"), false);

    QCOMPARE(info->parameterIsDBusProperty("account"), false);
    QCOMPARE(info->parameterIsDBusProperty("not-there"), false);

    QCOMPARE(info->parameterHasDefault("not-there"), false);

    QCOMPARE(info->canRegister(), false);

    QCOMPARE(mCM->supportedProtocols(), QStringList() << "dummy");
}


void TestCmBasics::cleanup()
{
    if (mCM != NULL) {
        delete mCM;
        mCM = NULL;
    }
    cleanupImpl();
}


void TestCmBasics::cleanupTestCase()
{
    cleanupTestCaseImpl();
}


QTEST_MAIN(TestCmBasics)
#include "_gen/cm-basics.cpp.moc.hpp"
