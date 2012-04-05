#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>

#include <QDebug>
#include <QtCore>

#include "protocol.h"

using namespace Tp;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    BaseProtocolPtr proto = BaseProtocol::create<Protocol>(
            QDBusConnection::sessionBus(),
            QLatin1String("example-proto"));
    BaseConnectionManagerPtr cm = BaseConnectionManager::create(
            QDBusConnection::sessionBus(), QLatin1String("TpQtExampleCM"));
    cm->addProtocol(proto);
    cm->registerObject();

    return app.exec();
}
