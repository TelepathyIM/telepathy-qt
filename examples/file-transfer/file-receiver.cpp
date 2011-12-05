/*
 * This file is part of TelepathyQt
 *
 * Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2011 Nokia Corporation
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

#include "file-receiver.h"

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/Debug>
#include <TelepathyQt/IncomingFileTransferChannel>

#include <QDebug>

FileReceiver::FileReceiver(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus(QDBusConnection::sessionBus());

    AccountFactoryPtr accountFactory = AccountFactory::create(bus);
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(bus);
    ChannelFactoryPtr channelFactory = ChannelFactory::create(bus);
    channelFactory->addCommonFeatures(Channel::FeatureCore);
    channelFactory->addFeaturesForIncomingFileTransfers(IncomingFileTransferChannel::FeatureCore);

    mCR = ClientRegistrar::create(accountFactory, connectionFactory,
            channelFactory);

    qDebug() << "Registering incoming file transfer handler";
    mHandler = FileReceiverHandler::create();
    QString handlerName(QLatin1String("TpQtExampleFileReceiverHandler"));
    if (!mCR->registerClient(AbstractClientPtr::dynamicCast(mHandler), handlerName)) {
        qWarning() << "Unable to register incoming file transfer handler, aborting";
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "Awaiting file transfers";
}

FileReceiver::~FileReceiver()
{
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(true);

    new FileReceiver(&app);

    return app.exec();
}

#include "_gen/file-receiver.moc.hpp"
