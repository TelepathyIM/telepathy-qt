#include "tests/lib/test-thread-helper.h"
#include <QEventLoop>
#include <QTimer>

TestThreadHelperBase::TestThreadHelperBase(ThreadObjectBase *threadObject)
{
    Q_ASSERT(threadObject);

    mThread = new QThread;
    mThreadObject = threadObject;
    mThreadObject->moveToThread(mThread);

    QEventLoop loop;
    QObject::connect(mThread, SIGNAL(started()), &loop, SLOT(quit()));
    QTimer::singleShot(0, mThread, SLOT(start()));
    loop.exec();
}

TestThreadHelperBase::~TestThreadHelperBase()
{
    QMetaObject::invokeMethod(mThreadObject, "deleteLater");
    mThread->quit();
    mThread->wait();
    mThread->deleteLater();
    QCoreApplication::processEvents();
}

void TestThreadHelperBase::executeCallback()
{
    QEventLoop loop;
    QObject::connect(mThreadObject, SIGNAL(callbackExecutionFinished()),
                     &loop, SLOT(quit()));
    QMetaObject::invokeMethod(mThreadObject, "executeCallback");
    loop.exec();
}

#include "_gen/test-thread-helper.h.moc.hpp"
