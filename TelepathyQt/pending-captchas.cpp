/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include "pending-captchas.h"

#include "captcha-authentication.h"
#include "captcha-authentication-internal.h"

#include "cli-channel.h"

namespace Tp {

struct TP_QT_NO_EXPORT PendingCaptchas::Private
{
    Private(PendingCaptchas *parent);
    ~Private();

    CaptchaAuthentication::ChallengeType stringToChallengeType(const QString &string) const;

    // Public object
    PendingCaptchas *parent;

    CaptchaAuthentication::ChallengeTypes preferredTypes;
    QStringList preferredMimeTypes;

    bool multipleRequired;
    QList<CaptchaAuthentication::Captcha> captchas;
    int captchasLeft;

    CaptchaAuthenticationPtr channel;
};

PendingCaptchas::Private::Private(PendingCaptchas *parent)
    : parent(parent)
{

}

PendingCaptchas::Private::~Private()
{
}

CaptchaAuthentication::ChallengeType PendingCaptchas::Private::stringToChallengeType(const QString &string) const
{
    if (string == "audio_recog") {
        return CaptchaAuthentication::AudioRecognitionChallenge;
    } else if (string == "ocr") {
        return CaptchaAuthentication::OCRChallenge;
    } else if (string == "picture_q") {
        return CaptchaAuthentication::PictureQuestionChallenge;
    } else if (string == "picture_recog") {
        return CaptchaAuthentication::PictureRecognitionChallenge;
    } else if (string == "qa") {
        return CaptchaAuthentication::TextQuestionChallenge;
    } else if (string == "speech_q") {
        return CaptchaAuthentication::SpeechQuestionChallenge;
    } else if (string == "speech_recog") {
        return CaptchaAuthentication::SpeechRecognitionChallenge;
    } else if (string == "video_q") {
        return CaptchaAuthentication::VideoQuestionChallenge;
    } else if (string == "video_recog") {
        return CaptchaAuthentication::VideoRecognitionChallenge;
    }

    // Not really making sense...
    return CaptchaAuthentication::UnknownChallenge;
}

/**
 * \class PendingCaptcha
 * \ingroup clientchannel
 * \headerfile TelepathyQt/incoming-stream-tube-channel.h <TelepathyQt/PendingCaptcha>
 *
 * \brief The PendingCaptcha class represents an asynchronous
 * operation for accepting an incoming stream tube.
 *
 * See \ref async_model
 */

PendingCaptchas::PendingCaptchas(
        const QDBusPendingCall &call,
        const QStringList &preferredMimeTypes,
        CaptchaAuthentication::ChallengeTypes preferredTypes,
        const CaptchaAuthenticationPtr &channel)
    : PendingOperation(channel),
      mPriv(new Private(this))
{
    mPriv->channel = channel;
    mPriv->preferredMimeTypes = preferredMimeTypes;
    mPriv->preferredTypes = preferredTypes;

    /* keep track of channel invalidation */
    connect(channel.data(),
            SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(onChannelInvalidated(Tp::DBusProxy*,QString,QString)));

    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(onGetCaptchasWatcherFinished(QDBusPendingCallWatcher*)));
}

PendingCaptchas::PendingCaptchas(
        const QString& errorName,
        const QString& errorMessage,
        const CaptchaAuthenticationPtr &channel)
    : PendingOperation(channel),
      mPriv(new PendingCaptchas::Private(this))
{
    setFinishedWithError(errorName, errorMessage);
}

/**
 * Class destructor.
 */
PendingCaptchas::~PendingCaptchas()
{
    delete mPriv;
}

void PendingCaptchas::onChannelInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);

    if (isFinished()) {
        return;
    }

    qWarning().nospace() << "StreamTube.Accept failed because channel was invalidated with " <<
        errorName << ": " << errorMessage;

    setFinishedWithError(errorName, errorMessage);
}

void PendingCaptchas::onGetCaptchasWatcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<Tp::CaptchaInfoList, uint, QString> reply = *watcher;

    if (reply.isError()) {
        qDebug().nospace() << "PendingDBusCall failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to PendingDBusCall";
    Tp::CaptchaInfoList list = reply.argumentAt(0).value<Tp::CaptchaInfoList>();
    int howManyRequired = reply.argumentAt(1).toUInt();

    // Compute which captchas are required
    Tp::CaptchaInfoList finalList;
    Q_FOREACH (const Tp::CaptchaInfo &info, list) {
        // If it's required, easy
        if (info.flags & CaptchaFlagRequired) {
            finalList.append(info);
            continue;
        }

        // Otherwise, let's see if the mimetype matches
        if (mPriv->preferredTypes & mPriv->stringToChallengeType(info.type)) {
            finalList.append(info);
        }

        if (finalList.size() == howManyRequired) {
            break;
        }
    }

    if (finalList.size() != howManyRequired) {
        // No captchas available with our preferences
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                "No captchas matching the handler's request");
        return;
    }

    // Now, get the infos for all the required captchas in our final list.
    mPriv->captchasLeft = finalList.size();
    mPriv->multipleRequired = howManyRequired > 1 ? true : false;
    Q_FOREACH (const Tp::CaptchaInfo &info, finalList) {
        QDBusPendingCall call =
        mPriv->channel->mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->GetCaptchaData(
                    info.ID, mPriv->preferredMimeTypes);

        QDBusPendingCallWatcher *dataWatcher = new QDBusPendingCallWatcher(call);
        dataWatcher->setProperty("__Tp_Qt_CaptchaID", info.ID);
        dataWatcher->setProperty("__Tp_Qt_CaptchaType",
                mPriv->stringToChallengeType(info.type));
        connect(dataWatcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(onGetCaptchaDataWatcherFinished(QDBusPendingCallWatcher*)));
    }

    watcher->deleteLater();
}

void PendingCaptchas::onGetCaptchaDataWatcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString, QByteArray> reply = *watcher;

    if (reply.isError()) {
        qDebug().nospace() << "PendingDBusCall failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    qDebug() << "Got reply to PendingDBusCall";

    // Add to the list
    CaptchaAuthentication::Captcha captcha;
    captcha.ID = watcher->property("__Tp_Qt_CaptchaID").toInt();
    captcha.type = (CaptchaAuthentication::ChallengeType)
            watcher->property("__Tp_Qt_CaptchaType").toUInt();
    captcha.mimeType = reply.argumentAt(0).toString();
    captcha.data = reply.argumentAt(1).toByteArray();
    --mPriv->captchasLeft;

    if (!mPriv->captchasLeft) {
        setFinished();
    }

    watcher->deleteLater();
}

CaptchaAuthentication::Captcha PendingCaptchas::captcha() const
{
    if (!isFinished()) {
        return CaptchaAuthentication::Captcha();
    }

    return mPriv->captchas.first();
}

QList<CaptchaAuthentication::Captcha> PendingCaptchas::captchaList() const
{
    if (!isFinished()) {
        return QList<CaptchaAuthentication::Captcha>();
    }

    return mPriv->captchas;
}

bool PendingCaptchas::requiresMultipleCaptchas() const
{
    return mPriv->multipleRequired;
}

}
