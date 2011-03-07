/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

// FIXME: This example is quite non-exemplary, as it uses requestConnection and createChannel
// directly!
#define TP_QT4_ENABLE_LOWLEVEL_API

#include <TelepathyQt4/Debug>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ConnectionLowlevel>
#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/ConnectionManagerLowlevel>
#include <TelepathyQt4/Constants>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/FileTransferChannel>
#include <TelepathyQt4/OutgoingFileTransferChannel>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Presence>

#include <QDebug>
#include <QFile>
#include <QFileInfo>

Sender::Sender(const QString &username, const QString &password,
        const QString &receiver, const QString &fileName)
    : mUsername(username),
      mPassword(password),
      mReceiver(receiver),
      mFileName(fileName),
      mTransferStarted(false),
      mCompleted(false)
{
    mFile.setFileName(mFileName);
    if (!mFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file for reading";
        return;
    }

    mCM = ConnectionManager::create(QLatin1String("gabble"));
    connect(mCM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCMReady(Tp::PendingOperation *)));
}

Sender::~Sender()
{
    mFile.close();
}

void Sender::onCMReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CM cannot become ready -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "CM ready!";

    qDebug() << "Creating connection...";
    QVariantMap params;
    params.insert(QLatin1String("account"), QVariant(mUsername));
    params.insert(QLatin1String("password"), QVariant(mPassword));
    PendingConnection *pconn = mCM->lowlevel()->requestConnection(QLatin1String("jabber"),
            params);
    connect(pconn,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionCreated(Tp::PendingOperation *)));
}

void Sender::onConnectionCreated(PendingOperation *op)
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
    connect(mConn->lowlevel()->requestConnect(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onConnectionConnected(Tp::PendingOperation *)));
    connect(mConn.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onInvalidated()));
}

void Sender::onConnectionConnected(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Connection cannot become connected -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Connected!";

    qDebug() << "Creating contact object for receiver" << mReceiver;
    connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << mReceiver,
                Features() << Contact::FeatureSimplePresence),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void Sender::onContactRetrieved(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create contact object for receiver" <<
            mReceiver << "-" << op->errorName() << ": " << op->errorMessage();
        return;
    }

    PendingContacts *pc = qobject_cast<PendingContacts *>(op);
    Q_ASSERT(pc->contacts().size() == 1);
    mContact = pc->contacts().first();

    qDebug() << "Checking contact presence...";
    connect(mContact.data(),
            SIGNAL(simplePresenceChanged(const QString &, uint, const QString &)),
            SLOT(onContactPresenceChanged()));
    onContactPresenceChanged();
}

void Sender::onContactPresenceChanged()
{
    if (mTransferStarted) {
        return;
    }

    if (mContact->presence().type() != ConnectionPresenceTypeUnset &&
        mContact->presence().type() != ConnectionPresenceTypeOffline &&
        mContact->presence().type() != ConnectionPresenceTypeUnknown &&
        mContact->presence().type() != ConnectionPresenceTypeError) {
        qDebug() << "Contact online!";

        // FIXME this is a workaround as we don't support contact capabilities yet
        sleep(5);
        createFileTransferChannel();

        /*
        connect(mConn->capabilitiesInterface(),
                SIGNAL(CapabilitiesChanged(const Tp::CapabilityChangeList &)),
                SLOT(onCapabilitiesChanged(const Tp::CapabilityChangeList &)));
        QDBusPendingCallWatcher *watcher =
            new QDBusPendingCallWatcher(
                    mConn->capabilitiesInterface()->GetCapabilities(
                        UIntList() << mContact->handle()[0]),
                    mConn.data());
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(gotContactCapabilities(QDBusPendingCallWatcher *)));
        */
    }
}

/*
void Sender::onCapabilitiesChanged(const CapabilityChangeList &caps)
{
    if (mTransferStarted) {
        return;
    }

    qDebug() << "Capabilities changed";
    foreach (const CapabilityChange &cap, caps) {
        qDebug() << "Checking cap channel type" << cap.channelType;
        if (cap.handle == mContact->handle()[0] &&
            cap.channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER) {
            qDebug() << "Contact supports file transfer!";
            createFileTransferChannel();
            break;
        }
    }
}

void Sender::gotContactCapabilities(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ContactCapabilityList> reply = *watcher;

    if (reply.isError()) {
        qWarning() << "Unable to get contact capabilities, trying "
            "anyway -" << reply.error().name() << ":" <<
            reply.error().message();
        createFileTransferChannel();
    } else {
        ContactCapabilityList caps = reply.value();
        qDebug() << "Got contact capabilities";
        foreach (const ContactCapability &cap, caps) {
            // no need to check the handle, as we only requested the caps for
            // one contact
            qDebug() << "Checking cap channel type" << cap.channelType;
            if (cap.channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER) {
                qDebug() << "Contact supports file transfer!";
                createFileTransferChannel();
                break;
            }
        }
    }
}
*/

void Sender::createFileTransferChannel()
{
    mTransferStarted = true;

    QFileInfo fileInfo(mFileName);
    qDebug() << "Creating file transfer channel...";
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mContact->handle()[0]);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename"),
                   fileInfo.fileName());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size"),
                   (qulonglong) fileInfo.size());
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentType"),
                   QLatin1String("application/octet-stream"));
    qDebug() << "Request:" << request;
    connect(mConn->lowlevel()->createChannel(request),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onFileTransferChannelCreated(Tp::PendingOperation*)));
}

void Sender::onFileTransferChannelCreated(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create file transfer channel -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "File transfer channel created!";
    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = OutgoingFileTransferChannelPtr::qObjectCast(pc->channel());
    connect(mChan.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onInvalidated()));
    connect(mChan->becomeReady(FileTransferChannel::FeatureCore),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onFileTransferChannelReady(Tp::PendingOperation *)));
}

void Sender::onFileTransferChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to make file transfer channel ready -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "File transfer channel ready!";
    connect(mChan.data(),
            SIGNAL(stateChanged(Tp::FileTransferState, Tp::FileTransferStateChangeReason)),
            SLOT(onFileTransferChannelStateChanged(Tp::FileTransferState, Tp::FileTransferStateChangeReason)));
    connect(mChan.data(),
            SIGNAL(transferredBytesChanged(qulonglong)),
            SLOT(onFileTransferChannelTransferredBytesChanged(qulonglong)));
    mChan->provideFile(&mFile);
}

void Sender::onFileTransferChannelStateChanged(Tp::FileTransferState state,
    Tp::FileTransferStateChangeReason stateReason)
{
    qDebug() << "File transfer channel state changed to" << state <<
        "with reason" << stateReason;
    mCompleted = (state == FileTransferStateCompleted);
    if (mCompleted) {
        qDebug() << "Transfer completed!";
        QCoreApplication::exit(0);
    }
}

void Sender::onFileTransferChannelTransferredBytesChanged(qulonglong count)
{
    qDebug().nospace() << "Tranferred bytes " << count << " - " <<
        ((int) (((double) count / mChan->size()) * 100)) << "% done";
}

void Sender::onInvalidated()
{
    QCoreApplication::exit(!mCompleted);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 5) {
        qDebug() << "usage: sender username password receiver filename";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);

    new Sender(QLatin1String(argv[1]), QLatin1String(argv[2]),
            QLatin1String(argv[3]), QLatin1String(argv[4]));

    return app.exec();
}
