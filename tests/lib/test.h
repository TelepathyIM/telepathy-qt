#ifndef _TelepathyQt_tests_lib_test_h_HEADER_GUARD_
#define _TelepathyQt_tests_lib_test_h_HEADER_GUARD_

#include <QtDBus>
#include <QtTest>

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingVariant>
#include <TelepathyQt/Constants>

namespace Tp
{
class DBusProxy;
}

class Test : public QObject
{
    Q_OBJECT

public:

    Test(QObject *parent = 0);

    virtual ~Test();

    QEventLoop *mLoop;
    void processDBusQueue(Tp::DBusProxy *proxy);

    // The last error received in expectFailure()
    QString mLastError;
    QString mLastErrorMessage;

protected:
    template<typename T> bool waitForProperty(Tp::PendingVariant *pv, T *value);

protected Q_SLOTS:
    void expectSuccessfulCall(QDBusPendingCallWatcher*);
    void expectSuccessfulCall(Tp::PendingOperation*);
    void expectFailure(Tp::PendingOperation*);
    void expectSuccessfulProperty(Tp::PendingOperation *op);
    void onWatchdog();

    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();

private:
    // The property retrieved by expectSuccessfulProperty()
    QVariant mPropertyValue;
};

template<typename T>
bool Test::waitForProperty(Tp::PendingVariant *pv, T *value)
{
    connect(pv,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulProperty(Tp::PendingOperation*)));
    if (mLoop->exec() == 1000) {
        *value = qdbus_cast<T>(mPropertyValue);
        return true;
    }
    else {
        *value = T();
        return false;
    }
}

#define TEST_VERIFY_OP(op) \
    if (!op->isFinished()) { \
        qWarning() << "unfinished"; \
        mLoop->exit(1); \
        return; \
    } \
    if (op->isError()) { \
        qWarning().nospace() << op->errorName() << ": " << op->errorMessage(); \
        mLoop->exit(2); \
        return; \
    } \
    if (!op->isValid()) { \
        qWarning() << "inconsistent results"; \
        mLoop->exit(3); \
        return; \
    } \
    qDebug() << "finished";

#endif // _TelepathyQt_tests_lib_test_h_HEADER_GUARD_
