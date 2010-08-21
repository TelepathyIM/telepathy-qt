#include <QtDBus>

#include <QtTest>

#include <TelepathyQt4/PendingOperation>
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

protected Q_SLOTS:
    void expectSuccessfulCall(QDBusPendingCallWatcher*);
    void expectSuccessfulCall(Tp::PendingOperation*);
    void expectFailure(Tp::PendingOperation*);

    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();
};
