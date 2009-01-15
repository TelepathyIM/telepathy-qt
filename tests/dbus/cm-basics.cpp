#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingStringList>
#include <TelepathyQt4/Debug>

#include <telepathy-glib/debug.h>

#include <tests/lib/simple-manager.h>

using namespace Telepathy::Client;

class TestCmBasics : public QObject
{
    Q_OBJECT

public:
    QEventLoop *mLoop;
    Telepathy::Client::ConnectionManager* mCM;
    TpBaseConnectionManager *mCmService;

protected Q_SLOTS:
    void expectSuccessfulCall(Telepathy::Client::PendingOperation*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();
};


void TestCmBasics::initTestCase()
{
    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    QVERIFY(QDBusConnection::sessionBus().isConnected());

    g_type_init();
    g_set_prgname("cm-basics");
    tp_debug_set_flags("all");

    mCmService = TP_BASE_CONNECTION_MANAGER(g_object_new(
        SIMPLE_TYPE_CONNECTION_MANAGER,
        NULL));
    QVERIFY(mCmService != NULL);
    QVERIFY(tp_base_connection_manager_register (mCmService));
}


void TestCmBasics::init()
{
    mLoop = new QEventLoop(this);
}


void TestCmBasics::expectSuccessfulCall(PendingOperation* op)
{
    qDebug() << "pending operation finished";
    if (op->isError()) {
        qWarning().nospace() << op->errorName()
            << ": " << op->errorMessage();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}


void TestCmBasics::testBasics()
{
    mCM = new ConnectionManager("simple");
    QCOMPARE(mCM->isReady(), false);

    connect(mCM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    qDebug() << "enter main loop";
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    // calling becomeReady() twice is a no-op
    connect(mCM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);

    QCOMPARE(mCM->interfaces(), QStringList());
    QCOMPARE(mCM->supportedProtocols(), QStringList() << "simple");

    Q_FOREACH (ProtocolInfo *info, mCM->protocols()) {
        QVERIFY(info != 0);
        QVERIFY(info->cmName() == "simple");
        QVERIFY(info->name() == "simple");

        QCOMPARE(info->hasParameter("account"), true);
        QCOMPARE(info->hasParameter("not-there"), false);

        Q_FOREACH (ProtocolParameter *param, info->parameters()) {
            QCOMPARE(param->name(), QString::fromAscii("account"));
            QCOMPARE(param->dbusSignature().signature(),
                         QLatin1String("s"));
            QCOMPARE(param->isRequired(), true);
            QCOMPARE(param->isSecret(), false);
        }
        QCOMPARE(info->canRegister(), false);
    }

    QCOMPARE(mCM->supportedProtocols(), QStringList() << "simple");
}


void TestCmBasics::cleanup()
{
    if (mCM != NULL) {
        delete mCM;
        mCM = NULL;
    }
    if (mLoop != NULL) {
        delete mLoop;
        mLoop = NULL;
    }
}


void TestCmBasics::cleanupTestCase()
{
    if (mCmService != NULL) {
        g_object_unref(mCmService);
        mCmService = NULL;
    }
}


QTEST_MAIN(TestCmBasics)
#include "_gen/cm-basics.cpp.moc.hpp"
