/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2012 Nokia Corporation
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

#include <TelepathyQt/PendingCaptchas>

#include "TelepathyQt/debug-internal.h"

#include "TelepathyQt/_gen/pending-captchas.moc.hpp"

#include <TelepathyQt/Captcha>
#include <TelepathyQt/captcha-authentication-internal.h>

namespace Tp
{

struct TP_QT_NO_EXPORT PendingCaptchas::Private
{
    Private(PendingCaptchas *parent);
    ~Private();

    CaptchaAuthentication::ChallengeType stringToChallengeType(const QString &string) const;
    void appendCaptchaResult(const QString &mimeType, const QString &label,
            const QByteArray &data, CaptchaAuthentication::ChallengeType type, uint id);

    // Public object
    PendingCaptchas *parent;

    CaptchaAuthentication::ChallengeTypes preferredTypes;
    QStringList preferredMimeTypes;

    bool multipleRequired;
    QList<Captcha> captchas;
    int captchasLeft;

    CaptchaAuthenticationPtr captchaAuthentication;
    ChannelPtr channel;
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
    if (string == QLatin1String("audio_recog")) {
        return CaptchaAuthentication::AudioRecognitionChallenge;
    } else if (string == QLatin1String("ocr")) {
        return CaptchaAuthentication::OCRChallenge;
    } else if (string == QLatin1String("picture_q")) {
        return CaptchaAuthentication::PictureQuestionChallenge;
    } else if (string == QLatin1String("picture_recog")) {
        return CaptchaAuthentication::PictureRecognitionChallenge;
    } else if (string == QLatin1String("qa")) {
        return CaptchaAuthentication::TextQuestionChallenge;
    } else if (string == QLatin1String("speech_q")) {
        return CaptchaAuthentication::SpeechQuestionChallenge;
    } else if (string == QLatin1String("speech_recog")) {
        return CaptchaAuthentication::SpeechRecognitionChallenge;
    } else if (string == QLatin1String("video_q")) {
        return CaptchaAuthentication::VideoQuestionChallenge;
    } else if (string == QLatin1String("video_recog")) {
        return CaptchaAuthentication::VideoRecognitionChallenge;
    }

    // Not really making sense...
    return CaptchaAuthentication::UnknownChallenge;
}

void PendingCaptchas::Private::appendCaptchaResult(const QString &mimeType, const QString &label,
        const QByteArray &data, CaptchaAuthentication::ChallengeType type, uint id)
{
    // Add to the list
    Captcha captchaItem(mimeType, label, data, type, id);

    captchas.append(captchaItem);

    --captchasLeft;

    if (!captchasLeft) {
        parent->setFinished();
    }
}

/**
 * \class PendingCaptchas
 * \ingroup client
 * \headerfile TelepathyQt/pending-captchas.h <TelepathyQt/PendingCaptchas>
 *
 * \brief The PendingCaptchas class represents an asynchronous
 * operation for retrieving a captcha challenge from a connection manager.
 *
 * See \ref async_model
 */

PendingCaptchas::PendingCaptchas(
        const QDBusPendingCall &call,
        const QStringList &preferredMimeTypes,
        CaptchaAuthentication::ChallengeTypes preferredTypes,
        const CaptchaAuthenticationPtr &captchaAuthentication)
    : PendingOperation(captchaAuthentication),
      mPriv(new Private(this))
{
    mPriv->captchaAuthentication = captchaAuthentication;
    mPriv->channel = captchaAuthentication->channel();
    mPriv->preferredMimeTypes = preferredMimeTypes;
    mPriv->preferredTypes = preferredTypes;

    /* keep track of channel invalidation */
    connect(mPriv->channel.data(),
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
        const CaptchaAuthenticationPtr &captchaAuthentication)
    : PendingOperation(captchaAuthentication),
      mPriv(new PendingCaptchas::Private(this))
{
    warning() << "PendingCaptchas created with instant failure";
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

    warning().nospace() << "PendingCaptchas failed because channel was invalidated with " <<
        errorName << ": " << errorMessage;

    setFinishedWithError(errorName, errorMessage);
}

void PendingCaptchas::onGetCaptchasWatcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<Tp::CaptchaInfoList, uint, QString> reply = *watcher;

    if (reply.isError()) {
        debug().nospace() << "PendingDBusCall failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to PendingDBusCall";
    Tp::CaptchaInfoList list = qdbus_cast<Tp::CaptchaInfoList>(reply.argumentAt(0));
    int howManyRequired = reply.argumentAt(1).toUInt();

    // Compute which captchas are required
    QList<QPair<Tp::CaptchaInfo,QString> > finalList;
    foreach (const Tp::CaptchaInfo &info, list) {
        // First of all, mimetype check
        QString mimeType;
        if (info.availableMIMETypes.isEmpty()) {
            // If it's one of the types which might not have a payload, go for it
            CaptchaAuthentication::ChallengeTypes noPayloadChallenges(
                    CaptchaAuthentication::TextQuestionChallenge |
                    CaptchaAuthentication::UnknownChallenge);
            if (mPriv->stringToChallengeType(info.type) & noPayloadChallenges) {
                // Ok, move on
            } else {
                // In this case, there's something wrong
                warning() << "Got a captcha with type " << info.type << " which does not "
                        "expose any available mimetype for its payload. Something might be "
                        "wrong with the connection manager.";
                continue;
            }
        } else if (mPriv->preferredMimeTypes.isEmpty()) {
            // No preference, let's take the first of the list
            mimeType = info.availableMIMETypes.first();
        } else {
            QSet<QString> supportedMimeTypes = info.availableMIMETypes.toSet().intersect(
                    mPriv->preferredMimeTypes.toSet());
            if (supportedMimeTypes.isEmpty()) {
                // Apparently our handler does not support any of this captcha's mimetypes, skip
                continue;
            }

            // Ok, use the first one
            mimeType = *supportedMimeTypes.constBegin();
        }

        // If it's required, easy
        if (info.flags & CaptchaFlagRequired) {
            finalList.append(qMakePair(info, mimeType));
            continue;
        }

        // Otherwise, let's see if the mimetype matches
        if (mPriv->preferredTypes & mPriv->stringToChallengeType(info.type)) {
            finalList.append(qMakePair(info, mimeType));
        }

        if (finalList.size() == howManyRequired) {
            break;
        }
    }

    if (finalList.size() != howManyRequired) {
        warning() << "No captchas available matching the specified preferences";
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("No captchas matching the handler's request"));
        return;
    }

    // Now, get the infos for all the required captchas in our final list.
    mPriv->captchasLeft = finalList.size();
    mPriv->multipleRequired = howManyRequired > 1 ? true : false;
    for (QList<QPair<Tp::CaptchaInfo,QString> >::const_iterator i = finalList.constBegin();
            i != finalList.constEnd(); ++i) {
        QString mimeType = (*i).second;
        Tp::CaptchaInfo captchaInfo = (*i).first;

        // If the captcha does not have a mimetype, we can add it straight
        if (mimeType.isEmpty()) {
            mPriv->appendCaptchaResult(mimeType, captchaInfo.label, QByteArray(),
                    mPriv->stringToChallengeType(captchaInfo.type), captchaInfo.ID);

            continue;
        }

        QDBusPendingCall call =
        mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->GetCaptchaData(
                    captchaInfo.ID, mimeType);

        QDBusPendingCallWatcher *dataWatcher = new QDBusPendingCallWatcher(call);
        dataWatcher->setProperty("__Tp_Qt_CaptchaID", captchaInfo.ID);
        dataWatcher->setProperty("__Tp_Qt_CaptchaMimeType", mimeType);
        dataWatcher->setProperty("__Tp_Qt_CaptchaLabel", captchaInfo.label);
        dataWatcher->setProperty("__Tp_Qt_CaptchaType",
                mPriv->stringToChallengeType(captchaInfo.type));

        connect(dataWatcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(onGetCaptchaDataWatcherFinished(QDBusPendingCallWatcher*)));
    }

    watcher->deleteLater();
}

void PendingCaptchas::onGetCaptchaDataWatcherFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QByteArray> reply = *watcher;

