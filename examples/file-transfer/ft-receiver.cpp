/*
 * This file is part of TelepathyQt4
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

#include "ft-receiver.h"

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/IncomingFileTransferChannel>

#include <QDebug>

FTReceiver::FTReceiver(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus(QDBusConnection::sessionBus());

    AccountFactoryPtr accountFactory = AccountFactory::create(bus, Account::FeatureCore);
    // We only care about CONNECTED connections, so let's specify that in a Connection Factory
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(bus,
            Connection::FeatureCore | Connection::FeatureConnected);
    ChannelFactoryPtr channelFactory = ChannelFactory::create(bus);
    channelFactory->addCommonFeatures(Channel::FeatureCore);
    channelFactory->addFeaturesForIncomingFileTransfers(IncomingFileTransferChannel::FeatureCore);
    ContactFactoryPtr contactFactory = ContactFactory::create();

    mCR = ClientRegistrar::create(bus, accountFactory, connectionFactory,
            channelFactory, contactFactory);

    qDebug() << "Registering incoming file transfer handler";
    ChannelClassSpecList channelFilter;
    channelFilter.append(ChannelClassSpec::incomingFileTransfer());
    mHandler = FTReceiverHandler::create(channelFilter);
    QString handlerName(QLatin1String("TpQt4ExampleFTReceiverHandler"));
    if (!mCR->registerClient(AbstractClientPtr::dynamicCast(mHandler), handlerName)) {
        qWarning() << "Unable to register incoming file transfer handler, aborting";
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "Awaiting file transfers";
}

FTReceiver::~FTReceiver()
{
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(true);

    new FTReceiver(&app);

    return app.exec();
}

#include "_gen/ft-receiver.moc.hpp"
