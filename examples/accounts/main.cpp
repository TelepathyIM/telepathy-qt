#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QtGui>

#include "accounts-window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    AccountsWindow w;
    w.show();

    return app.exec();
}
