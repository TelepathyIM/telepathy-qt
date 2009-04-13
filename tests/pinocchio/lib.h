#include <QtCore/QProcess>

#include <QtDBus/QtDBus>

#include <QtTest/QtTest>

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Constants>

#include "tests/lib/test.h"

class PinocchioTest : public Test
{
    Q_OBJECT

public:

    PinocchioTest(QObject *parent = 0);

    virtual ~PinocchioTest();

    static inline QLatin1String pinocchioBusName()
    {
        return QLatin1String(
            TELEPATHY_CONNECTION_MANAGER_BUS_NAME_BASE "pinocchio");
    }

    static inline QLatin1String pinocchioObjectPath()
    {
        return QLatin1String(
            TELEPATHY_CONNECTION_MANAGER_OBJECT_PATH_BASE "pinocchio");
    }

    bool waitForPinocchio(uint timeoutMs = 5000);

protected:
    QString mPinocchioPath;
    QString mPinocchioCtlPath;
    QProcess mPinocchio;

    virtual void initTestCaseImpl();

    virtual void cleanupTestCaseImpl();

protected Q_SLOTS:
    void gotNameOwner(QDBusPendingCallWatcher* watcher);
    void onNameOwnerChanged(const QString&, const QString&, const QString&);
};
