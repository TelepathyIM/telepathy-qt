#include <QtDBus>

#include <QtTest>

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingVariant>
#include <TelepathyQt4/Constants>

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
    if (mLoop->exec() == 0) {
        *value = qdbus_cast<T>(mPropertyValue);
        return true;
    }
    else {
        *value = T();
        return false;
    }
}
