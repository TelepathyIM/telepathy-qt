/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include "receiver.h"
#include "_gen/receiver.moc.hpp"

#include "receiver-channel.h"

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/FileTransferChannel>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <QDebug>

Receiver::Receiver(const QString &username, const QString &password)
    : mUsername(username),
      mPassword(password)
{
    mCM = ConnectionManager::create("gabble");
    connect(mCM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCMReady(Tp::PendingOperation *)));
}

Receiver::~Receiver()
{
    mFile.close();
}

void Receiver::onCMReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CM cannot become ready -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "CM ready!";

    qDebug() << "Creating connection...";
    QVariantMap params;
    params.insert("account", QVariant(mUsername));
    params.insert("password", QVariant(mPassword));
    PendingConnection *pconn = mCM->requestConnection("jabber", params);
    connect(pconn,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionCreated(Tp::PendingOperation *)));
}

void Receiver::onConnectionCreated(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create connection -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Connection ready!";

    qDebug() << "Connecting...";
    PendingConnection *pconn =
        qobject_cast<PendingConnection *>(op);
    mConn = pconn->connection();
    connect(mConn->requestConnect(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionConnected(Tp::PendingOperation *)));
    connect(mConn.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onInvalidated()));
}

void Receiver::onConnectionConnected(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Connection cannot become connected -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Connected!";

    connect(mConn->requestsInterface(),
            SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
            SLOT(onNewChannels(const Tp::ChannelDetailsList&)));
}

void Receiver::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    foreach (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER &&
            !requested) {
            ReceiverChannel *channel = new ReceiverChannel(mConn,
                    details.channel.path(),
                    details.properties);
            connect(channel,
                    SIGNAL(finished()),
                    channel,
                    SLOT(deleteLater()));
        }
    }
}

void Receiver::onInvalidated()
{
    QCoreApplication::exit(1);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        qDebug() << "usage: receiver username password";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);

    new Receiver(argv[1], argv[2]);

    return app.exec();
}
