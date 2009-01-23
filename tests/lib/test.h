#include <QtDBus>

#include <QtTest>

#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Constants>

class Test : public QObject
{
    Q_OBJECT

public:

    Test(QObject *parent = 0);

    virtual ~Test();

    QEventLoop *mLoop;

protected Q_SLOTS:
    void expectSuccessfulCall(QDBusPendingCallWatcher*);
    void expectSuccessfulCall(Telepathy::Client::PendingOperation*);

    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();
};
