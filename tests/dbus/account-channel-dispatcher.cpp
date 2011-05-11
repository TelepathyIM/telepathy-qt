#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtTest/QtTest>

#include <QDateTime>
#include <QString>
#include <QVariantMap>

#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelRequest>
#include <TelepathyQt4/ChannelRequestHints>
#include <TelepathyQt4/Client>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/HandledChannelNotifier>
#include <TelepathyQt4/PendingAccount>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingChannelRequest>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Types>

#include <telepathy-glib/debug.h>

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <tests/lib/glib/contacts-conn.h>
#include <tests/lib/glib/echo/chan.h>
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
"    <property name=\"Hints\" type=\"a{sv}\" access=\"read\" />\n"
"    <method name=\"Proceed\" />\n"
"    <method name=\"Cancel\" />\n"
"    <signal name=\"Failed\" >\n"
"      <arg name=\"Error\" type=\"s\" />\n"
"      <arg name=\"Message\" type=\"s\" />\n"
"    </signal>\n"
"    <signal name=\"Succeeded\" />\n"
"    <signal name=\"SucceededWithChannel\" >\n"
"      <arg name=\"Connection\" type=\"o\" />\n"
"      <arg name=\"ConnectionProperties\" type=\"a{sv}\" />\n"
"      <arg name=\"Channel\" type=\"o\" />\n"
"      <arg name=\"ChannelProperties\" type=\"a{sv}\" />\n"
"    </signal>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QDBusObjectPath Account READ Account)
    Q_PROPERTY(qulonglong UserActionTime READ UserActionTime)
    Q_PROPERTY(QString PreferredHandler READ PreferredHandler)
    Q_PROPERTY(QualifiedPropertyValueMapList Requests READ Requests)
    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(QVariantMap Hints READ Hints)

public:
    ChannelRequestAdaptor(QDBusObjectPath account,
            qulonglong userActionTime,
            QString preferredHandler,
            QualifiedPropertyValueMapList requests,
            QStringList interfaces,
            bool shouldFail,
            bool proceedNoop,
            QVariantMap hints,
            QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mAccount(account), mUserActionTime(userActionTime),
          mPreferredHandler(preferredHandler), mRequests(requests),
          mInterfaces(interfaces), mShouldFail(shouldFail),
          mProceedNoop(proceedNoop), mHints(hints)
    {
    }

    virtual ~ChannelRequestAdaptor()
    {
    }

    void setChan(const QString &connPath, const QVariantMap &connProps,
            const QString &chanPath, const QVariantMap &chanProps)
    {
        mConnPath = connPath;
        mConnProps = connProps;
        mChanPath = chanPath;
        mChanProps = chanProps;
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

    inline QVariantMap Hints() const
    {
        return mHints;
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
            QTimer::singleShot(0, this, SLOT(succeed()));
        }
    }

    void Cancel()
    {
        Q_EMIT Failed(QLatin1String(TELEPATHY_ERROR_CANCELLED), QLatin1String("Cancelled"));
    }

Q_SIGNALS: // Signals
    void Failed(const QString &error, const QString &message);
    void Succeeded();
    void SucceededWithChannel(const QDBusObjectPath &connPath, const QVariantMap &connProps,
            const QDBusObjectPath &chanPath, const QVariantMap &chanProps);

private Q_SLOTS:
    void fail()
    {
        Q_EMIT Failed(QLatin1String(TELEPATHY_ERROR_NOT_AVAILABLE), QLatin1String("Not available"));
    }

    void succeed()
    {
        if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) {
            Q_EMIT SucceededWithChannel(QDBusObjectPath(mConnPath), mConnProps,
                    QDBusObjectPath(mChanPath), mChanProps);
        }

        Q_EMIT Succeeded();
    }

