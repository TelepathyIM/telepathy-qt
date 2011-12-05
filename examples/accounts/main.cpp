#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

#include <QApplication>
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
