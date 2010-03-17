#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingChannelRequest>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/test.h>

using namespace Tp;
using namespace Tp::Client;

class ChannelRequestAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelRequest")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelRequest\" >\n"
"    <property name=\"Account\" type=\"o\" access=\"read\" />\n"
"    <property name=\"UserActionTime\" type=\"x\" access=\"read\" />\n"
"    <property name=\"PreferredHandler\" type=\"s\" access=\"read\" />\n"
"    <property name=\"Requests\" type=\"aa{sv}\" access=\"read\" />\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <method name=\"Proceed\" />\n"
"    <method name=\"Cancel\" />\n"
"    <signal name=\"Failed\" >\n"
"      <arg name=\"Error\" type=\"s\" />\n"
"      <arg name=\"Message\" type=\"s\" />\n"
"    </signal>\n"
"    <signal name=\"Succeeded\" />"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Account READ Account)
    Q_PROPERTY(qulonglong UserActionTime READ UserActionTime)
    Q_PROPERTY(QString PreferredHandler READ PreferredHandler)
    Q_PROPERTY(QualifiedPropertyValueMapList Requests READ Requests)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    ChannelRequestAdaptor(QDBusObjectPath account,
            qulonglong userActionTime,
            QString preferredHandler,
            QualifiedPropertyValueMapList requests,
            QStringList interfaces,
            bool shouldFail,
            bool proceedNoop,
            QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mAccount(account), mUserActionTime(userActionTime),
          mPreferredHandler(preferredHandler), mRequests(requests),
          mInterfaces(interfaces), mShouldFail(shouldFail),
          mProceedNoop(false)
    {
    }

    virtual ~ChannelRequestAdaptor()
    {
    }

public: // Properties
    inline QDBusObjectPath Account() const
    {
        return mAccount;
    }

    inline qulonglong UserActionTime() const
    {
        return mUserActionTime;
    }

    inline QString PreferredHandler() const
    {
        return mPreferredHandler;
    }

    inline QualifiedPropertyValueMapList Requests() const
    {
        return mRequests;
    }

    inline QStringList Interfaces() const
    {
        return mInterfaces;
    }

public Q_SLOTS: // Methods
    void Proceed()
    {
        if (mProceedNoop) {
            return;
        }

        if (mShouldFail) {
            QTimer::singleShot(0, this, SLOT(fail()));
        } else {
            QTimer::singleShot(0, this, SIGNAL(Succeeded()));
        }
    }

    void Cancel()
    {
        emit Failed(QLatin1String(TELEPATHY_ERROR_CANCELLED), QLatin1String("Cancelled"));
    }

Q_SIGNALS: // Signals
    void Failed(const QString &error, const QString &message);
    void Succeeded();

private Q_SLOTS:
    void fail()
    {
        emit Failed(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE), QLatin1String("Not available"));
    }

private:
    QDBusObjectPath mAccount;
    qulonglong mUserActionTime;
    QString mPreferredHandler;
    QualifiedPropertyValueMapList mRequests;
    QStringList mInterfaces;
    bool mShouldFail;
    bool mProceedNoop;
};

class ChannelDispatcherAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelDispatcher")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelDispatcher\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <method name=\"CreateChannel\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requested_Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"x\" direction=\"in\" />\n"
"      <arg name=\"Preferred_Handler\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Channel_Object_Path\" type=\"o\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"EnsureChannel\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requested_Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"x\" direction=\"in\" />\n"
"      <arg name=\"Preferred_Handler\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Channel_Object_Path\" type=\"o\" direction=\"out\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)

public:
    ChannelDispatcherAdaptor(const QDBusConnection &bus, QObject *parent)
        : QDBusAbstractAdaptor(parent), mBus(bus),
          mRequests(0), mChannelRequestShouldFail(false),
          mChannelRequestProceedNoop(false)
    {
    }

    virtual ~ChannelDispatcherAdaptor()
    {
    }

public: // Properties
    inline QStringList Interfaces() const
    {
        return QStringList();
    }