private:
    QDBusObjectPath mAccount;
    qulonglong mUserActionTime;
    QString mPreferredHandler;
    QualifiedPropertyValueMapList mRequests;
    QStringList mInterfaces;
    bool mShouldFail;
    bool mProceedNoop;
    QVariantMap mHints;
    QString mConnPath, mChanPath;
    QVariantMap mConnProps, mChanProps;
};

class ChannelDispatcherAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ChannelDispatcher")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Telepathy.ChannelDispatcher\" >\n"
"    <property name=\"Interfaces\" type=\"as\" access=\"read\" />\n"
"    <property name=\"SupportsRequestHints\" type=\"b\" access=\"read\" />\n"
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
"    <method name=\"CreateChannelWithHints\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requested_Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"x\" direction=\"in\" />\n"
"      <arg name=\"Preferred_Handler\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Hints\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"Channel_Object_Path\" type=\"o\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"EnsureChannelWithHints\" >\n"
"      <arg name=\"Account\" type=\"o\" direction=\"in\" />\n"
"      <arg name=\"Requested_Properties\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"User_Action_Time\" type=\"x\" direction=\"in\" />\n"
"      <arg name=\"Preferred_Handler\" type=\"s\" direction=\"in\" />\n"
"      <arg name=\"Hints\" type=\"a{sv}\" direction=\"in\" />\n"
"      <arg name=\"Channel_Object_Path\" type=\"o\" direction=\"out\" />\n"
"    </method>\n"
"  </interface>\n"
        "")

    Q_PROPERTY(QStringList Interfaces READ Interfaces)
    Q_PROPERTY(bool SupportsRequestHints READ SupportsRequestHints)

public:
    ChannelDispatcherAdaptor(const QDBusConnection &bus, QObject *parent)
        : QDBusAbstractAdaptor(parent),
          mBus(bus),
          mRequests(0),
          mCurRequest(0),
          mInvokeHandler(false),
          mChannelRequestShouldFail(false),
          mChannelRequestProceedNoop(false)
    {
    }

    virtual ~ChannelDispatcherAdaptor()
    {
    }

    void setChan(const QString &connPath, const QVariantMap &connProps,
            const QString &chanPath, const QVariantMap &chanProps)
    {
        mConnPath = connPath;
        mConnProps = connProps;
        mChanPath = chanPath;
        mChanProps = chanProps;
    }

    void clearChan()
    {
        mConnPath = QString();
        mConnProps.clear();
        mChanPath = QString();
        mChanProps.clear();
    }

