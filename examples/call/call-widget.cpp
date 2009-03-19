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

#include "video-widget.h"

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Contact>
#include <TelepathyQt4/Client/ContactManager>
#include <TelepathyQt4/Client/PendingChannel>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Client/StreamedMediaChannel>

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

using namespace Telepathy::Client;

CallWidget::CallWidget(StreamedMediaChannel *chan,
        const ContactPtr &contact,
        QWidget *parent)
    : QWidget(parent),
      mChan(chan),
      mContact(contact),
      mTfChan(new FarsightChannel(chan, this)),
      mPmsAudio(0),
      mPmsVideo(0)
{
    setWindowTitle(QString("Call (%1)").arg(mContact->id()));

    setAttribute(Qt::WA_DeleteOnClose);

    setupGui();

    connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            SLOT(onChannelReady(Telepathy::Client::PendingOperation*)));
    connect(mChan,
            SIGNAL(invalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)),
            SLOT(onChannelInvalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)));

    connect(mTfChan,
            SIGNAL(statusChanged(Telepathy::Client::FarsightChannel::Status)),
            SLOT(onTfChannelStatusChanged(Telepathy::Client::FarsightChannel::Status)));
}

CallWidget::~CallWidget()
{
    if (mChan) {
        mChan->requestClose();
    }
}

void CallWidget::setupGui()
{
    QVBoxLayout *mainBox = new QVBoxLayout;

    // buttons
    QHBoxLayout *btnBox = new QHBoxLayout;

    mBtnHangup = new QPushButton("Hangup");
    connect(mBtnHangup,
            SIGNAL(clicked(bool)),
            SLOT(onBtnHangupClicked()));
    btnBox->addWidget(mBtnHangup);

    mBtnSendAudio = new QPushButton("Send Audio");
    mBtnSendAudio->setCheckable(true);
    mBtnSendAudio->setChecked(true);
    mBtnSendAudio->setEnabled(false);
    connect(mBtnSendAudio,
            SIGNAL(toggled(bool)),
            SLOT(onBtnSendAudioToggled(bool)));
    btnBox->addWidget(mBtnSendAudio);

    mBtnSendVideo = new QPushButton("Send Video");
    mBtnSendVideo->setCheckable(true);
    mBtnSendVideo->setChecked(false);
    mBtnSendVideo->setEnabled(false);
    connect(mBtnSendVideo,
            SIGNAL(toggled(bool)),
            SLOT(onBtnSendVideoToggled(bool)));
    btnBox->addWidget(mBtnSendVideo);

    mainBox->addLayout(btnBox);

    // video
    QHBoxLayout *videoBox = new QHBoxLayout;

    VideoWidget *videoWidget = mTfChan->videoWidget();
    videoWidget->setMinimumSize(320, 240);
    videoBox->addWidget(videoWidget);

    QVBoxLayout *previewBox = new QVBoxLayout;

    VideoWidget *videoPreview = mTfChan->videoPreview();
    videoPreview->setFixedSize(160, 120);
    previewBox->addWidget(videoPreview);

    previewBox->addStretch(1);

    videoBox->addLayout(previewBox);

    mainBox->addLayout(videoBox);

    // stream status
    QFrame *frame = new QFrame;
    frame->setFrameStyle(QFrame::Box | QFrame::Sunken);

    QVBoxLayout *streamBox = new QVBoxLayout;

    QLabel *lbl = new QLabel(QLatin1String("<b>Streams</b>"));
    streamBox->addWidget(lbl);
    streamBox->addSpacing(4);

    lbl = new QLabel(QLatin1String("<b>Audio</b>"));
    streamBox->addWidget(lbl);
    mLblAudioDirection = new QLabel(QLatin1String("Direction: None"));
    streamBox->addWidget(mLblAudioDirection);
    mLblAudioState = new QLabel(QLatin1String("State: Disconnected"));
    streamBox->addWidget(mLblAudioState);
    streamBox->addSpacing(4);

    lbl = new QLabel(QLatin1String("<b>Video</b>"));
    streamBox->addWidget(lbl);
    mLblVideoDirection = new QLabel(QLatin1String("Direction: None"));
    streamBox->addWidget(mLblVideoDirection);
    mLblVideoState = new QLabel(QLatin1String("State: Disconnected"));
    streamBox->addWidget(mLblVideoState);

    streamBox->addStretch(1);
    frame->setLayout(streamBox);

    videoBox->addSpacing(4);
    frame->setLayout(streamBox);
    videoBox->addWidget(frame);

    // status bar
    mStatusBar = new QStatusBar;
    mainBox->addWidget(mStatusBar);

    setLayout(mainBox);
}

