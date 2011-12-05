/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
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

#include "sender.h"
#include "_gen/sender.moc.hpp"

#include <TelepathyQt/Account>
#include <TelepathyQt/Debug>
#include <TelepathyQt/Constants>
#include <TelepathyQt/ContactMessenger>
#include <TelepathyQt/PendingSendMessage>
#include <TelepathyQt/Types>

#include <QCoreApplication>

Sender::Sender(const QString &accountPath,
        const QString &contactIdentifier, const QString &message)
{
    Tp::AccountPtr acc = Tp::Account::create(TP_QT_ACCOUNT_MANAGER_BUS_NAME,
            accountPath);
    messenger = Tp::ContactMessenger::create(acc, contactIdentifier);
    connect(messenger->sendMessage(message),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onSendMessageFinished(Tp::PendingOperation*)));
}

Sender::~Sender()
{
}

void Sender::onSendMessageFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qDebug() << "Error sending message:" << op->errorName() << "-" << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    Tp::PendingSendMessage *psm = qobject_cast<Tp::PendingSendMessage *>(op);
    qDebug() << "Message sent, token is" << psm->sentMessageToken();
    QCoreApplication::exit(0);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    if (argc < 4) {
        qDebug() << "Usage: contact-messenger account_path contact_id message";
        return -1;
    }

    Sender *sender = new Sender(QLatin1String(argv[1]), QLatin1String(argv[2]),
            QLatin1String(argv[3]));

    int ret = app.exec();
    delete sender;
    return ret;
}