public: // Properties
    inline QStringList Interfaces() const
    {
        return QStringList();
    }

    inline bool SupportsRequestHints() const
    {
        return true;
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

    QDBusObjectPath CreateChannelWithHints(const QDBusObjectPath &account,
            const QVariantMap &requestedProperties, qlonglong userActionTime,
            const QString &preferredHandler, const QVariantMap &hints)
    {
        return createChannel(account, requestedProperties,
                userActionTime, preferredHandler, hints);
    }

    QDBusObjectPath EnsureChannelWithHints(const QDBusObjectPath &account,
            const QVariantMap &requestedProperties, qlonglong userActionTime,
            const QString &preferredHandler, const QVariantMap &hints)
    {
        return createChannel(account, requestedProperties,
                userActionTime, preferredHandler, hints);
    }

private:
    friend class TestAccountChannelDispatcher;

    QDBusObjectPath createChannel(const QDBusObjectPath &account,
            const QVariantMap &requestedProperties, qlonglong userActionTime,
            const QString &preferredHandler, const QVariantMap &hints = QVariantMap())
    {
        QObject *request = new QObject(this);

        mCurRequest = new ChannelRequestAdaptor(
                account,
                userActionTime,
                preferredHandler,
                QualifiedPropertyValueMapList(),
                QStringList(),
                mChannelRequestShouldFail,
                mChannelRequestProceedNoop,
                hints,
                request);
        mCurRequest->setChan(mConnPath, mConnProps, mChanPath, mChanProps);
        mCurRequestPath = QString(QLatin1String("/org/freedesktop/Telepathy/ChannelRequest/_%1"))
                .arg(mRequests++);
        mBus.registerService(QLatin1String("org.freedesktop.Telepathy.ChannelDispatcher"));
        mBus.registerObject(mCurRequestPath, request);

        mCurPreferredHandler = preferredHandler;

        if (mInvokeHandler && !mConnPath.isEmpty() && !mChanPath.isEmpty()) {
            invokeHandler(userActionTime);
        }

        return QDBusObjectPath(mCurRequestPath);
    }

    void invokeHandler(const qulonglong &userActionTime)
    {
        QString channelHandlerPath = QString(QLatin1String("/%1")).arg(mCurPreferredHandler);
        channelHandlerPath.replace(QLatin1Char('.'), QLatin1Char('/'));
        Client::ClientHandlerInterface *clientHandlerInterface =
            new Client::ClientHandlerInterface(mBus, mCurPreferredHandler,
                    channelHandlerPath, this);

        ChannelDetails channelDetails = { QDBusObjectPath(mChanPath), mChanProps };
        clientHandlerInterface->HandleChannels(mCurRequest->Account(),
                QDBusObjectPath(mConnPath),
                ChannelDetailsList() << channelDetails,
                ObjectPathList() << QDBusObjectPath(mCurRequestPath),
                userActionTime, QVariantMap());
    }

    QDBusConnection mBus;
    uint mRequests;
    ChannelRequestAdaptor *mCurRequest;
    QString mCurRequestPath;
    QString mCurPreferredHandler;
    bool mInvokeHandler;
    bool mChannelRequestShouldFail;
    bool mChannelRequestProceedNoop;
    QString mConnPath, mChanPath;
    QVariantMap mConnProps, mChanProps;
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
    void onPendingChannelFinished(Tp::PendingOperation *op);
    void onChannelHandledAgain(const QDateTime &userActionTime,
            const Tp::ChannelRequestHints &hints);

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

    void testCreateAndHandleChannel();
    void testCreateAndHandleChannelNotYours();
    void testCreateAndHandleChannelFail();
    void testCreateAndHandleChannelHandledAgain();
    void testCreateAndHandleChannelHandledChannels();

    void cleanup();
    void cleanupTestCase();

private:
    void testPCR(PendingChannelRequest *pcr);
    void testPC(PendingChannel *pc, PendingChannel **pcOut = 0, ChannelPtr *channelOut = 0);

    QList<ClientHandlerInterface *> ourHandlers();
    QStringList ourHandledChannels();
    void checkHandlerHandledChannels(ClientHandlerInterface *handler, const QStringList &toCompare);

    AccountManagerPtr mAM;
    AccountPtr mAccount;
    ChannelDispatcherAdaptor *mChannelDispatcherAdaptor;
    QDateTime mUserActionTime;
    ChannelRequestPtr mChannelRequest;
    bool mChannelRequestFinished;
    bool mChannelRequestFinishedWithError;
    QString mChannelRequestFinishedErrorName;
    bool mChannelRequestAndHandleFinished;
    bool mChannelRequestAndHandleFinishedWithError;
    QString mChannelRequestAndHandleFinishedErrorName;
    QDateTime mChannelHandledAgainActionTime;
    ChannelRequestHints mHints;
    QString mConnPath, mChanPath;
    QVariantMap mConnProps, mChanProps;
};

void TestAccountChannelDispatcher::onPendingChannelRequestFinished(
        Tp::PendingOperation *op)
{
    mChannelRequestFinished = true;
    mChannelRequestFinishedWithError = op->isError();
    mChannelRequestFinishedErrorName = op->errorName();
    mLoop->exit(0);
}

void TestAccountChannelDispatcher::onPendingChannelFinished(
        Tp::PendingOperation *op)
{
    mChannelRequestAndHandleFinished = true;
    mChannelRequestAndHandleFinishedWithError = op->isError();
    mChannelRequestAndHandleFinishedErrorName = op->errorName();
    mLoop->exit(0);
}

