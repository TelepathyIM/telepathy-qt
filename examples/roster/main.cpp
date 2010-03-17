#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/Types>

#include <QDebug>
#include <QtCore>
#include <QtGui>

#include "roster-window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (argc < 3) {
        qDebug() << "usage: roster username password";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    QString username = QLatin1String(argv[1]);
    QString password = QLatin1String(argv[2]);
    RosterWindow w(username, password);
    w.show();

    return app.exec();
}
