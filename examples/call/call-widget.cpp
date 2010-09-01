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

#include "call-widget.h"
#include "_gen/call-widget.moc.hpp"

#include "video-widget.h"

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Connection>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingChannel>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/StreamedMediaChannel>

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

using namespace Tp;

CallWidget::CallWidget(const StreamedMediaChannelPtr &chan,
        const ContactPtr &contact,
        QWidget *parent)
    : QWidget(parent),
      mChan(chan),
      mContact(contact),
      mTfChan(new FarsightChannel(chan, this)),
      mPmsAudio(0),
      mPmsVideo(0)
{
    setWindowTitle(QString(QLatin1String("Call (%1)")).arg(mContact->id()));

    setAttribute(Qt::WA_DeleteOnClose);

    setupGui();

    connect(mChan->becomeReady(StreamedMediaChannel::FeatureStreams),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onChannelReady(Tp::PendingOperation*)));
    connect(mChan.data(),
            SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
            SLOT(onChannelInvalidated(Tp::DBusProxy *, const QString &, const QString &)));

    connect(mTfChan,
            SIGNAL(statusChanged(Tp::FarsightChannel::Status)),
            SLOT(onTfChannelStatusChanged(Tp::FarsightChannel::Status)));
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

    mBtnHangup = new QPushButton(QLatin1String("Hangup"));
    connect(mBtnHangup,
            SIGNAL(clicked(bool)),
            SLOT(onBtnHangupClicked()));
    btnBox->addWidget(mBtnHangup);

    mBtnSendAudio = new QPushButton(QLatin1String("Send Audio"));
    mBtnSendAudio->setCheckable(true);
    mBtnSendAudio->setChecked(true);
    mBtnSendAudio->setEnabled(false);
    connect(mBtnSendAudio,
            SIGNAL(toggled(bool)),
            SLOT(onBtnSendAudioToggled(bool)));
    btnBox->addWidget(mBtnSendAudio);

    mBtnSendVideo = new QPushButton(QLatin1String("Send Video"));
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

    connect(mChan.data(),
            SIGNAL(streamAdded(const Tp::MediaStreamPtr &)),
            SLOT(onStreamAdded(const Tp::MediaStreamPtr &)));
    connect(mChan.data(),
            SIGNAL(streamRemoved(const Tp::MediaStreamPtr &)),
            SLOT(onStreamRemoved(const Tp::MediaStreamPtr &)));
    connect(mChan.data(),
            SIGNAL(streamDirectionChanged(const Tp::MediaStreamPtr &,
                                          Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)),
            SLOT(onStreamDirectionChanged(const Tp::MediaStreamPtr &,
                                          Tp::MediaStreamDirection,
                                          Tp::MediaStreamPendingSend)));
    connect(mChan.data(),
            SIGNAL(streamStateChanged(const Tp::MediaStreamPtr &,
                                      Tp::MediaStreamState)),
            SLOT(onStreamStateChanged(const Tp::MediaStreamPtr &,
                                      Tp::MediaStreamState)));

    MediaStreams streams = mChan->streams();
    qDebug() << "CallWidget::onChannelReady: number of streams:" << streams.size();
    if (streams.size() > 0) {
        Q_FOREACH (const MediaStreamPtr &stream, streams) {
            qDebug() << "  type:" <<
                (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video");
            qDebug() << "  direction:" << stream->direction();
            qDebug() << "  state:" << stream->state();

            onStreamDirectionChanged(stream, stream->direction(),
                    stream->pendingSend());
            onStreamStateChanged(stream, stream->state());
        }
    }

    mBtnSendAudio->setEnabled(true);
    mBtnSendVideo->setEnabled(true);

    onBtnSendAudioToggled(mBtnSendAudio->isChecked());
}

void CallWidget::onChannelInvalidated(DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    qDebug() << "CallWidget::onChannelInvalidated: channel became invalid:" <<
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

        QPushButton *btn;

        // we cannot create the stream for some reason, update buttons
        if (!streamForType(Tp::MediaStreamTypeAudio)) {
            btn = mBtnSendAudio;
        } else if (!streamForType(Tp::MediaStreamTypeVideo)) {
            btn = mBtnSendVideo;
        } else {
            // we only handle audio and video, so ignore here
            return;
        }

        btn->blockSignals(true);
        btn->setChecked(false);
        btn->blockSignals(false);

        MediaStreams streams = mChan->streams();
        if (streams.size() == 0) {
            callEnded(op->errorMessage());
        }
        return;
    }

    // do nothing as streamAdded signal will be emitted
}