void TestAccountChannelDispatcher::onChannelHandledAgain(const QDateTime &userActionTime,
        const Tp::ChannelRequestHints &hints)
{
    mChannelHandledAgainActionTime = userActionTime;
    mLoop->exit(0);
}

void TestAccountChannelDispatcher::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("account-channel-dispatcher");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    // Create the CD first, because Accounts try to introspect it
    QDBusConnection bus = QDBusConnection::sessionBus();
    QString channelDispatcherBusName = QLatin1String(TELEPATHY_INTERFACE_CHANNEL_DISPATCHER);
    QString channelDispatcherPath = QLatin1String("/org/freedesktop/Telepathy/ChannelDispatcher");
    QObject *dispatcher = new QObject(this);
    mChannelDispatcherAdaptor = new ChannelDispatcherAdaptor(bus, dispatcher);
    QVERIFY(bus.registerService(channelDispatcherBusName));
    QVERIFY(bus.registerObject(channelDispatcherPath, dispatcher));

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

    QCOMPARE(mAccount->supportsRequestHints(), true);
    QCOMPARE(mAccount->requestsSucceedWithChannel(), true);
}

void TestAccountChannelDispatcher::init()
{
    initImpl();

    mChannelRequest.reset();
    mChannelRequestFinished = false;
    mChannelRequestFinishedWithError = false;
    mChannelRequestFinishedErrorName = QString();
    mChannelRequestAndHandleFinished = false;
    mChannelRequestAndHandleFinishedWithError = false;
    mChannelRequestAndHandleFinishedErrorName = QString();
    mChannelHandledAgainActionTime = QDateTime();
    QDateTime mUserActionTime = QDateTime::currentDateTime();
    mHints = ChannelRequestHints();

    mConnPath.clear();
    mConnProps.clear();
    mChanPath.clear();
    mChanProps.clear();
}

void TestAccountChannelDispatcher::testPCR(PendingChannelRequest *pcr)
{
    QVERIFY(connect(pcr,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onPendingChannelRequestFinished(Tp::PendingOperation *))));
    mLoop->exec(0);

    mChannelRequest = pcr->channelRequest();

    if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) {
        QVERIFY(!mChannelRequest->channel().isNull());
        QCOMPARE(mChannelRequest->channel()->connection()->objectPath(), mConnPath);
        QCOMPARE(mChannelRequest->channel()->objectPath(), mChanPath);
        QCOMPARE(mChannelRequest->channel()->immutableProperties(), mChanProps);
    } else {
        QVERIFY(mChannelRequest->channel().isNull());
    }

    QVERIFY(connect(mChannelRequest->becomeReady(),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    mLoop->exec(0);
    QCOMPARE(mChannelRequest->userActionTime(), mUserActionTime);
    QCOMPARE(mChannelRequest->account().data(), mAccount.data());

    QVERIFY(mChannelRequest->hints().isValid());
    QCOMPARE(mChannelRequest->hints().allHints(), mHints.allHints());
}

void TestAccountChannelDispatcher::testPC(PendingChannel *pc, PendingChannel **pcOut, ChannelPtr *channelOut)
{
    if (pcOut) {
        *pcOut = pc;
    }

    QVERIFY(connect(pc,
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onPendingChannelFinished(Tp::PendingOperation *))));
    mLoop->exec(0);

    ChannelPtr channel = pc->channel();
    if (channelOut) {
        *channelOut = channel;
    }

    if (mChannelDispatcherAdaptor->mInvokeHandler && !mConnPath.isEmpty() && !mChanPath.isEmpty()) {
        QVERIFY(!channel.isNull());
        QCOMPARE(channel->connection()->objectPath(), mConnPath);
        QCOMPARE(channel->objectPath(), mChanPath);
        QCOMPARE(channel->immutableProperties(), mChanProps);
    } else {
        QVERIFY(channel.isNull());
    }
}