public Q_SLOTS: // Methods
    QDBusObjectPath CreateChannel(const QDBusObjectPath& account,
            const QVariantMap& requestedProperties, qlonglong userActionTime,
            const QString& preferredHandler)
    {
        return createChannel(account, requestedProperties,
                userActionTime, preferredHandler);
    }

    QDBusObjectPath EnsureChannel(const QDBusObjectPath& account,
            const QVariantMap& requestedProperties, qlonglong userActionTime,
            const QString& preferredHandler)
    {
        return createChannel(account, requestedProperties,
                userActionTime, preferredHandler);
    }

private:
    friend class TestAccountChannelDispatcher;

    QDBusObjectPath createChannel(const QDBusObjectPath& account,
            const QVariantMap& requestedProperties, qlonglong userActionTime,
            const QString& preferredHandler)
    {
        QObject *request = new QObject(this);
        new ChannelRequestAdaptor(
                account,
                userActionTime,
                preferredHandler,
                QualifiedPropertyValueMapList(),
                QStringList(),
                mChannelRequestShouldFail,
                mChannelRequestProceedNoop,
                request);
        QString channelRequestPath =
            QString(QLatin1String("/org/freedesktop/Telepathy/ChannelRequest/_%1"))
                .arg(mRequests++);
        mBus.registerService(QLatin1String("org.freedesktop.Telepathy.ChannelDispatcher"));
        mBus.registerObject(channelRequestPath, request);
        return QDBusObjectPath(channelRequestPath);
    }

    QDBusConnection mBus;
    uint mRequests;
    bool mChannelRequestShouldFail;
    bool mChannelRequestProceedNoop;
};

class TestAccountChannelDispatcher : public Test
{
    Q_OBJECT

public:
    TestAccountChannelDispatcher(QObject *parent = 0)
        : Test(parent),
          mChannelDispatcherAdaptor(0),
          mChannelRequestFinished(false),
          mChannelRequestFinishedWithError(false)
    { }

protected Q_SLOTS:
    void onPendingChannelRequestFinished(Tp::PendingOperation *op);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testEnsureTextChat();
    void testEnsureTextChatFail();
    void testEnsureTextChatCancel();
    void testEnsureTextChatroom();
    void testEnsureTextChatroomFail();
    void testEnsureTextChatroomCancel();
    void testEnsureMediaCall();
    void testEnsureMediaCallFail();
    void testEnsureMediaCallCancel();
    void testCreateChannel();
    void testCreateChannelFail();
    void testCreateChannelCancel();
    void testEnsureChannel();
    void testEnsureChannelFail();
    void testEnsureChannelCancel();

    void cleanup();
    void cleanupTestCase();

private:
    void testPCR(PendingChannelRequest *pcr);

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    ChannelDispatcherAdaptor *mChannelDispatcherAdaptor;
    QDateTime mUserActionTime;
    ChannelRequestPtr mChannelRequest;
    bool mChannelRequestFinished;
    bool mChannelRequestFinishedWithError;
    QString mChannelRequestFinishedErrorName;
};

void TestAccountChannelDispatcher::onPendingChannelRequestFinished(
        Tp::PendingOperation *op)
{
    mChannelRequestFinished = true;
    mChannelRequestFinishedWithError = op->isError();
    mChannelRequestFinishedErrorName = op->errorName();
    mLoop->exit(0);
}

