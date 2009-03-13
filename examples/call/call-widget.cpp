/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "call-widget.h"
#include "_gen/call-widget.moc.hpp"

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/StreamedMediaChannel>

#include <QAction>
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QVariantMap>

using namespace Telepathy::Client;

CallWidget::CallWidget(const QSharedPointer<Contact> &contact,
        QWidget *parent)
    : QWidget(parent),
      mContact(contact)
{
    setWindowTitle(QString("Call (%1)").arg(mContact->id()));

    setAttribute(Qt::WA_DeleteOnClose);

    setupGui();

    QVariantMap request;
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"),
                   TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"),
                   Telepathy::HandleTypeContact);
    request.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"),
                   contact->handle()[0]);

    Connection *conn = contact->manager()->connection();
    connect(conn->ensureChannel(request),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onChannelCreated(Telepathy::Client::PendingOperation*)));
}

CallWidget::CallWidget(const ChannelPtr &chan,
        QWidget *parent)
    : QWidget(parent),
      mChan(chan)
{
    setWindowTitle(QString("Call"));

    setAttribute(Qt::WA_DeleteOnClose);

    setupGui();

    connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onChannelReady(Telepathy::Client::PendingOperation*)));
    connect(mChan.data(),
            SIGNAL(invalidated(DBusProxy *, const QString &, const QString &)),
            SLOT(onChannelInvalidated(DBusProxy *, const QString &, const QString &)));
}

CallWidget::~CallWidget()
{
    if (mChan) {
        mChan->requestClose();
    }
}

void CallWidget::setupGui()
{
    QHBoxLayout *mainBox = new QHBoxLayout;

    mStack = new QStackedWidget();

    mLblStatus = new QLabel("Initiating...");
    mLblStatus->setAlignment(Qt::AlignCenter);
    mStack->addWidget(mLblStatus);

    QFrame *frame = new QFrame;

    QHBoxLayout *hbox = new QHBoxLayout;

    mBtnAccept = new QPushButton("Call");
    mBtnAccept->setEnabled(false);
    connect(mBtnAccept,
            SIGNAL(clicked(bool)),
            SLOT(onBtnAcceptClicked()));
    hbox->addWidget(mBtnAccept);
    mBtnReject = new QPushButton("Terminate");
    mBtnReject->setEnabled(false);
    connect(mBtnReject,
            SIGNAL(clicked(bool)),
            SLOT(onBtnRejectClicked()));
    hbox->addWidget(mBtnReject);

    frame->setLayout(hbox);

    mStack->addWidget(frame);

    mStack->setCurrentIndex(0);

    mainBox->addWidget(mStack);

    setLayout(mainBox);
}

void CallWidget::onChannelCreated(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CallWidget::onChannelCreated: channel cannot be created:" <<
            op->errorName() << "-" << op->errorMessage();
        mLblStatus->setText("Unable to establish call");
        mStack->setCurrentIndex(0);
        return;
    }

    PendingChannel *pc = qobject_cast<PendingChannel *>(op);

    mChan = pc->channel();

    connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onChannelReady(Telepathy::Client::PendingOperation*)));
}

void CallWidget::onChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CallWidget::onChannelReady: channel cannot become ready:" <<
            op->errorName() << "-" << op->errorMessage();
        mLblStatus->setText("Unable to establish call");
        mStack->setCurrentIndex(0);
        return;
    }

    mStack->setCurrentIndex(1);

    StreamedMediaChannel *streamChan =
        qobject_cast<StreamedMediaChannel *>(mChan.data());
    if (streamChan->awaitingLocalAnswer()) {
        mBtnAccept->setText("Accept");
        mBtnAccept->setEnabled(true);
        mBtnReject->setText("Reject");
        mBtnReject->setEnabled(true);
    } else {
        mBtnAccept->setText("Call");
        mBtnAccept->setEnabled(false);
        mBtnReject->setText("Terminate");
        mBtnReject->setEnabled(true);

        PendingMediaStreams *pms =
            streamChan->requestStream(mContact, Telepathy::MediaStreamTypeAudio);
        connect(pms,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onStreamCreated(Telepathy::Client::PendingOperation*)));
    }

    mTfChan = new FarsightChannel(streamChan, this);
    connect(mTfChan,
            SIGNAL(statusChanged(Telepathy::Client::FarsightChannel::Status)),
            SLOT(onTfChannelStatusChanged(Telepathy::Client::FarsightChannel::Status)));
}

void CallWidget::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "CallWindow::onChannelInvalidated: channel became invalid:" <<
        errorName << "-" << errorMessage;
    mLblStatus->setText("Call terminated");
    mStack->setCurrentIndex(0);
}

void CallWidget::onStreamCreated(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CallWidget::onStreamCreated: unable to create audio stream:" <<
            op->errorName() << "-" << op->errorMessage();
        mLblStatus->setText("Unable to establish call");
        mStack->setCurrentIndex(0);
        return;
    }
}

void CallWidget::onTfChannelStatusChanged(FarsightChannel::Status status)
{
    if (status == FarsightChannel::StatusDisconnected) {
        mLblStatus->setText("Call terminated");
        mStack->setCurrentIndex(0);
    }
}

void CallWidget::onBtnAcceptClicked()
{
    StreamedMediaChannel *streamChan =
        qobject_cast<StreamedMediaChannel *>(mChan.data());
    streamChan->acceptCall();

    mBtnAccept->setText("Call");
    mBtnAccept->setEnabled(false);
    mBtnReject->setText("Terminate");
    mBtnReject->setEnabled(true);
}

void CallWidget::onBtnRejectClicked()
{
    mChan->requestClose();

    mLblStatus->setText("Call terminated");
    mStack->setCurrentIndex(0);
}