void CallWidget::onChannelReady(PendingOperation *op)
{
    if (op->isError()) {
        qWarning() << "CallWidget::onChannelReady: channel cannot become ready:" <<
            op->errorName() << "-" << op->errorMessage();
        mChan->requestClose();
        callEnded(QLatin1String("Unable to establish call"));
        return;
    }

#if 1
    MediaStreams streams = mChan->streams();
    qDebug() << "CallWidget::onChannelReady: number of streams:" << streams.size();
    qDebug() << " streams:";
    foreach (const QSharedPointer<MediaStream> &stream, streams) {
        qDebug() << "  " <<
            (stream->type() == Telepathy::MediaStreamTypeAudio ? "Audio" : "Video");
    }
#endif

    connect(mChan,
            SIGNAL(streamAdded(const QSharedPointer<Telepathy::Client::MediaStream> &)),
            SLOT(onStreamAdded(const QSharedPointer<Telepathy::Client::MediaStream> &)));

    QSharedPointer<MediaStream> stream = streamForType(Telepathy::MediaStreamTypeAudio);
    if (stream) {
        connectStreamSignals(stream);
        onStreamDirectionChanged(stream.data(), stream->direction(),
                stream->pendingSend());
        onStreamStateChanged(stream.data(), stream->state());
    }

    mBtnSendAudio->setEnabled(true);
    mBtnSendVideo->setEnabled(true);

    onBtnSendAudioToggled(mBtnSendAudio->isChecked());
}

void CallWidget::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "CallWindow::onChannelInvalidated: channel became invalid:" <<
        errorName << "-" << errorMessage;
    callEnded(errorMessage);
}

void CallWidget::onStreamCreated(PendingOperation *op)
{
    if (op == mPmsAudio) {
        mPmsAudio = 0;
    } else if (op == mPmsVideo) {
        mPmsVideo = 0;
    }

    if (op->isError()) {
        qWarning() << "CallWidget::onStreamCreated: unable to create stream:" <<
            op->errorName() << "-" << op->errorMessage();

        // we cannot create the stream for some reason, update buttons
        if (!streamForType(Telepathy::MediaStreamTypeAudio)) {
            mBtnSendAudio->setChecked(false);
        }
        if (!streamForType(Telepathy::MediaStreamTypeVideo)) {
            mBtnSendVideo->setChecked(false);
        }
        return;
    }

    PendingMediaStreams *pms = qobject_cast<PendingMediaStreams *>(op);
    Q_ASSERT(pms->streams().size() == 1);
    QSharedPointer<MediaStream> stream = pms->streams().first();
    connectStreamSignals(stream);
    updateStreamDirection(stream);
    onStreamDirectionChanged(stream.data(), stream->direction(),
            stream->pendingSend());
    onStreamStateChanged(stream.data(), stream->state());
}

void CallWidget::onStreamAdded(const QSharedPointer<MediaStream> &stream)
{
    connectStreamSignals(stream);
    updateStreamDirection(stream);
    onStreamDirectionChanged(stream.data(), stream->direction(),
            stream->pendingSend());
    onStreamStateChanged(stream.data(), stream->state());
}

void CallWidget::onStreamRemoved(MediaStream *stream)
{
    if (stream->type() == Telepathy::MediaStreamTypeAudio) {
        mBtnSendAudio->setChecked(false);
    } else if (stream->type() == Telepathy::MediaStreamTypeVideo) {
        mBtnSendVideo->setChecked(false);
    }
}

