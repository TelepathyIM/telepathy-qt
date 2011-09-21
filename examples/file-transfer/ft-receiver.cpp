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

#include <TelepathyQt4/Account>
#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/ChannelFactory>
#include <TelepathyQt4/ClientRegistrar>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionFactory>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/IncomingFileTransferChannel>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <QDebug>

FTReceiver::FTReceiver(const QString &accountName, QObject *parent)
    : QObject(parent),
      mAccountName(accountName)
{
    qDebug() << "Retrieving account from AccountManager";

    QDBusConnection bus(QDBusConnection::sessionBus());

    AccountFactoryPtr accountFactory = AccountFactory::create(bus, Account::FeatureCore);
    // We only care about CONNECTED connections, so let's specify that in a Connection Factory
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(bus,
            Connection::FeatureCore | Connection::FeatureConnected);
    ChannelFactoryPtr channelFactory = ChannelFactory::create(bus);
    channelFactory->addCommonFeatures(Channel::FeatureCore);
    channelFactory->addFeaturesForIncomingFileTransfers(IncomingFileTransferChannel::FeatureCore);
    ContactFactoryPtr contactFactory = ContactFactory::create();

    mAM = AccountManager::create(bus, accountFactory, connectionFactory,
            channelFactory, contactFactory);
    connect(mAM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAMReady(Tp::PendingOperation*)));
}

FTReceiver::~FTReceiver()
{
}

void FTReceiver::onAMReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "AccountManager cannot become ready -" <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingReady *pr = qobject_cast<PendingReady*>(op);
    Q_ASSERT(pr != NULL);
    qDebug() << "AccountManager ready";

    mAccount = mAM->accountForPath(
            TP_QT4_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + mAccountName);
    if (!mAccount) {
        qWarning() << "The account given does not exist";
        QCoreApplication::exit(1);
    }
    Q_ASSERT(mAccount->isReady());

    mCR = ClientRegistrar::create(mAM);

    qDebug() << "Registering incoming file transfer handler";
    ChannelClassSpecList channelFilter;
    channelFilter.append(ChannelClassSpec::incomingFileTransfer());
    mHandler = FTReceiverHandler::create(channelFilter, mAccount);
    QString handlerName(QLatin1String("TpQt4ExampleFTReceiverHandler"));
    if (!mCR->registerClient(AbstractClientPtr::dynamicCast(mHandler), handlerName)) {
        qWarning() << "Unable to register incoming file transfer handler, aborting";
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "Checking if account is online...";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    onAccountConnectionChanged(mAccount->connection());
}

void FTReceiver::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (!conn) {
        qDebug() << "The account given has no connection. "
            "Please set it online to be able to receive file transfers";
        return;
    }

    Q_ASSERT(conn->isValid());
    Q_ASSERT(conn->status() == ConnectionStatusConnected);

    qDebug() << "Account online, awaiting file transfers!";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 2) {
        qDebug() << "usage:" << argv[0] << "<account name, as in mc-tool list>";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(true);

    new FTReceiver(QLatin1String(argv[1]), &app);

    return app.exec();
}

#include "_gen/ft-receiver.moc.hpp"
