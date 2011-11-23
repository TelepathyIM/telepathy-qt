/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt/Debug>
#include <TelepathyQt/Types>

#include <QApplication>
#include <QDebug>
#include <QtGui>

#include "roster-window.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (argc < 2) {
        qDebug() << "usage:" << argv[0] << "<account name, as in mc-tool list>";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    QString accountPath = QLatin1String(argv[1]);
    RosterWindow w(accountPath);
    w.show();

    return app.exec();
}
