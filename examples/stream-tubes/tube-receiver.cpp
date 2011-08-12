/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "tube-receiver.h"

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/StreamTubeClient>

#include <QDebug>
#include <QIODevice>
#include <QStringList>

TubeReceiver::TubeReceiver(const QString &accountName, QObject *parent)
    : QObject(parent)
{
    mTubeClient = StreamTubeClient::create(QStringList() << QLatin1String("tp-qt4-stube-example"));
    connect(mTubeClient.data(),
            SIGNAL(tubeAcceptedAsUnix(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString,
                    bool,uchar)),
            SLOT(onTubeAccepted(Tp::AccountPtr,Tp::IncomingStreamTubeChannelPtr,QString)));
    mTubeClient->setToAcceptAsUnix(false); // no SCM_CREDENTIALS required
}

TubeReceiver::~TubeReceiver()
{
}

void TubeReceiver::onTubeAccepted(const Tp::AccountPtr &acc,
        const Tp::IncomingStreamTubeChannelPtr &tube, const QString &listenAddress)
{
    qDebug() << "Stream tube channel" << tube->objectPath() << "accepted and opened!";

    mDevice = new QLocalSocket(this);
    mDevice->connectToServer(listenAddress);

    connect(mDevice, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)),
            this, SLOT(onStateChanged(QLocalSocket::LocalSocketState)));
    onStateChanged(mDevice->state());
}

void TubeReceiver::onStateChanged(QLocalSocket::LocalSocketState state)
{
    if (state == QLocalSocket::ConnectedState) {
        qDebug() << "Local socket connected and ready";
        connect(mDevice, SIGNAL(readyRead()), this, SLOT(onDataFromSocket()));
        mDevice->write(QByteArray("Hi there!!\n"));

        // Throw in some stuff
        QTimer *timer = new QTimer(this);
        timer->setInterval(2000);
        connect(timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
        timer->start();
    } else {
        qDebug() << "Socket in state " << state;
    }
}

void TubeReceiver::onDataFromSocket()
{
    QIODevice *source = qobject_cast<QIODevice*>(sender());
    QString data = QLatin1String(source->readLine());
    data.remove(QLatin1Char('\n'));
    qDebug() << "New data from socket: " << data;
}

void TubeReceiver::onTimerTimeout()
{
    mDevice->write(QByteArray("ping, I'm alive\n"));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 2) {
        qDebug() << "usage:" << argv[0] << "<account name, as in mc-tool list>";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);

    new TubeReceiver(QLatin1String(argv[1]), &app);

    return app.exec();
}

#include "_gen/tube-receiver.moc.hpp"
