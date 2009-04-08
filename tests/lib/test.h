#include <QtDBus>

#include <QtTest>

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Constants>

namespace Telepathy
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
    void processDBusQueue(Telepathy::DBusProxy *proxy);

protected Q_SLOTS:
    void expectSuccessfulCall(QDBusPendingCallWatcher*);
    void expectSuccessfulCall(Telepathy::PendingOperation*);

    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();
};