void CallWidget::onStreamDirectionChanged(MediaStream *stream,
        Telepathy::MediaStreamDirection direction,
        Telepathy::MediaStreamPendingSend pendingSend)
{
    qDebug() << "CallWidget::onStreamDirectionChanged: stream " <<
        (stream->type() == Telepathy::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "direction changed to" << direction;

    QLabel *lbl;
    if (stream->type() == Telepathy::MediaStreamTypeAudio) {
        lbl = mLblAudioDirection;
    } else {
        lbl = mLblVideoDirection;
    }

    if (direction == Telepathy::MediaStreamDirectionSend) {
        lbl->setText(QLatin1String("Direction: Sending"));
    } else if (direction == Telepathy::MediaStreamDirectionReceive) {
        lbl->setText(QLatin1String("Direction: Receiving"));
    } else if (direction == (Telepathy::MediaStreamDirectionSend | Telepathy::MediaStreamDirectionReceive)) {
        lbl->setText(QLatin1String("Direction: Sending/Receiving"));
    } else {
        lbl->setText(QLatin1String("Direction: None"));
    }
}

void CallWidget::onStreamStateChanged(MediaStream *stream,
        Telepathy::MediaStreamState state)
{
    qDebug() << "CallWidget::onStreamStateChanged: stream " <<
        (stream->type() == Telepathy::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "state changed to" << state;

    QLabel *lbl;
    if (stream->type() == Telepathy::MediaStreamTypeAudio) {
        lbl = mLblAudioState;
    } else {
        lbl = mLblVideoState;
    }

    if (state == Telepathy::MediaStreamStateDisconnected) {
        lbl->setText(QLatin1String("State: Disconnected"));
    } else if (state == Telepathy::MediaStreamStateConnecting) {
        lbl->setText(QLatin1String("State: Connecting"));
    } else if (state == Telepathy::MediaStreamStateConnected) {
        lbl->setText(QLatin1String("State: Connected"));
    }
}

void CallWidget::onTfChannelStatusChanged(FarsightChannel::Status status)
{
    switch (status) {
    case FarsightChannel::StatusConnecting:
        mStatusBar->showMessage("Connecting...");
        break;
    case FarsightChannel::StatusConnected:
        mStatusBar->showMessage("Connected");
        break;
    case FarsightChannel::StatusDisconnected:
        mChan->requestClose();
        callEnded("Call terminated");
        break;
    }
}

void CallWidget::onBtnHangupClicked()
{
    mChan->requestClose();
    callEnded("Call terminated");
}

void CallWidget::onBtnSendAudioToggled(bool checked)
{
    QSharedPointer<MediaStream> stream =
        streamForType(Telepathy::MediaStreamTypeAudio);
    qDebug() << "CallWidget::onBtnSendAudioToggled: checked:" << checked;
    if (!stream) {
        if (mPmsAudio) {
            return;
        }
        qDebug() << "CallWidget::onBtnSendAudioToggled: creating audio stream";
        mPmsAudio = mChan->requestStream(mContact, Telepathy::MediaStreamTypeAudio);
        connect(mPmsAudio,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onStreamCreated(Telepathy::Client::PendingOperation*)));
    } else {
        updateStreamDirection(stream);
    }
}

void CallWidget::onBtnSendVideoToggled(bool checked)
{
    QSharedPointer<MediaStream> stream =
        streamForType(Telepathy::MediaStreamTypeVideo);
    qDebug() << "CallWidget::onBtnSendVideoToggled: checked:" << checked;
    if (!stream) {
        if (mPmsVideo) {
            return;
        }
        qDebug() << "CallWidget::onBtnSendVideoToggled: creating video stream";
        mPmsVideo = mChan->requestStream(mContact, Telepathy::MediaStreamTypeVideo);
        connect(mPmsVideo,
                SIGNAL(finished(Telepathy::Client::PendingOperation*)),
                SLOT(onStreamCreated(Telepathy::Client::PendingOperation*)));
    } else {
        updateStreamDirection(stream);
    }
}

QSharedPointer<MediaStream> CallWidget::streamForType(Telepathy::MediaStreamType type) const
{
    MediaStreams streams = mChan->streams();
    foreach (const QSharedPointer<MediaStream> &stream, streams) {
        if (stream->type() == type) {
            return stream;
        }
    }
    return QSharedPointer<MediaStream>();
}

void CallWidget::connectStreamSignals(const QSharedPointer<MediaStream> &stream)
{
    connect(stream.data(),
            SIGNAL(removed(Telepathy::Client::MediaStream *)),
            SLOT(onStreamRemoved(Telepathy::Client::MediaStream *)));
    connect(stream.data(),
            SIGNAL(directionChanged(Telepathy::Client::MediaStream *,
                                    Telepathy::MediaStreamDirection,
                                    Telepathy::MediaStreamPendingSend)),
            SLOT(onStreamDirectionChanged(Telepathy::Client::MediaStream *,
                                          Telepathy::MediaStreamDirection,
                                          Telepathy::MediaStreamPendingSend)));
    connect(stream.data(),
            SIGNAL(stateChanged(Telepathy::Client::MediaStream *,
                                Telepathy::MediaStreamState)),
            SLOT(onStreamStateChanged(Telepathy::Client::MediaStream *,
                                      Telepathy::MediaStreamState)));
}

void CallWidget::updateStreamDirection(const QSharedPointer<MediaStream> &stream)
{
    bool checked = false;
    if (stream->type() == Telepathy::MediaStreamTypeAudio) {
        checked = mBtnSendAudio->isChecked();
    } else if (stream->type() == Telepathy::MediaStreamTypeVideo) {
        checked = mBtnSendVideo->isChecked();
    }

    qDebug() << "CallWidget::updateStreamDirection: current stream direction:" <<
        stream->direction() << "- checked:" << checked;

    if (checked) {
        if (!(stream->direction() & Telepathy::MediaStreamDirectionSend)) {
            int dir = stream->direction() | Telepathy::MediaStreamDirectionSend;
            stream->requestStreamDirection((Telepathy::MediaStreamDirection) dir);
            qDebug() << "CallWidget::updateStreamDirection: start sending " <<
                (stream->type() == Telepathy::MediaStreamTypeAudio ? "audio" : "video");
        }
    } else {
        if (stream->direction() & Telepathy::MediaStreamDirectionSend) {
            int dir = stream->direction() & ~Telepathy::MediaStreamDirectionSend;
            qDebug() << "CallWidget::updateStreamDirection: stop sending " <<
                (stream->type() == Telepathy::MediaStreamTypeAudio ? "audio" : "video");
            stream->requestStreamDirection((Telepathy::MediaStreamDirection) dir);
        }
    }
}

void CallWidget::callEnded(const QString &message)
{
    mStatusBar->showMessage(message);
    disconnect(mChan,
               SIGNAL(invalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)),
               this,
               SLOT(onChannelInvalidated(Telepathy::Client::DBusProxy *, const QString &, const QString &)));
    disconnect(mTfChan,
               SIGNAL(statusChanged(Telepathy::Client::FarsightChannel::Status)),
               this,
               SLOT(onTfChannelStatusChanged(Telepathy::Client::FarsightChannel::Status)));
    setEnabled(false);
}