QList<ClientHandlerInterface *> TestAccountChannelDispatcher::ourHandlers()
{
    QList<ClientHandlerInterface *> handlers;
    QDBusConnection bus = QDBusConnection::sessionBus();
    QStringList registeredNames = bus.interface()->registeredServiceNames();
    Q_FOREACH (QString name, registeredNames) {
        if (!name.startsWith(QLatin1String("org.freedesktop.Telepathy.Client."))) {
            continue;
        }

        if (QDBusConnection::sessionBus().interface()->serviceOwner(name).value() !=
                QDBusConnection::sessionBus().baseService()) {
            continue;
        }

        QString path = QLatin1Char('/') + name;
        path.replace(QLatin1Char('.'), QLatin1Char('/'));

        ClientInterface client(name, path);
        QStringList ifaces;
        if (!waitForProperty(client.requestPropertyInterfaces(), &ifaces)) {
            continue;
        }

        if (!ifaces.contains(TP_QT4_IFACE_CLIENT_HANDLER)) {
            continue;
        }

        ClientHandlerInterface *handler = new ClientHandlerInterface(name, path, this);
        handlers.append(handler);
    }

    return handlers;
}

QStringList TestAccountChannelDispatcher::ourHandledChannels()
{
    ObjectPathList handledChannels;
    QDBusConnection bus = QDBusConnection::sessionBus();
    QStringList registeredNames = bus.interface()->registeredServiceNames();
    Q_FOREACH (QString name, registeredNames) {
        if (!name.startsWith(QLatin1String("org.freedesktop.Telepathy.Client."))) {
            continue;
        }

        if (QDBusConnection::sessionBus().interface()->serviceOwner(name).value() !=
                QDBusConnection::sessionBus().baseService()) {
            continue;
        }

        QString path = QLatin1Char('/') + name;
        path.replace(QLatin1Char('.'), QLatin1Char('/'));

        ClientInterface client(name, path);
        QStringList ifaces;
        if (!waitForProperty(client.requestPropertyInterfaces(), &ifaces)) {
            continue;
        }

        if (!ifaces.contains(TP_QT4_IFACE_CLIENT_HANDLER)) {
            continue;
        }

        ClientHandlerInterface *handler = new ClientHandlerInterface(bus, name, path, this);
        handledChannels.clear();
        if (waitForProperty(handler->requestPropertyHandledChannels(), &handledChannels)) {
            break;
        }
    }

    QStringList ret;
    Q_FOREACH (const QDBusObjectPath &objectPath, handledChannels) {
        ret << objectPath.path();
    }

    return ret;
}

void TestAccountChannelDispatcher::checkHandlerHandledChannels(ClientHandlerInterface *handler,
        const QStringList &toCompare)
{
    ObjectPathList handledChannels;
    QVERIFY(waitForProperty(handler->requestPropertyHandledChannels(), &handledChannels));
    QStringList sortedHandledChannels;
    Q_FOREACH (const QDBusObjectPath &objectPath, handledChannels) {
        sortedHandledChannels << objectPath.path();
    }
    sortedHandledChannels.sort();
    QCOMPARE(sortedHandledChannels, toCompare);
}

#define TEST_ENSURE_CHANNEL_SPECIFIC(method_name, shouldFail, proceedNoop, expectedError) \
    mChannelDispatcherAdaptor->mInvokeHandler = false; \
    mChannelDispatcherAdaptor->mChannelRequestShouldFail = shouldFail; \
    mChannelDispatcherAdaptor->mChannelRequestProceedNoop = proceedNoop; \
    if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) { \
        mChannelDispatcherAdaptor->setChan(mConnPath, mConnProps, mChanPath, mChanProps); \
    } else { \
        mChannelDispatcherAdaptor->clearChan(); \
    } \
    PendingChannelRequest *pcr = mAccount->method_name(QLatin1String("foo@bar"), \
            mUserActionTime, QString(), mHints); \
    if (shouldFail && proceedNoop) { \
        pcr->cancel(); \
    } \
    testPCR(pcr); \
    QCOMPARE(mChannelRequestFinishedWithError, shouldFail); \
    if (shouldFail) {\
        QCOMPARE(mChannelRequestFinishedErrorName, QString(QLatin1String(expectedError))); \
    }

