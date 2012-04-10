#ifndef _TelepathyQt_tests_lib_test_thread_helper_h_HEADER_GUARD_
#define _TelepathyQt_tests_lib_test_thread_helper_h_HEADER_GUARD_

#include <QObject>
#include <QThread>
#include <QCoreApplication>
#include <TelepathyQt/Callbacks>

class ThreadObjectBase : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    virtual void executeCallback() = 0;

Q_SIGNALS:
    void callbackExecutionFinished();
};

template <typename Context>
class ThreadObject : public ThreadObjectBase
{
public:
    typedef Tp::Callback1<void, Context&> Callback;
    Callback mCallback;

    virtual void executeCallback()
    {
        Q_ASSERT(mCallback.isValid());
        Q_ASSERT(QThread::currentThread() != QCoreApplication::instance()->thread());

        mCallback(mContext);
        Q_EMIT callbackExecutionFinished();
    }

private:
    Context mContext;
};

class TestThreadHelperBase
{
public:
    virtual ~TestThreadHelperBase();

protected:
    TestThreadHelperBase(ThreadObjectBase *threadObject);
    void executeCallback();

protected:
    QThread *mThread;
    ThreadObjectBase *mThreadObject;
};

template <typename Context>
class TestThreadHelper : public TestThreadHelperBase
{
public:
    TestThreadHelper()
        : TestThreadHelperBase(new ThreadObject<Context>())
    { }

    void executeCallback(typename ThreadObject<Context>::Callback const & cb)
    {
        static_cast<ThreadObject<Context>*>(mThreadObject)->mCallback = cb;
        TestThreadHelperBase::executeCallback();
    }
};

#define TEST_THREAD_HELPER_EXECUTE(helper, callback) \
    do { \
        (helper)->executeCallback(Tp::ptrFun(callback)); \
        if (QTest::currentTestFailed()) { \
            return; \
        } \
    } while(0)

#endif // _TelepathyQt_tests_lib_test_thread_helper_h_HEADER_GUARD_
