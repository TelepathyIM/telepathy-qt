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

#ifndef _TelepathyQt4_examples_call_call_widget_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_call_widget_h_HEADER_GUARD_

#include <QSharedPointer>
#include <QWidget>

#include <TelepathyQt4/Client/Channel>
#include <TelepathyQt4/Constants>

#include "farsight-channel.h"

namespace Telepathy {
namespace Client {
class Contact;
class DBusProxy;
class MediaStream;
class PendingMediaStreams;
class PendingOperation;
class StreamedMediaChannel;
}
}

class QLabel;
class QPushButton;
class QStatusBar;

class CallWidget : public QWidget
{
    Q_OBJECT

public:
    CallWidget(Telepathy::Client::StreamedMediaChannel *channel,
               const QSharedPointer<Telepathy::Client::Contact> &contact,
               QWidget *parent = 0);
    virtual ~CallWidget();

    Telepathy::Client::StreamedMediaChannel *channel() const { return mChan; }
    QSharedPointer<Telepathy::Client::Contact> contact() const { return mContact; }

private Q_SLOTS:
    void onChannelReady(Telepathy::Client::PendingOperation *);
    void onChannelInvalidated(Telepathy::Client::DBusProxy *,
            const QString &, const QString &);
    void onStreamCreated(Telepathy::Client::PendingOperation *);
    void onStreamAdded(const QSharedPointer<Telepathy::Client::MediaStream> &);
    void onStreamRemoved(Telepathy::Client::MediaStream *);
    void onStreamDirectionChanged(Telepathy::Client::MediaStream *,
            Telepathy::MediaStreamDirection,
            Telepathy::MediaStreamPendingSend);
    void onStreamStateChanged(Telepathy::Client::MediaStream *,
            Telepathy::MediaStreamState);
    void onTfChannelStatusChanged(Telepathy::Client::FarsightChannel::Status);

    void onBtnHangupClicked();
    void onBtnSendAudioToggled(bool);
    void onBtnSendVideoToggled(bool);

private:
    void createActions();
    void setupGui();

    QSharedPointer<Telepathy::Client::MediaStream> streamForType(Telepathy::MediaStreamType type) const;
    void connectStreamSignals(const QSharedPointer<Telepathy::Client::MediaStream> &stream);
    void updateStreamDirection(const QSharedPointer<Telepathy::Client::MediaStream> &stream);

    void callEnded(const QString &message);

    Telepathy::Client::StreamedMediaChannel *mChan;
    QSharedPointer<Telepathy::Client::Contact> mContact;
    Telepathy::Client::FarsightChannel *mTfChan;

    Telepathy::Client::PendingMediaStreams *mPmsAudio;
    Telepathy::Client::PendingMediaStreams *mPmsVideo;

    QPushButton *mBtnHangup;
    QPushButton *mBtnSendAudio;
    QPushButton *mBtnSendVideo;

    QLabel *mLblAudioDirection;
    QLabel *mLblVideoDirection;
    QLabel *mLblAudioState;
    QLabel *mLblVideoState;

    QStatusBar *mStatusBar;
};

#endif
