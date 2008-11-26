#include <cstdlib>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

extern "C" int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    char *command = ::getenv("PINOCCHIO");

    if (!command) {
        qFatal("Put $PINOCCHIO in your environment");
        return 1;
    }

    if (!QDBusConnection::sessionBus().isConnected()) {
        qFatal("Session bus not available");
        return 1;
    }

    QProcess pinocchio;

    pinocchio.setProcessChannelMode(QProcess::ForwardedChannels);
    pinocchio.start(command, QStringList());

    if (!pinocchio.waitForStarted(5000)) {
        qFatal("Timed out waiting for pinocchio to start");
        return 1;
    }

    pinocchio.closeWriteChannel();

    /* insert test logic here... */
    QTimer::singleShot(0, &app, SLOT(quit()));
    int ret = app.exec();

    pinocchio.terminate();

    if (!pinocchio.waitForFinished(1000)) {
        pinocchio.kill();
    }

    return ret;
}
