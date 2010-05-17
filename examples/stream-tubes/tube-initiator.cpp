/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "tube-initiator.h"

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
#include <TelepathyQt4/StreamTubeChannel>
#include <TelepathyQt4/OutgoingStreamTubeChannel>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <TelepathyQt4/Presence>

TubeInitiator::TubeInitiator(const QString &username, const QString &password,
        const QString &receiver)
    : mUsername(username),
      mPassword(password),
      mReceiver(receiver),
      mTubeOffered(false)
{
    mServer = new QTcpServer(this);
    connect(mServer, SIGNAL(newConnection()), this, SLOT(onTcpServerNewConnection()));
    mServer->listen();
    mCM = ConnectionManager::create(QLatin1String("gabble"));
    connect(mCM->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onCMReady(Tp::PendingOperation *)));
}

TubeInitiator::~TubeInitiator()
{
}

void TubeInitiator::onCMReady(PendingOperation *op)
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

void TubeInitiator::onConnectionCreated(PendingOperation *op)
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

void TubeInitiator::onConnectionConnected(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Connection cannot become connected -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Connected!";

    qDebug() << "Creating contact object for receiver" << mReceiver;
    connect(mConn->contactManager()->contactsForIdentifiers(QStringList() << mReceiver,
                Features(Contact::FeatureSimplePresence)),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onContactRetrieved(Tp::PendingOperation *)));
}

void TubeInitiator::onContactRetrieved(PendingOperation *op)
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

void TubeInitiator::onContactPresenceChanged()
{
    if (mTubeOffered) {
        return;
    }

    if (mContact->presence().type() != ConnectionPresenceTypeUnset &&
        mContact->presence().type() != ConnectionPresenceTypeOffline &&
        mContact->presence().type() != ConnectionPresenceTypeUnknown &&
        mContact->presence().type() != ConnectionPresenceTypeError) {
        qDebug() << "Contact online!";

        // FIXME this is a workaround as we don't support contact capabilities yet
        sleep(5);
        createStreamTubeChannel();

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
void TubeInitiator::onCapabilitiesChanged(const CapabilityChangeList &caps)
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

void TubeInitiator::gotContactCapabilities(QDBusPendingCallWatcher *watcher)
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

void TubeInitiator::createStreamTubeChannel()
{
    mTubeOffered = true;

    qDebug() << "Creating stream tube channel...";
    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE));
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   (uint) Tp::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   mContact->handle()[0]);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAM_TUBE ".Service"),
                   QLatin1String("rsync"));
    qDebug() << "Request:" << request;
    connect(mConn->lowlevel()->createChannel(request),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onStreamTubeChannelCreated(Tp::PendingOperation*)));
}

void TubeInitiator::onStreamTubeChannelCreated(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Stream tube channel created!";
    PendingChannel *pc = qobject_cast<PendingChannel*>(op);
    mChan = OutgoingStreamTubeChannelPtr::dynamicCast(pc->channel());
    connect(mChan.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onInvalidated()));
    Features features = Features() << TubeChannel::FeatureTube
                                   << StreamTubeChannel::FeatureStreamTube
                                   << StreamTubeChannel::FeatureConnectionMonitoring;
    connect(mChan->becomeReady(features),
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(onStreamTubeChannelReady(Tp::PendingOperation *)));
}

void TubeInitiator::onStreamTubeChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to make stream tube channel ready -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Stream tube channel ready!";
    connect(mChan.data(),
            SIGNAL(newRemoteConnection(Tp::ContactPtr,QVariant,uint)),
            SLOT(onStreamTubeChannelNewRemoteConnection(Tp::ContactPtr,QVariant,uint)));
    connect(mChan->offerTubeAsTcpSocket(mServer, QVariantMap()),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onOfferTubeFinished(Tp::PendingOperation*)));
}

void TubeInitiator::onOfferTubeFinished(PendingOperation* op)
{
    if (op->isError()) {
        qWarning() << "Unable to open stream tube channel -" <<
            op->errorName() << ": " << op->errorMessage();
        return;
    }

    qDebug() << "Stream tube channel opened!";
}

void TubeInitiator::onStreamTubeChannelNewRemoteConnection(
        const Tp::ContactPtr &handle, const QVariant &parameter, uint connectionId)
{
    qDebug() << "New remote connection from "<< handle << parameter << connectionId;
}

void TubeInitiator::onTcpServerNewConnection()
{
    qDebug() << "Pending connection found";
    QTcpSocket *socket = mServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(onDataFromSocket()));
}

void TubeInitiator::onDataFromSocket()
{
    QIODevice *source = qobject_cast<QIODevice*>(sender());
    QString data = QLatin1String(source->readLine());
    data.remove(QLatin1Char('\n'));
    qDebug() << "New data from socket: " << data;
    if (data == QLatin1String("Hi there!!")) {
        source->write(QByteArray("Hey back mate.\n"));
    } else {
        source->write(QByteArray("Sorry, I have no time for you right now.\n"));
    }
}

void TubeInitiator::onInvalidated()
{
    QCoreApplication::quit();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 4) {
        qDebug() << "usage: sender username password receiver";
        return 1;
    }

    Tp::registerTypes();
    Tp::enableDebug(true);

    new TubeInitiator(QLatin1String(argv[1]), QLatin1String(argv[2]),
            QLatin1String(argv[3]));

    return app.exec();
}

#include "_gen/tube-initiator.moc.hpp"