#define TEST_CREATE_ENSURE_CHANNEL(method_name, shouldFail, proceedNoop, expectedError) \
    QVariantMap request; \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"), \
                                 QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"), \
                                 (uint) Tp::HandleTypeContact); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"), \
                                 QLatin1String("foo@bar")); \
    mChannelDispatcherAdaptor->mInvokeHandler = false; \
    mChannelDispatcherAdaptor->mChannelRequestShouldFail = shouldFail; \
    mChannelDispatcherAdaptor->mChannelRequestProceedNoop = proceedNoop; \
    if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) { \
        mChannelDispatcherAdaptor->setChan(mConnPath, mConnProps, mChanPath, mChanProps); \
    } else { \
        mChannelDispatcherAdaptor->clearChan(); \
    } \
    PendingChannelRequest *pcr = mAccount->method_name(request, \
            mUserActionTime, QString(), mHints); \
    if (shouldFail && proceedNoop) { \
        pcr->cancel(); \
    } \
    testPCR(pcr); \
    QCOMPARE(mChannelRequestFinishedWithError, shouldFail); \
    if (shouldFail) {\
        QCOMPARE(mChannelRequestFinishedErrorName, QString(QLatin1String(expectedError))); \
    }

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
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChat, true, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testEnsureTextChatroom()
{
    mHints.setHint(QLatin1String("uk.co.willthompson"), QLatin1String("MomOrDad"), QString::fromLatin1("Mommy"));
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureTextChatroomFail()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureTextChatroomCancel()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureTextChatroom, true, true, TELEPATHY_ERROR_CANCELLED);
}

void TestAccountChannelDispatcher::testEnsureMediaCall()
{
    mConnPath = QLatin1String("/org/freedesktop/Telepathy/Connection/cmname/proto/account");
    mChanPath = mConnPath + QLatin1String("/channel");
    mChanProps = ChannelClassSpec::streamedMediaCall().allProperties();

    TEST_ENSURE_CHANNEL_SPECIFIC(ensureStreamedMediaCall, false, false, "");
}

void TestAccountChannelDispatcher::testEnsureMediaCallFail()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureStreamedMediaCall, true, false, TELEPATHY_ERROR_NOT_AVAILABLE);
}

void TestAccountChannelDispatcher::testEnsureMediaCallCancel()
{
    TEST_ENSURE_CHANNEL_SPECIFIC(ensureStreamedMediaCall, true, true, TELEPATHY_ERROR_CANCELLED);
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
    TEST_CREATE_ENSURE_CHANNEL(createChannel, true, true, TELEPATHY_ERROR_CANCELLED);
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
    TEST_CREATE_ENSURE_CHANNEL(ensureChannel, true, true, TELEPATHY_ERROR_CANCELLED);
}

#define TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(method_name, channelRequestShouldFail, shouldFail, invokeHandler, expectedError, channelOut, pcOut) \
  { \
    QVariantMap request; \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"), \
                                 QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT)); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"), \
                                 (uint) Tp::HandleTypeContact); \
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetID"), \
                                 QLatin1String("foo@bar")); \
    mChannelDispatcherAdaptor->mInvokeHandler = invokeHandler; \
    mChannelDispatcherAdaptor->mChannelRequestShouldFail = channelRequestShouldFail; \
    mChannelDispatcherAdaptor->mChannelRequestProceedNoop = false; \
    if (!mConnPath.isEmpty() && !mChanPath.isEmpty()) { \
        mChannelDispatcherAdaptor->setChan(mConnPath, mConnProps, mChanPath, mChanProps); \
    } else { \
        mChannelDispatcherAdaptor->clearChan(); \
    } \
    PendingChannel *pc = mAccount->method_name(request, mUserActionTime); \
    testPC(pc, pcOut, channelOut); \
    QCOMPARE(mChannelRequestAndHandleFinishedWithError, shouldFail); \
    if (shouldFail) {\
        QCOMPARE(mChannelRequestAndHandleFinishedErrorName, QString(QLatin1String(expectedError))); \
    } \
  }

