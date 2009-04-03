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

#ifndef _TelepathyQt4_examples_call_call_widget_h_HEADER_GUARD_
#define _TelepathyQt4_examples_call_call_widget_h_HEADER_GUARD_

#include <QWidget>

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/StreamedMediaChannel>
#include <TelepathyQt4/Constants>

#include "farsight-channel.h"

namespace Telepathy {
namespace Client {
class DBusProxy;
class MediaStream;
class PendingMediaStreams;
class PendingOperation;
}
}

class QLabel;
class QPushButton;
class QStatusBar;

class CallWidget : public QWidget
{
    Q_OBJECT

public:
    CallWidget(const Telepathy::Client::StreamedMediaChannelPtr &channel,
               const Telepathy::Client::ContactPtr &contact,
               QWidget *parent = 0);
    virtual ~CallWidget();

    Telepathy::Client::StreamedMediaChannelPtr channel() const { return mChan; }
    Telepathy::Client::ContactPtr contact() const { return mContact; }

private Q_SLOTS:
    void onChannelReady(Telepathy::Client::PendingOperation *);
    void onChannelInvalidated(Telepathy::Client::DBusProxy *,
            const QString &, const QString &);
    void onStreamCreated(Telepathy::Client::PendingOperation *);
    void onStreamAdded(const Telepathy::Client::MediaStreamPtr &);
    void onStreamRemoved(const Telepathy::Client::MediaStreamPtr &);
    void onStreamDirectionChanged(const Telepathy::Client::MediaStreamPtr &,
            Telepathy::MediaStreamDirection,
            Telepathy::MediaStreamPendingSend);
    void onStreamStateChanged(const Telepathy::Client::MediaStreamPtr &,
            Telepathy::MediaStreamState);
    void onTfChannelStatusChanged(Telepathy::Client::FarsightChannel::Status);

    void onBtnHangupClicked();
    void onBtnSendAudioToggled(bool);
    void onBtnSendVideoToggled(bool);

private:
    void createActions();
    void setupGui();

    Telepathy::Client::MediaStreamPtr streamForType(Telepathy::MediaStreamType type) const;
    void updateStreamDirection(const Telepathy::Client::MediaStreamPtr &stream);

    void callEnded(const QString &message);

    Telepathy::Client::StreamedMediaChannelPtr mChan;
    Telepathy::Client::ContactPtr mContact;
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
