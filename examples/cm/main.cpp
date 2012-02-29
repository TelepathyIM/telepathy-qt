#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>

#include <TelepathyQt/BaseConnectionManager>

#include <QDebug>
#include <QtCore>

using namespace Tp;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    BaseConnectionManager *cm = new BaseConnectionManager(
            QDBusConnection::sessionBus(), QLatin1String("TpQtExampleCM"));
    cm->registerObject();

    return app.exec();
}