void CallWidget::onStreamAdded(const MediaStreamPtr &stream)
{
    qDebug() << "CallWidget::onStreamAdded:" <<
        (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "stream created";
    qDebug() << " direction:" << stream->direction();
    qDebug() << " state:" << stream->state();

    updateStreamDirection(stream);
    onStreamDirectionChanged(stream, stream->direction(),
            stream->pendingSend());
    onStreamStateChanged(stream, stream->state());
}

void CallWidget::onStreamRemoved(const MediaStreamPtr &stream)
{
    qDebug() << "CallWidget::onStreamRemoved:" <<
        (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "stream removed";
    if (stream->type() == Tp::MediaStreamTypeAudio) {
        mBtnSendAudio->blockSignals(true);
        mBtnSendAudio->setChecked(false);
        mBtnSendAudio->blockSignals(false);
        mLblAudioDirection->setText(QLatin1String("Direction: None"));
        mLblAudioState->setText(QLatin1String("State: Disconnected"));
    } else if (stream->type() == Tp::MediaStreamTypeVideo) {
        mBtnSendVideo->blockSignals(true);
        mBtnSendVideo->setChecked(false);
        mBtnSendVideo->blockSignals(false);
        mLblVideoDirection->setText(QLatin1String("Direction: None"));
        mLblVideoState->setText(QLatin1String("State: Disconnected"));
    }
}

void CallWidget::onStreamDirectionChanged(const MediaStreamPtr &stream,
        Tp::MediaStreamDirection direction,
        Tp::MediaStreamPendingSend pendingSend)
{
    qDebug() << "CallWidget::onStreamDirectionChanged:" <<
        (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "stream direction changed to" << direction;

    QLabel *lbl = 0;
    QPushButton *btn = 0;
    if (stream->type() == Tp::MediaStreamTypeAudio) {
        lbl = mLblAudioDirection;
        btn = mBtnSendAudio;
    } else if (stream->type() == Tp::MediaStreamTypeVideo) {
        lbl = mLblVideoDirection;
        btn = mBtnSendVideo;
    } else {
        Q_ASSERT(false);
    }

    if (direction == Tp::MediaStreamDirectionSend) {
        lbl->setText(QLatin1String("Direction: Sending"));
    } else if (direction == Tp::MediaStreamDirectionReceive) {
        lbl->setText(QLatin1String("Direction: Receiving"));
    } else if (direction == (Tp::MediaStreamDirectionSend | Tp::MediaStreamDirectionReceive)) {
        lbl->setText(QLatin1String("Direction: Sending/Receiving"));
    } else {
        lbl->setText(QLatin1String("Direction: None"));
    }

    btn->blockSignals(true);
    btn->setChecked(direction & Tp::MediaStreamDirectionSend);
    btn->blockSignals(false);
}

void CallWidget::onStreamStateChanged(const MediaStreamPtr &stream,
        Tp::MediaStreamState state)
{
    qDebug() << "CallWidget::onStreamStateChanged:" <<
        (stream->type() == Tp::MediaStreamTypeAudio ? "Audio" : "Video") <<
        "stream state changed to" << state;

    QLabel *lbl = 0;
    if (stream->type() == Tp::MediaStreamTypeAudio) {
        lbl = mLblAudioState;
    } else if (stream->type() == Tp::MediaStreamTypeVideo) {
        lbl = mLblVideoState;
    } else {
        // ignore unknown streams
        return;
    }
    if (state == Tp::MediaStreamStateDisconnected) {
        lbl->setText(QLatin1String("State: Disconnected"));
    } else if (state == Tp::MediaStreamStateConnecting) {
        lbl->setText(QLatin1String("State: Connecting"));
    } else if (state == Tp::MediaStreamStateConnected) {
        lbl->setText(QLatin1String("State: Connected"));
    }
}

void CallWidget::onTfChannelStatusChanged(FarsightChannel::Status status)
{
    switch (status) {
    case FarsightChannel::StatusConnecting:
        mStatusBar->showMessage(QLatin1String("Connecting..."));
        break;
    case FarsightChannel::StatusConnected:
        mStatusBar->showMessage(QLatin1String("Connected"));
        break;
    case FarsightChannel::StatusDisconnected:
        mChan->requestClose();
        callEnded(QLatin1String("Call terminated"));
        break;
    }
}

void CallWidget::onBtnHangupClicked()
{
    mChan->requestClose();
    callEnded(QLatin1String("Call terminated"));
}

void CallWidget::onBtnSendAudioToggled(bool checked)
{
    MediaStreamPtr stream =
        streamForType(Tp::MediaStreamTypeAudio);
    qDebug() << "CallWidget::onBtnSendAudioToggled: checked:" << checked;
    if (!stream) {
        if (mPmsAudio) {
            return;
        }
        qDebug() << "CallWidget::onBtnSendAudioToggled: creating audio stream";
        mPmsAudio = mChan->requestStream(mContact, Tp::MediaStreamTypeAudio);
        connect(mPmsAudio,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onStreamCreated(Tp::PendingOperation*)));
    } else {
        updateStreamDirection(stream);
    }
}

void CallWidget::onBtnSendVideoToggled(bool checked)
{
    MediaStreamPtr stream =
        streamForType(Tp::MediaStreamTypeVideo);
    qDebug() << "CallWidget::onBtnSendVideoToggled: checked:" << checked;
    if (!stream) {
        if (mPmsVideo) {
            return;
        }
        qDebug() << "CallWidget::onBtnSendVideoToggled: creating video stream";
        mPmsVideo = mChan->requestStream(mContact, Tp::MediaStreamTypeVideo);
        connect(mPmsVideo,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onStreamCreated(Tp::PendingOperation*)));
    } else {
        updateStreamDirection(stream);
    }
}

MediaStreamPtr CallWidget::streamForType(Tp::MediaStreamType type) const
{
    MediaStreams streams = mChan->streams();
    Q_FOREACH (const MediaStreamPtr &stream, streams) {
        if (stream->type() == type) {
            return stream;
        }
    }
    return MediaStreamPtr();
}

void CallWidget::updateStreamDirection(const MediaStreamPtr &stream)
{
    bool checked = false;
    if (stream->type() == Tp::MediaStreamTypeAudio) {
        checked = mBtnSendAudio->isChecked();
    } else if (stream->type() == Tp::MediaStreamTypeVideo) {
        checked = mBtnSendVideo->isChecked();
    } else {
        Q_ASSERT(false);
    }

    qDebug() << "CallWidget::updateStreamDirection: updating" <<
        (stream->type() == Tp::MediaStreamTypeAudio ? "audio" : "video") <<
        "stream direction";

    if (checked) {
        if (!(stream->direction() & Tp::MediaStreamDirectionSend)) {
            int dir = stream->direction() | Tp::MediaStreamDirectionSend;
            stream->requestDirection((Tp::MediaStreamDirection) dir);
            qDebug() << "CallWidget::updateStreamDirection: start sending" <<
                (stream->type() == Tp::MediaStreamTypeAudio ? "audio" : "video");
        } else {
            qDebug() << "CallWidget::updateStreamDirection:" <<
                (stream->type() == Tp::MediaStreamTypeAudio ? "audio" : "video") <<
                "stream updated";
        }
    } else {
        if (stream->direction() & Tp::MediaStreamDirectionSend) {
            int dir = stream->direction() & ~Tp::MediaStreamDirectionSend;
            qDebug() << "CallWidget::updateStreamDirection: stop sending " <<
                (stream->type() == Tp::MediaStreamTypeAudio ? "audio" : "video");
            stream->requestDirection((Tp::MediaStreamDirection) dir);
        } else {
            qDebug() << "CallWidget::updateStreamDirection:" <<
                (stream->type() == Tp::MediaStreamTypeAudio ? "audio" : "video") <<
                "stream updated";
        }
    }
}

void CallWidget::callEnded(const QString &message)
{
    mStatusBar->showMessage(message);
    disconnect(mChan.data(),
               SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
               this,
               SLOT(onChannelInvalidated(Tp::DBusProxy *, const QString &, const QString &)));
    disconnect(mTfChan,
               SIGNAL(statusChanged(Tp::FarsightChannel::Status)),
               this,
               SLOT(onTfChannelStatusChanged(Tp::FarsightChannel::Status)));
    setEnabled(false);
}