void TestAccountChannelDispatcher::testCreateAndHandleChannel()
{
    mConnPath = QLatin1String("/org/freedesktop/Telepathy/Connection/cmname/proto/account");
    mChanPath = mConnPath + QLatin1String("/channel");
    mChanProps = ChannelClassSpec::textChat().allProperties();

    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, false, false, true, "", 0, 0);
}

void TestAccountChannelDispatcher::testCreateAndHandleChannelNotYours()
{
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(ensureAndHandleChannel, false, true, false, TP_QT4_ERROR_NOT_YOURS, 0, 0);
}

void TestAccountChannelDispatcher::testCreateAndHandleChannelFail()
{
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, true, true, false, TP_QT4_ERROR_NOT_AVAILABLE, 0, 0);
}

void TestAccountChannelDispatcher::testCreateAndHandleChannelHandledAgain()
{
    TpTestsContactsConnection *connService = TP_TESTS_CONTACTS_CONNECTION(g_object_new(
            TP_TESTS_TYPE_CONTACTS_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL));
    QVERIFY(connService != 0);
    TpBaseConnection *baseConnService = TP_BASE_CONNECTION(connService);
    QVERIFY(baseConnService != 0);

    gchar *name;
    gchar *connPath;
    GError *error = 0;

    QVERIFY(tp_base_connection_register(baseConnService, "example", &name, &connPath, &error));
    QVERIFY(error == 0);

    QVERIFY(name != 0);
    QVERIFY(connPath != 0);

    QString connName = QLatin1String(name);
    mConnPath = QLatin1String(connPath);

    g_free(name);
    g_free(connPath);

    ConnectionPtr conn = Connection::create(connName, mConnPath,
            ChannelFactory::create(QDBusConnection::sessionBus()),
            ContactFactory::create());
    QCOMPARE(conn->isReady(), false);

    QVERIFY(connect(conn->lowlevel()->requestConnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(conn->isReady(), true);
    QCOMPARE(conn->status(), ConnectionStatusConnected);

    // create a Channel by magic, rather than doing D-Bus round-trips for it

    TpHandleRepoIface *contactRepo = tp_base_connection_get_handles(baseConnService,
            TP_HANDLE_TYPE_CONTACT);
    guint handle = tp_handle_ensure(contactRepo, "someone@localhost", 0, 0);

    mChanPath = mConnPath + QLatin1String("/TextChannel");
    QByteArray chanPath(mChanPath.toAscii());
    ExampleEchoChannel *textChanService = EXAMPLE_ECHO_CHANNEL(g_object_new(
                EXAMPLE_TYPE_ECHO_CHANNEL,
                "connection", connService,
                "object-path", chanPath.data(),
                "handle", handle,
                NULL));

    tp_handle_unref(contactRepo, handle);

    PendingChannel *pcOut = 0;
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, false, false, true, "", 0, &pcOut);

    HandledChannelNotifier *notifier = pcOut->handledChannelNotifier();
    connect(notifier,
            SIGNAL(handledAgain(QDateTime,Tp::ChannelRequestHints)),
            SLOT(onChannelHandledAgain(QDateTime,Tp::ChannelRequestHints)));
    QDateTime timestamp(QDate::currentDate());

    mChannelDispatcherAdaptor->invokeHandler(timestamp.toTime_t());
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mChannelHandledAgainActionTime, timestamp);

    // Disconnect and wait for the readiness change
    QVERIFY(connect(conn->lowlevel()->requestDisconnect(),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(expectSuccessfulCall(Tp::PendingOperation*))));
    QCOMPARE(mLoop->exec(), 0);

    if (conn->isValid()) {
        QVERIFY(connect(conn.data(),
                        SIGNAL(invalidated(Tp::DBusProxy *,
                                           const QString &, const QString &)),
                        mLoop,
                        SLOT(quit())));
        QCOMPARE(mLoop->exec(), 0);
    }

    if (textChanService != 0) {
        g_object_unref(textChanService);
        textChanService = 0;
    }

    if (connService != 0) {
        baseConnService = 0;
        g_object_unref(connService);
        connService = 0;
    }
}

