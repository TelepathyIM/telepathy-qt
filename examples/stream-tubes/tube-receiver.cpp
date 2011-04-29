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

// FIXME: This example is quite non-exemplary, as it uses RequestConnection and the Requests
// low-level interface (!!!) directly!
#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/Account>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/ConnectionManagerLowlevel>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/IncomingStreamTubeChannel>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/PendingStreamTubeConnection>

#include <QDebug>
#include <QIODevice>

TubeReceiver::TubeReceiver(const QString &accountName, QObject *parent)
    : QObject(parent)
{
    // We only care about CONNECTED connections, so let's specify that in a Connection Factory
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(
            QDBusConnection::sessionBus(), Connection::FeatureConnected);

    mAccount = Account::create(TP_QT4_ACCOUNT_MANAGER_BUS_NAME,
            TP_QT4_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + accountName,
            connectionFactory);

    connect(mAccount->becomeReady(Account::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onAccountReady(Tp::PendingOperation *)));
}

TubeReceiver::~TubeReceiver()
{
}

void TubeReceiver::onAccountReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Account cannot become ready - " <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "Account ready";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));

    if (mAccount->connection().isNull()) {
        qDebug() << "The account given has no Connection. Please set it online to continue.";
    } else {
        onAccountConnectionChanged(mAccount->connection());
    }
}

void TubeReceiver::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (!conn) {
        return;
    }

    Q_ASSERT(conn->isValid());
    Q_ASSERT(conn->status() == ConnectionStatusConnected);

    qDebug() << "Got a Connected Connection!";
    mConn = conn;

    QMap<QString, QDBusVariant> filter;
    filter[QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")] =
        QDBusVariant(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE));
    filter[QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType")] =
        QDBusVariant(uint(1));
    HandlerCapabilities capabilities;
    capabilities.channelClasses << Tp::ChannelClass(filter);

    Client::ConnectionInterfaceContactCapabilitiesInterface *contactCapabilitiesInterface =
        mConn->optionalInterface<Client::ConnectionInterfaceContactCapabilitiesInterface>();
    contactCapabilitiesInterface->UpdateCapabilities(HandlerCapabilitiesList() <<
            capabilities);

    connect(mConn->optionalInterface<Client::ConnectionInterfaceRequestsInterface>(),
            SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
            SLOT(onNewChannels(const Tp::ChannelDetailsList&)));
}

void TubeReceiver::onNewChannels(const Tp::ChannelDetailsList &channels)
{
    foreach (const Tp::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL
                                                                     ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE) &&
            !requested) {
            mChan = IncomingStreamTubeChannel::create(mConn,
                    details.channel.path(),
                    details.properties);
            connect(mChan.data(),
                    SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
                    SLOT(onInvalidated()));
            Features features = Features() << TubeChannel::FeatureTube
                                           << StreamTubeChannel::FeatureStreamTube;
            connect(mChan->becomeReady(features),
                    SIGNAL(finished(Tp::PendingOperation *)),
                    SLOT(onStreamTubeChannelReady(Tp::PendingOperation*)));
        }
    }
}

void TubeReceiver::onStreamTubeChannelReady(PendingOperation* op)
{
    if (op->isError()) {
        qWarning() << "Unable to make stream tube channel ready -" <<
            op->errorName() << ": " << op->errorMessage();
        onInvalidated();
        return;
    }

    qDebug() << "Stream tube channel ready!";

    connect(mChan->acceptTubeAsUnixSocket(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamTubeAccepted(Tp::PendingOperation*)));
}

void TubeReceiver::onStreamTubeAccepted(PendingOperation* op)
{
    if (op->isError()) {
        qWarning() << "Unable to accept stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        onInvalidated();
        return;
    }

    qDebug() << "Stream tube channel accepted and opened!";

    mDevice = new QLocalSocket(this);
    mDevice->connectToServer(mChan->localAddress());

    connect(mDevice, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)),
            this, SLOT(onStateChanged(QLocalSocket::LocalSocketState)));
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

void TubeReceiver::onInvalidated()
{
    QCoreApplication::exit(1);
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
