/*
 * This file is part of TelepathyQt4
 *
 * Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright © 2009 Nokia Corporation
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

#include "call-window.h"
#include "_gen/call-window.moc.hpp"

#include "call-handler.h"
#include "call-roster-widget.h"

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/ConnectionManager>
#include <TelepathyQt4/Client/PendingConnection>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/StreamedMediaChannel>

#include <QDebug>

using namespace Telepathy::Client;

CallWindow::CallWindow(const QString &username, const QString &password,
        QWidget *parent)
    : QMainWindow(parent),
      mUsername(username),
      mPassword(password)
{
    setWindowTitle("Call");

    mCM = ConnectionManager::create("gabble");
    connect(mCM->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onCMReady(Telepathy::Client::PendingOperation *)));

    mCallHandler = new CallHandler(this);

    setupGui();

    resize(240, 320);
}

CallWindow::~CallWindow()
{
    if (mConn) {
        mConn->requestDisconnect();
    }
}

void CallWindow::setupGui()
{
    mRoster = new CallRosterWidget(mCallHandler);
    setCentralWidget(mRoster);
}

void CallWindow::onCMReady(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CM cannot become ready";
        return;
    }

    qDebug() << "CM ready";
    QVariantMap params;
    params.insert("account", QVariant(mUsername));
    params.insert("password", QVariant(mPassword));
    PendingConnection *pconn = mCM->requestConnection("jabber", params);
    connect(pconn,
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onConnectionCreated(Telepathy::Client::PendingOperation *)));
}

void CallWindow::onConnectionCreated(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Unable to create connection";
        return;
    }

    qDebug() << "Connection created";
    PendingConnection *pconn =
        qobject_cast<PendingConnection *>(op);
    ConnectionPtr conn = pconn->connection();
    mConn = conn;
    connect(conn->requestConnect(Connection::FeatureSelfContact),
            SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            SLOT(onConnectionConnected(Telepathy::Client::PendingOperation *)));
    connect(conn.data(),
            SIGNAL(invalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)),
            SLOT(onConnectionInvalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)));
}

void CallWindow::onConnectionConnected(Telepathy::Client::PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "Connection cannot become connected";
        return;
    }

    PendingReady *pr = qobject_cast<PendingReady *>(op);
    ConnectionPtr conn = ConnectionPtr(qobject_cast<Connection *>(pr->object()));

    if (conn->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_CAPABILITIES)) {
        Telepathy::CapabilityPair capability = {
            TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA,
            Telepathy::ChannelMediaCapabilityAudio |
            Telepathy::ChannelMediaCapabilityVideo |
                Telepathy::ChannelMediaCapabilityNATTraversalSTUN |
                Telepathy::ChannelMediaCapabilityNATTraversalGTalkP2P
        };
        qDebug() << "CallWindow::onConnectionConnected: advertising capabilities";
        conn->capabilitiesInterface()->AdvertiseCapabilities(
                Telepathy::CapabilityPairList() << capability,
                QStringList());
    }

    if (conn->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_REQUESTS)) {
        qDebug() << "CallWindow::onConnectionConnected: connecting to Connection.Interface.NewChannels";
        connect(conn->requestsInterface(),
                SIGNAL(NewChannels(const Telepathy::ChannelDetailsList&)),
                SLOT(onNewChannels(const Telepathy::ChannelDetailsList&)));
    }

    mRoster->addConnection(conn);
}

void CallWindow::onConnectionInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "CallWindow::onConnectionInvalidated: connection became invalid:" <<
        errorName << "-" << errorMessage;
    mRoster->removeConnection(mConn);
    mConn.reset();
}

void CallWindow::onNewChannels(const Telepathy::ChannelDetailsList &channels)
{
    qDebug() << "CallWindow::onNewChannels";
    foreach (const Telepathy::ChannelDetails &details, channels) {
        QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
        bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
        qDebug() << " channelType:" << channelType;
        qDebug() << " requested  :" << requested;

        if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA &&
            !requested) {
            StreamedMediaChannelPtr channel = StreamedMediaChannel::create(mConn,
                        details.channel.path(),
                        details.properties);
            mCallHandler->addIncomingCall(channel);
        }
    }
}