void TestAccountChannelDispatcher::testCreateAndHandleChannelHandledChannels()
{
    mConnPath = QLatin1String("/org/freedesktop/Telepathy/Connection/cmname/proto/account");
    mChanPath = mConnPath + QLatin1String("/channel");
    mChanProps = ChannelClassSpec::textChat().allProperties();

    QVERIFY(ourHandledChannels().isEmpty());
    QVERIFY(ourHandlers().isEmpty());

    ChannelPtr channel;
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, false, false, true, "", &channel, 0);

    // check that the channel appears in the HandledChannels property of the first handler
    QVERIFY(!ourHandledChannels().isEmpty());
    QCOMPARE(ourHandledChannels().size(), 1);
    QVERIFY(ourHandledChannels().contains(mChanPath));

    QVERIFY(!ourHandlers().isEmpty());
    QCOMPARE(ourHandlers().size(), 1);

    channel.reset();

    // reseting the channel should unregister the handler
    while (!ourHandlers().isEmpty()) {
        mLoop->processEvents();
    }

    QVERIFY(ourHandledChannels().isEmpty());

    ChannelPtr channel1;
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, false, false, true, "", &channel1, 0);

    // check that the channel appears in the HandledChannels property of the first handler
    QVERIFY(!ourHandledChannels().isEmpty());
    QCOMPARE(ourHandledChannels().size(), 1);
    QVERIFY(ourHandledChannels().contains(mChanPath));

    QVERIFY(!ourHandlers().isEmpty());
    QCOMPARE(ourHandlers().size(), 1);

    mConnPath = QLatin1String("/org/freedesktop/Telepathy/Connection/cmname/proto/account");
    mChanPath = mConnPath + QLatin1String("/channelother");
    mChanProps = ChannelClassSpec::textChat().allProperties();

    ChannelPtr channel2;
    TEST_CREATE_ENSURE_AND_HANDLE_CHANNEL(createAndHandleChannel, false, false, true, "", &channel2, 0);

    // check that the channel appears in the HandledChannels property of some handler
    QVERIFY(!ourHandledChannels().isEmpty());
    QCOMPARE(ourHandledChannels().size(), 2);
    QVERIFY(ourHandledChannels().contains(mChanPath));

    QVERIFY(!ourHandlers().isEmpty());
    QCOMPARE(ourHandlers().size(), 2);

    QStringList sortedOurHandledChannels = ourHandledChannels();
    sortedOurHandledChannels.sort();
    Q_FOREACH (ClientHandlerInterface *handler, ourHandlers()) {
        checkHandlerHandledChannels(handler, sortedOurHandledChannels);
    }

    channel1.reset();

    while (ourHandlers().size() != 1) {
        mLoop->processEvents();
    }

    QVERIFY(!ourHandledChannels().isEmpty());
    QCOMPARE(ourHandledChannels().size(), 1);
    QVERIFY(ourHandledChannels().contains(mChanPath));

    channel2.reset();

    while (!ourHandlers().isEmpty()) {
        mLoop->processEvents();
    }

    QVERIFY(ourHandledChannels().isEmpty());
}

void TestAccountChannelDispatcher::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

void TestAccountChannelDispatcher::cleanup()
{
    cleanupImpl();
}

QTEST_MAIN(TestAccountChannelDispatcher)
#include "_gen/account-channel-dispatcher.cpp.moc.hpp"
