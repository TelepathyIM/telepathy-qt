#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

extern "C" int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected()) {
        qFatal("Session bus not available");
        return 1;
    }

    QTimer::singleShot(0, &app, SLOT(quit()));

    return app.exec();
}
