#include <QtCore/QProcess>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

class PinocchioTest : public QObject
{
    Q_OBJECT

public:

    PinocchioTest(QObject *parent = 0)
        : QObject(parent), mLoop(new QEventLoop(this))
    { }

    virtual ~PinocchioTest();

    static inline QLatin1String pinocchioBusName()
    {
        return QLatin1String(
            "org.freedesktop.Telepathy.ConnectionManager.pinocchio");
    }

    static inline QLatin1String pinocchioObjectPath()
    {
        return QLatin1String(
            "/org/freedesktop/Telepathy/ConnectionManager/pinocchio");
    }

    bool waitForPinocchio(uint timeoutMs = 5000);

protected:
    QString mPinocchioPath;
    QString mPinocchioCtlPath;
    QProcess mPinocchio;
    QEventLoop *mLoop;

protected Q_SLOTS:
    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();

    void gotNameOwner(QDBusPendingCallWatcher* watcher);
    void onNameOwnerChanged(const QString&, const QString&, const QString&);
};
