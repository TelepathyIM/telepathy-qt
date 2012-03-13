#include <tests/lib/test.h>

#include <tests/lib/glib-helpers/test-conn-helper.h>

#include <tests/lib/glib/simple-conn.h>
#include <tests/lib/glib/captcha-chan.h>

#include <TelepathyQt/Captcha>
#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/Connection>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingCaptchas>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/ServerAuthenticationChannel>

#include <telepathy-glib/telepathy-glib.h>

#include <stdio.h>

using namespace Tp;

class TestCaptchaAuthentication : public Test
{
    Q_OBJECT

public:
    TestCaptchaAuthentication(QObject *parent = 0)
        : Test(parent),
          mConn(0), mChanService(0)
    { }

private Q_SLOTS:
    void initTestCase();
    void init();

    void testCreation();
    void testCaptchaSuccessful();
    void testCaptchaRetry();
    void testCaptchaCancel();
    void testNoCaptcha();

    void cleanup();
    void cleanupTestCase();

private:
    void createCaptchaChannel(bool canRetry = false);

    TestConnHelper *mConn;
    TpTestsCaptchaChannel *mChanService;
    ServerAuthenticationChannelPtr mChan;
    CaptchaAuthenticationPtr mCaptcha;
};

void TestCaptchaAuthentication::createCaptchaChannel(bool canRetry)
{
    mChan.reset();
    mLoop->processEvents();
    tp_clear_object(&mChanService);

    /* Create service-side tube channel object */
    QString chanPath = QString(QLatin1String("%1/Channel")).arg(mConn->objectPath());

    mChanService = TP_TESTS_CAPTCHA_CHANNEL(g_object_new(
            TP_TESTS_TYPE_CAPTCHA_CHANNEL,
            "connection", mConn->service(),
            "requested", FALSE,
            "object-path", chanPath.toLatin1().constData(),
            "can-retry-captcha", canRetry,
            NULL));

    /* Create client-side tube channel object */
    mChan = ServerAuthenticationChannel::create(mConn->client(), chanPath, QVariantMap());

    // Verify features shouldn't be Ready
    QVERIFY(mChan->captchaAuthentication().isNull());
    QVERIFY(!mChan->hasCaptchaInterface());
    //QVERIFY(!mChan->hasSaslInterface());

    QVERIFY(connect(mChan->becomeReady(ServerAuthenticationChannel::FeatureCore),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(mChan->isReady(ServerAuthenticationChannel::FeatureCore));
    QVERIFY(mChan->hasCaptchaInterface());
    //QVERIFY(!mChan->hasSaslInterface());

    mCaptcha = mChan->captchaAuthentication();

    QCOMPARE(mCaptcha.isNull(), false);
}

void TestCaptchaAuthentication::initTestCase()
{
    initTestCaseImpl();

    g_type_init();
    g_set_prgname("captcha-authentication");
    tp_debug_set_flags("all");
    dbus_g_bus_get(DBUS_BUS_STARTER, 0);

    mConn = new TestConnHelper(this,
            TP_TESTS_TYPE_SIMPLE_CONNECTION,
            "account", "me@example.com",
            "protocol", "example",
            NULL);
    QCOMPARE(mConn->connect(), true);
}

void TestCaptchaAuthentication::init()
{
    initImpl();
}

void TestCaptchaAuthentication::testCreation()
{
    createCaptchaChannel();

    QCOMPARE(mCaptcha->status(), Tp::CaptchaStatusLocalPending);
    QCOMPARE(mCaptcha->canRetry(), false);
    QVERIFY(mCaptcha->error().isEmpty());
    QVERIFY(mCaptcha->errorDetails().allDetails().isEmpty());
}

void TestCaptchaAuthentication::testCaptchaSuccessful()
{
    createCaptchaChannel();

    QSignalSpy spy(mCaptcha.data(), SIGNAL(statusChanged(Tp::CaptchaStatus)));

    PendingCaptchas *pendingCaptchas = mCaptcha->requestCaptchas(QStringList() << QLatin1String("image/png"));
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(pendingCaptchas->requiresMultipleCaptchas(), false);
    QCOMPARE(pendingCaptchas->captchaList().size(), 1);

    Captcha captchaData = pendingCaptchas->captcha();

    QCOMPARE(captchaData.mimeType(), QLatin1String("image/png"));
    QCOMPARE(captchaData.label(), QLatin1String("Enter the text displayed"));
    QCOMPARE(captchaData.data(), QByteArray("This is a fake payload"));
    QCOMPARE(captchaData.type(), CaptchaAuthentication::OCRChallenge);
    QCOMPARE(captchaData.id(), (uint)42);

    QVERIFY(connect(mCaptcha->answer(42, QLatin1String("This is the right answer")),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(spy.size(), 2);
    QCOMPARE(mCaptcha->status(), CaptchaStatusSucceeded);
}

void TestCaptchaAuthentication::testCaptchaRetry()
{
    createCaptchaChannel(true);
    QSignalSpy spy(mCaptcha.data(), SIGNAL(statusChanged(Tp::CaptchaStatus)));

    PendingCaptchas *pendingCaptchas = mCaptcha->requestCaptchas(QStringList() << QLatin1String("image/png"));
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mCaptcha->answer(42, QLatin1String("What is this I don't even")),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mCaptcha->status(), CaptchaStatusTryAgain);

    pendingCaptchas = mCaptcha->requestCaptchas(QStringList() << QLatin1String("image/png"));
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(pendingCaptchas->requiresMultipleCaptchas(), false);
    QCOMPARE(pendingCaptchas->captchaList().size(), 1);

    Captcha captchaData = pendingCaptchas->captcha();

    QCOMPARE(captchaData.mimeType(), QLatin1String("image/png"));
    QCOMPARE(captchaData.label(), QLatin1String("Enter the text displayed"));
    QCOMPARE(captchaData.data(), QByteArray("This is a reloaded payload"));
    QCOMPARE(captchaData.type(), CaptchaAuthentication::OCRChallenge);
    QCOMPARE(captchaData.id(), (uint)42);

    QVERIFY(connect(mCaptcha->answer(42, QLatin1String("This is the right answer")),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mCaptcha->status(), CaptchaStatusSucceeded);

    // Check signals now
    QCOMPARE(spy.size(), 5);
}

void TestCaptchaAuthentication::testCaptchaCancel()
{
    createCaptchaChannel();

    PendingCaptchas *pendingCaptchas = mCaptcha->requestCaptchas(QStringList() << QLatin1String("image/png"));
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    // Check that the result is not still available
    QCOMPARE(pendingCaptchas->captchaList().size(), 0);
    QCOMPARE(pendingCaptchas->captcha().id(), (uint)0);
    QCOMPARE(mLoop->exec(), 0);

    // Cancel now
    QVERIFY(connect(mCaptcha->cancel(CaptchaCancelReasonUserCancelled),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mCaptcha->status(), CaptchaStatusFailed);

    // Now try performing random actions which should fail
    QVERIFY(connect(mCaptcha->answer(42, QLatin1String("This is the right answer")),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mCaptcha->answer(CaptchaAnswers()),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    QVERIFY(connect(mCaptcha->requestCaptchas(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestCaptchaAuthentication::testNoCaptcha()
{
    createCaptchaChannel();

    PendingCaptchas *pendingCaptchas = mCaptcha->requestCaptchas(QStringList(), CaptchaAuthentication::AudioRecognitionChallenge);
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    pendingCaptchas = mCaptcha->requestCaptchas(QStringList() << QLatin1String("nosuchtype"),
        CaptchaAuthentication::SpeechRecognitionChallenge | CaptchaAuthentication::SpeechQuestionChallenge);
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    // Get the qa one
    pendingCaptchas = mCaptcha->requestCaptchas(QStringList(), CaptchaAuthentication::TextQuestionChallenge);
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);

    Captcha data = pendingCaptchas->captcha();
    Captcha data2(pendingCaptchas->captcha());

    QCOMPARE(data.id(), pendingCaptchas->captcha().id());
    QCOMPARE(data.id(), data2.id());

    // Get the video one to fail utterly
    pendingCaptchas = mCaptcha->requestCaptchas(QStringList(), CaptchaAuthentication::VideoRecognitionChallenge);
    QVERIFY(connect(pendingCaptchas,
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(expectFailure(Tp::PendingOperation *))));
    QCOMPARE(mLoop->exec(), 0);
}

void TestCaptchaAuthentication::cleanup()
{
    cleanupImpl();

    if (mChan && mChan->isValid()) {
        qDebug() << "waiting for the channel to become invalidated";

        QVERIFY(connect(mChan.data(),
                SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
                mLoop,
                SLOT(quit())));
        tp_base_channel_close(TP_BASE_CHANNEL(mChanService));
        QCOMPARE(mLoop->exec(), 0);
    }

    mChan.reset();

    if (mChanService != 0) {
        g_object_unref(mChanService);
        mChanService = 0;
    }

    mLoop->processEvents();
}

void TestCaptchaAuthentication::cleanupTestCase()
{
    QCOMPARE(mConn->disconnect(), true);
    delete mConn;

    cleanupTestCaseImpl();
}

QTEST_MAIN(TestCaptchaAuthentication)
#include "_gen/captcha-authentication.cpp.moc.hpp"
