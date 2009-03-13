#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QDebug>
#include <QtGui>

#include "roster-window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (argc < 3) {
        qDebug() << "usage: roster username password";
        return 1;
    }

    Telepathy::registerTypes();
    Telepathy::enableDebug(true);
    Telepathy::enableWarnings(true);

    RosterWindow w(argv[1], argv[2]);
    w.show();

    return app.exec();
}
