#include <QtCore/QEventLoop>
#include <QtTest/QtTest>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/PendingOperation>

using namespace Telepathy;
using namespace Telepathy::Client;

class TestAccountBasics : public QObject
{
    Q_OBJECT

public:
    TestAccountBasics(QObject *parent = 0);
    ~TestAccountBasics();

protected Q_SLOTS:
    void expectSuccessfulCall(Telepathy::Client::PendingOperation *);

private Q_SLOTS:
    void init();

    void testBasics();

    void cleanup();

private:
    QEventLoop *mLoop;
    AccountManager *mAccount;
};

TestAccountBasics::TestAccountBasics(QObject *parent)
    : QObject(parent),
      mLoop(new QEventLoop(this)),
      mAccount(0)
{
}

TestAccountBasics::~TestAccountBasics()
{
    delete mLoop;
}

void TestAccountBasics::expectSuccessfulCall(PendingOperation *operation)
{
    if (operation->isError()) {
        qWarning().nospace() << operation->errorName()
            << ": " << operation->errorMessage();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}

void TestAccountBasics::init()
{
    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);
}

void TestAccountBasics::testBasics()
{
    mAccount = new AccountManager();
    QCOMPARE(mAccount->isReady(), false);

    connect(mAccount->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    qDebug() << "enter main loop";
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAccount->isReady(), true);

    QCOMPARE(mAccount->interfaces(), QStringList());

    QCOMPARE(mAccount->validAccountPaths(), ObjectPathList());
    QCOMPARE(mAccount->invalidAccountPaths(), ObjectPathList());
    QCOMPARE(mAccount->allAccountPaths(), ObjectPathList());
}

void TestAccountBasics::cleanup()
{
    if (mAccount) {
        delete mAccount;
        mAccount = 0;
    }
}

QTEST_MAIN(TestAccountBasics)
#include "_gen/account-basics.moc.hpp"
