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

#include "ft-sender.h"

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
#include <TelepathyQt4/ContactCapabilities>
#include <TelepathyQt4/ContactFactory>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/Debug>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/PendingChannelRequest>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <QDebug>

FTSender::FTSender(const QString &accountName, const QString &receiver,
        const QString &filePath, QObject *parent)
    : QObject(parent),
      mAccountName(accountName),
      mReceiver(receiver),
      mFilePath(filePath),
      mFTRequested(false)
{
    qDebug() << "Retrieving account from AccountManager";

    QDBusConnection bus(QDBusConnection::sessionBus());

    // Let's not prepare any account feature as we only care about one account, thus no need to
    // prepare features for all accounts
    AccountFactoryPtr accountFactory = AccountFactory::create(bus);
    // We only care about CONNECTED connections, so let's specify that in a Connection Factory
    ConnectionFactoryPtr connectionFactory = ConnectionFactory::create(bus,
            Connection::FeatureCore | Connection::FeatureConnected);
    ChannelFactoryPtr channelFactory = ChannelFactory::create(bus);
    channelFactory->addCommonFeatures(Channel::FeatureCore);
    channelFactory->addFeaturesForOutgoingFileTransfers(OutgoingFileTransferChannel::FeatureCore);
    ContactFactoryPtr contactFactory = ContactFactory::create();

    mAM = AccountManager::create(bus, accountFactory, connectionFactory,
            channelFactory, contactFactory);
    connect(mAM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAMReady(Tp::PendingOperation*)));
}

FTSender::~FTSender()
{
}

void FTSender::onAMReady(PendingOperation *op)
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
    Q_ASSERT(!mAccount->isReady());
    connect(mAccount->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountReady(Tp::PendingOperation*)));
}

void FTSender::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Account cannot become ready -" <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingReady *pr = qobject_cast<PendingReady*>(op);
    Q_ASSERT(pr != NULL);
    qDebug() << "Account ready";

    mCR = ClientRegistrar::create(mAM);

    qDebug() << "Registering outgoing file transfer handler";
    ChannelClassSpecList channelFilter;
    channelFilter.append(ChannelClassSpec::outgoingFileTransfer());
    mHandler = FTSenderHandler::create(channelFilter);
    QString handlerName(QLatin1String("TpQt4ExampleFTSenderHandler"));
    if (!mCR->registerClient(AbstractClientPtr::dynamicCast(mHandler), handlerName)) {
        qWarning() << "Unable to register outgoing file transfer handler, aborting";
        QCoreApplication::exit(1);
        return;
    }

    mHandlerBusName = QLatin1String("org.freedesktop.Telepathy.Client.") + handlerName;

    qDebug() << "Checking if account is online...";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    onAccountConnectionChanged(mAccount->connection());
}

void FTSender::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (!conn) {
        qDebug() << "The account given has no connection. "
            "Please set it online to continue";
        return;
    }

    Q_ASSERT(conn->isValid());
    Q_ASSERT(conn->status() == ConnectionStatusConnected);

    qDebug() << "Account online, got a connected connection!";
    mConn = conn;

    qDebug() << "Creating contact object for receiver" << mReceiver;
    connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << mReceiver,
                Contact::FeatureCapabilities),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void FTSender::onContactRetrieved(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create contact object for receiver" <<
            mReceiver << "-" << op->errorName() << "-" << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts*>(op);
    Q_ASSERT(pc->contacts().size() == 1);
    mContact = pc->contacts().first();

    qDebug() << "Checking contact capabilities...";
    connect(mContact.data(),
            SIGNAL(capabilitiesChanged(Tp::ContactCapabilities)),
            SLOT(onContactCapabilitiesChanged()));

#if 0
    qDebug() << "Contact capabilities:";
    Q_FOREACH (const RequestableChannelClassSpec &spec, mContact->capabilities().allClassSpecs()) {
        qDebug() << "    fixed:" << spec.fixedProperties();
        qDebug() << "  allowed:" << spec.allowedProperties();
    }
#endif

    if (mContact->capabilities().fileTransfers()) {
        onContactCapabilitiesChanged();
    } else {
        qDebug() << "The receiver needs to be online and support file transfers to continue";
    }
}

void FTSender::onContactCapabilitiesChanged()
{
    if (mFTRequested) {
        return;
    }

    if (mContact->capabilities().fileTransfers()) {
        qDebug() << "The remote contact is capable of receiving file transfers. "
            "Requesting file transfer channel";

        mFTRequested = true;
        FileTransferChannelCreationProperties ftProps(mFilePath,
                QLatin1String("application/octet-stream"));
        connect(mAccount->createFileTransfer(mContact, ftProps,
                    QDateTime::currentDateTime(), mHandlerBusName),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onFTRequestFinished(Tp::PendingOperation*)));
    }
}

void FTSender::onFTRequestFinished(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to request stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "File transfer channel request finished successfully!";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qDebug() << "usage:" << argv[0] << "<account name, as in mc-tool list> <receiver contact ID> <file>";
        return 1;
    }

    QString filePath = QLatin1String(argv[3]);
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "File" << filePath << "does not exist";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(true);

    new FTSender(QLatin1String(argv[1]), QLatin1String(argv[2]), filePath, &app);

    return app.exec();
}

#include "_gen/ft-sender.moc.hpp"