void TestAccountChannelDispatcher::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("account-channel-dispatcher");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mAM = AccountManager::create();
    QVERIFY(connect(mAM->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAM->isReady(), true);

    QVariantMap parameters;
    parameters[QLatin1String("account")] = QLatin1String("foobar");
    PendingAccount *pacc = mAM->createAccount(QLatin1String("foo"),
            QLatin1String("bar"), QLatin1String("foobar"), parameters);
    QVERIFY(connect(pacc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QVERIFY(pacc->account());
    mAccount = pacc->account();
    QVERIFY(connect(mAccount->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mAccount->isReady(), true);

    QDBusConnection bus = mAccount->dbusConnection();
    QString channelDispatcherBusName = QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCHER);
    QString channelDispatcherPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher");
    QObject *dispatcher = new QObject(this);
    mChannelDispatcherAdaptor = new ChannelDispatcherAdaptor(bus, dispatcher);
    QVERIFY(bus.registerService(channelDispatcherBusName));
    QVERIFY(bus.registerObject(channelDispatcherPath, dispatcher));
}

void TestAccountChannelDispatcher::init()
{
    initImpl();

    mChannelRequest.reset();
    mChannelRequestFinished = false;
    mChannelRequestFinishedWithError = false;
    mChannelRequestFinishedErrorName = QString();
    QDateTime mUserActionTime = QDateTime::currentDateTime();
}

void TestAccountChannelDispatcher::testPCR(PendingChannelRequest *pcr)
{
    QVERIFY(connect(pcr,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onPendingChannelRequestFinished(Tp::PendingOperation *))));
    mLoop->exec(0);

    mChannelRequest = pcr->channelRequest();
    QVERIFY(connect(mChannelRequest->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    mLoop->exec(0);
    QCOMPARE(mChannelRequest->userActionTime(), mUserActionTime);
}

#define TEST_ENSURE_CHANNEL_SPECIFIC(method_name, shouldFail, proceedNoop, expectedError) \
    mChannelDispatcherAdaptor->mChannelRequestShouldFail = shouldFail; \
    mChannelDispatcherAdaptor->mChannelRequestProceedNoop = proceedNoop; \
    PendingChannelRequest *pcr = mAccount->method_name(QLatin1String("foo@bar"), \
            mUserActionTime, QString()); \
    testPCR(pcr); \
    QCOMPARE(mChannelRequestFinishedWithError, shouldFail); \
    if (shouldFail) \
        QCOMPARE(mChannelRequestFinishedErrorName, QString(QLatin1String(expectedError)));

#define TEST_CREATE_ENSURE_CHANNEL(method_name, shouldFail, proceedNoop, expectedError) \
    QVariantMap request; \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"), \
                                 QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"), \
                                 (uint) Tp::HandleTypeContact); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"), \
                                 QLatin1String("foo@bar")); \
    mChannelDispatcherAdaptor->mChannelRequestShouldFail = shouldFail; \
    mChannelDispatcherAdaptor->mChannelRequestProceedNoop = proceedNoop; \
    PendingChannelRequest *pcr = mAccount->method_name(request, \
            mUserActionTime, QString()); \
    testPCR(pcr); \
    QCOMPARE(mChannelRequestFinishedWithError, shouldFail); \
    if (shouldFail) \
        QCOMPARE(mChannelRequestFinishedErrorName, QString(QLatin1String(expectedError)));

void TestAccountChannelDispatcher::testEnsureTextChat()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChat, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureTextChatFail()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChat, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureTextChatCancel()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChat, false, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testEnsureTextChatroom()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureTextChatroomFail()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureTextChatroomCancel()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, false, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testEnsureMediaCall()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureMediaCall, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureMediaCallFail()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureMediaCall, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureMediaCallCancel()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureMediaCall, false, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testCreateChannel()
{
    TEST_CREATE_ENSURE_CHANNEL(createChannel, false, false, "");
}

void TestAccountChannelDispatcher::testCreateChannelFail()
{
    TEST_CREATE_ENSURE_CHANNEL(createChannel, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testCreateChannelCancel()
{
    TEST_CREATE_ENSURE_CHANNEL(createChannel, false, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testEnsureChannel()
{
    TEST_CREATE_ENSURE_CHANNEL(ensureChannel, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureChannelFail()
{
    TEST_CREATE_ENSURE_CHANNEL(ensureChannel, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureChannelCancel()
{
    TEST_CREATE_ENSURE_CHANNEL(ensureChannel, false, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::cleanup()
{
    cleanupImpl();
}

void TestAccountChannelDispatcher::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestAccountChannelDispatcher)
#include "_gen/account-channel-dispatcher.cpp.moc.hpp"