    if (reply.isError()) {
        debug().nospace() << "PendingDBusCall failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        watcher->deleteLater();
        return;
    }

    debug() << "Got reply to PendingDBusCall";

    // Add to the list
    mPriv->appendCaptchaResult(watcher->property("__Tp_Qt_CaptchaMimeType").toString(),
            watcher->property("__Tp_Qt_CaptchaLabel").toString(),
            reply.value(), static_cast<CaptchaAuthentication::ChallengeType>(
                watcher->property("__Tp_Qt_CaptchaType").toUInt()),
            watcher->property("__Tp_Qt_CaptchaID").toUInt());

    watcher->deleteLater();
}

/**
 * Return the main captcha of the request. This captcha is guaranteed to be compatible
 * with any constraint specified in CaptchaAuthentication::requestCaptchas().
 *
 * This is a convenience method which should be used when requiresMultipleCaptchas()
 * is false - otherwise, you should use captchaList.
 *
 * The returned Captcha can be answered through CaptchaAuthentication::answer() by
 * using its id.
 *
 * This method will return a meaningful value only if the operation was completed
 * successfully.
 *
 * \return The main captcha for the pending request.
 * \sa captchaList()
 *     CaptchaAuthentication::requestCaptchas()
 *     requiresMultipleCaptchas()
 *     CaptchaAuthentication::answer()
 */
Captcha PendingCaptchas::captcha() const
{
    if (!isFinished()) {
        return Captcha();
    }

    return mPriv->captchas.first();
}

/**
 * Return all the captchas of the request. These captchas are guaranteed to be compatible
 * with any constraint specified in CaptchaAuthentication::requestCaptchas().
 *
 * If requiresMultipleCaptchas() is false, you probably want to use the convenience method
 * captcha() instead.
 *
 * The returned Captchas can be answered through CaptchaAuthentication::answer() by
 * using their ids.
 *
 * This method will return a meaningful value only if the operation was completed
 * successfully.
 *
 * \return All the captchas for the pending request.
 * \sa captcha()
 *     CaptchaAuthentication::requestCaptchas()
 *     requiresMultipleCaptchas()
 *     CaptchaAuthentication::answer()
 */
QList<Captcha> PendingCaptchas::captchaList() const
{
    if (!isFinished()) {
        return QList<Captcha>();
    }

    return mPriv->captchas;
}

/**
 * Return whether this request requires more than one captcha to be answered or not.
 *
 * This method should always be checked before answering to find out what the
 * connection manager expects. Depending on the result, you might want to use the
 * result from captcha() if just a single answer is required, or from captchaList()
 * otherwise.
 *
 * This method will return a meaningful value only if the operation was completed
 * successfully.
 *
 * \return The main captcha for the pending request.
 * \sa captcha()
 *     captchaList()
 */
bool PendingCaptchas::requiresMultipleCaptchas() const
{
    return mPriv->multipleRequired;
}

}
