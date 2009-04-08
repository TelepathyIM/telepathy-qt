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
class DBusProxy;
class MediaStream;
class PendingMediaStreams;
class PendingOperation;
}

class QLabel;
class QPushButton;
class QStatusBar;

class CallWidget : public QWidget
{
    Q_OBJECT

public:
    CallWidget(const Telepathy::StreamedMediaChannelPtr &channel,
               const Telepathy::ContactPtr &contact,
               QWidget *parent = 0);
    virtual ~CallWidget();

    Telepathy::StreamedMediaChannelPtr channel() const { return mChan; }
    Telepathy::ContactPtr contact() const { return mContact; }

private Q_SLOTS:
    void onChannelReady(Telepathy::PendingOperation *);
    void onChannelInvalidated(Telepathy::DBusProxy *,
            const QString &, const QString &);
    void onStreamCreated(Telepathy::PendingOperation *);
    void onStreamAdded(const Telepathy::MediaStreamPtr &);
    void onStreamRemoved(const Telepathy::MediaStreamPtr &);
    void onStreamDirectionChanged(const Telepathy::MediaStreamPtr &,
            Telepathy::MediaStreamDirection,
            Telepathy::MediaStreamPendingSend);
    void onStreamStateChanged(const Telepathy::MediaStreamPtr &,
            Telepathy::MediaStreamState);
    void onTfChannelStatusChanged(Telepathy::FarsightChannel::Status);

    void onBtnHangupClicked();
    void onBtnSendAudioToggled(bool);
    void onBtnSendVideoToggled(bool);

private:
    void createActions();
    void setupGui();

    Telepathy::MediaStreamPtr streamForType(Telepathy::MediaStreamType type) const;
    void updateStreamDirection(const Telepathy::MediaStreamPtr &stream);

    void callEnded(const QString &message);

    Telepathy::StreamedMediaChannelPtr mChan;
    Telepathy::ContactPtr mContact;
    Telepathy::FarsightChannel *mTfChan;

    Telepathy::PendingMediaStreams *mPmsAudio;
    Telepathy::PendingMediaStreams *mPmsVideo;

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
