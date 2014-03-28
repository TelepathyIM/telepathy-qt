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

#include "file-sender.h"

#include "pending-file-send.h"

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ChannelClassSpecList>
#include <TelepathyQt/ChannelFactory>
#include <TelepathyQt/ClientRegistrar>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionFactory>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Debug>
#include <TelepathyQt/OutgoingFileTransferChannel>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingChannelRequest>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>

#include <QDebug>

FileSender::FileSender(const QString &accountName, const QString &receiver,
        const QString &filePath, QObject *parent)
    : QObject(parent),
      mAccountName(accountName),
      mReceiver(receiver),
      mFilePath(filePath),
      mTransferRequested(false)
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

FileSender::~FileSender()
{
}

void FileSender::onAMReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "AccountManager cannot become ready -" <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingReady *pr = qobject_cast<PendingReady*>(op);
    Q_ASSERT(pr != NULL);
    Q_UNUSED(pr);
    qDebug() << "AccountManager ready";

    mAccount = mAM->accountForObjectPath(
            TP_QT_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + mAccountName);
    if (!mAccount) {
        qWarning() << "The account given does not exist";
        QCoreApplication::exit(1);
    }
    Q_ASSERT(!mAccount->isReady());
    connect(mAccount->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountReady(Tp::PendingOperation*)));
}

void FileSender::onAccountReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Account cannot become ready -" <<
            op->errorName() << '-' << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    PendingReady *pr = qobject_cast<PendingReady*>(op);
    Q_ASSERT(pr != NULL);
    Q_UNUSED(pr);
    qDebug() << "Account ready";

    qDebug() << "Checking if account is online...";
    connect(mAccount.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
    onAccountConnectionChanged(mAccount->connection());
}

void FileSender::onAccountConnectionChanged(const ConnectionPtr &conn)
{
    if (!conn) {
        qDebug() << "The account given has no connection. "
            "Please set it online to continue";
        return;
    }

    Q_ASSERT(conn->isValid());
    Q_ASSERT(conn->status() == ConnectionStatusConnected);

    qDebug() << "Account online, got a connected connection!";
    mConnection = conn;

    qDebug() << "Creating contact object for receiver" << mReceiver;
    connect(mConnection->contactManager()->contactsForIdentifiers(QStringList() << mReceiver,
                Contact::FeatureCapabilities),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void FileSender::onContactRetrieved(PendingOperation *op)
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

void FileSender::onContactCapabilitiesChanged()
{
    if (mTransferRequested) {
        return;
    }

    if (mContact->capabilities().fileTransfers()) {
        qDebug() << "The remote contact is capable of receiving file transfers. "
            "Requesting file transfer channel";

        mTransferRequested = true;
        FileTransferChannelCreationProperties ftProps(mFilePath,
                QLatin1String("application/octet-stream"));
        connect(mAccount->createAndHandleFileTransfer(mContact, ftProps),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onTransferRequestFinished(Tp::PendingOperation*)));
    }
}

void FileSender::onTransferRequestFinished(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to request stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        QCoreApplication::exit(1);
        return;
    }

    qDebug() << "File transfer channel request finished successfully!";

    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    Q_ASSERT(pc);

    ChannelPtr chan = pc->channel();
    if (!chan->isValid()) {
        qWarning() << "Channel received to handle is invalid, aborting file transfer";
        QCoreApplication::exit(1);
        return;
    }

    // We should always receive outgoing channels of type FileTransfer, as requested,
    // otherwise either MC or tp-qt itself is bogus, so let's assert in case they are
    Q_ASSERT(chan->channelType() == TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    Q_ASSERT(chan->isRequested());

    OutgoingFileTransferChannelPtr transferChannel = OutgoingFileTransferChannelPtr::qObjectCast(chan);
    Q_ASSERT(transferChannel);

    // We just passed the URI when requesting the channel, so it has to be set
    Q_ASSERT(!transferChannel->uri().isEmpty());

    PendingFileSend *sendOperation = new PendingFileSend(transferChannel, SharedPtr<RefCounted>());
    connect(sendOperation,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onSendFinished(Tp::PendingOperation*)));
}

void FileSender::onSendFinished(PendingOperation *op)
{
    PendingFileSend *sendOperation = qobject_cast<PendingFileSend*>(op);
    qDebug() << "Closing channel";
    sendOperation->channel()->requestClose();

    QCoreApplication::exit(0);
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

    new FileSender(QLatin1String(argv[1]), QLatin1String(argv[2]), filePath, &app);

    return app.exec();
}

#include "_gen/file-sender.moc.hpp"
