#include <QtDBus>

#include <QtTest>

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Constants>

namespace Telepathy
{
namespace Client
{
class DBusProxy;
}
}

class Test : public QObject
{
    Q_OBJECT

public:

    Test(QObject *parent = 0);

    virtual ~Test();

    QEventLoop *mLoop;
    void processDBusQueue(Telepathy::Client::DBusProxy *proxy);

protected Q_SLOTS:
    void expectSuccessfulCall(QDBusPendingCallWatcher*);
    void expectSuccessfulCall(Telepathy::Client::PendingOperation*);

    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();
};
