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

protected:
    QString mPinocchioPath;
    QString mPinocchioCtlPath;
    QProcess mPinocchio;
    QEventLoop *mLoop;

protected slots:
    virtual void initTestCaseImpl();
    virtual void initImpl();

    virtual void cleanupImpl();
    virtual void cleanupTestCaseImpl();
};
