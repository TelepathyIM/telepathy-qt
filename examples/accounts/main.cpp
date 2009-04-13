#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QtGui>

#include "accounts-window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    AccountsWindow w;
    w.show();

    return app.exec();
}
