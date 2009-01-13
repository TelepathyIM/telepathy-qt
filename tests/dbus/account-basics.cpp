#include <QtCore/QEventLoop>
#include <QtTest/QtTest>
#include <QtDBus/QDBusObjectPath>

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingAccount>
#include <TelepathyQt4/Client/PendingOperation>

using namespace Telepathy;
using namespace Telepathy::Client;

static QStringList objectPathListToStringList(const ObjectPathList &paths)
{
    QStringList result;
    Q_FOREACH (const QDBusObjectPath &path, paths) {
        result << path.path();
    }
    return result;
}

class TestAccountBasics : public QObject
{
    Q_OBJECT

public:
    TestAccountBasics(QObject *parent = 0);
    ~TestAccountBasics();

protected Q_SLOTS:
    void expectSuccessfulCall(Telepathy::Client::PendingOperation *);
    void onAccountCreated(Telepathy::Client::PendingOperation *);
    void onAccountReady(Telepathy::Client::PendingOperation *);
    void onAvatarChanged(const Telepathy::Avatar &);

private Q_SLOTS:
    void init();

    void testBasics();

    void cleanup();

private:
    QEventLoop *mLoop;
    AccountManager *mAM;
};

TestAccountBasics::TestAccountBasics(QObject *parent)
    : QObject(parent),
      mLoop(new QEventLoop(this)),
      mAM(0)
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

void TestAccountBasics::onAccountCreated(Telepathy::Client::PendingOperation *operation)
{
    if (operation->isError()) {
        qWarning().nospace() << operation->errorName()
            << ": " << operation->errorMessage();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}

void TestAccountBasics::onAccountReady(Telepathy::Client::PendingOperation *operation)
{
    if (operation->isError()) {
        qWarning().nospace() << operation->errorName()
            << ": " << operation->errorMessage();
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}

void TestAccountBasics::onAvatarChanged(const Telepathy::Avatar &avatar)
{
    qDebug() << "on avatar changed";
    QCOMPARE(avatar.avatarData, QByteArray("asdfg"));
    QCOMPARE(avatar.MIMEType, QString("image/jpeg"));
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
    mAM = new AccountManager();
    QCOMPARE(mAM->isReady(), false);

    connect(mAM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    qDebug() << "enter main loop";
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVariantMap parameters;
    parameters["account"] = "foobar";
    PendingAccount *pacc = mAM->createAccount("foo",
            "bar", "foobar", parameters);
    connect(pacc,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onAccountCreated(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mAM->interfaces(), QStringList());

    QCOMPARE(objectPathListToStringList(mAM->validAccountPaths()),
             QStringList() <<
               "/org/freedesktop/Telepathy/Account/foo/bar/Account0");
    QCOMPARE(objectPathListToStringList(mAM->invalidAccountPaths()),
             QStringList());
    QCOMPARE(objectPathListToStringList(mAM->allAccountPaths()),
             QStringList() <<
               "/org/freedesktop/Telepathy/Account/foo/bar/Account0");

    Account *acc = mAM->accountForPath(
            QDBusObjectPath("/org/freedesktop/Telepathy/Account/foo/bar/Account0"));
    connect(acc->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onAccountReady(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->displayName(), QString("foobar (account 0)"));

    connect(acc->becomeReady(Account::FeatureAvatar),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));

    connect(acc,
            SIGNAL(avatarChanged(const Telepathy::Avatar &)),
            SLOT(onAvatarChanged(const Telepathy::Avatar &)));

    Telepathy::Avatar avatar = { QByteArray("asdfg"), "image/jpeg" };
    connect(acc->setAvatar(avatar),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    connect(acc->becomeReady(Account::FeatureAvatar),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    // wait for avatarChanged signal
    QCOMPARE(mLoop->exec(), 0);

    pacc = mAM->createAccount("spurious",
            "normal", "foobar", parameters);
    connect(pacc,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onAccountCreated(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    acc = mAM->accountForPath(
            QDBusObjectPath("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));
    connect(acc->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(onAccountReady(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    acc = mAM->accountForPath(
            QDBusObjectPath("/org/freedesktop/Telepathy/Account/spurious/normal/Account0"));
    connect(acc->becomeReady(Account::FeatureProtocolInfo),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    ProtocolInfo *protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));
    QCOMPARE(protocolInfo->hasParameter("account"), true);
    QCOMPARE(protocolInfo->hasParameter("password"), true);
    QCOMPARE(protocolInfo->hasParameter("register"), true);

    connect(acc->becomeReady(Account::FeatureAvatar),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));

    connect(acc->becomeReady(Account::FeatureProtocolInfo | Account::FeatureAvatar),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this,
            SLOT(expectSuccessfulCall(Telepathy::Client::PendingOperation *)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(acc->avatar().MIMEType, QString("image/png"));
    protocolInfo = acc->protocolInfo();
    QCOMPARE((bool) protocolInfo, !((ProtocolInfo *) 0));
}

void TestAccountBasics::cleanup()
{
    if (mAM) {
        delete mAM;
        mAM = 0;
    }
}

QTEST_MAIN(TestAccountBasics)

#include "_gen/account-basics.cpp.moc.hpp"
