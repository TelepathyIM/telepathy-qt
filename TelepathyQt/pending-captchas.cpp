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

class TP_QT_NO_EXPORT CaptchaData : public QSharedData
{
public:
    CaptchaData();
    CaptchaData(const CaptchaData &other);
    ~CaptchaData();

    QString mimeType;
    QString label;
    QByteArray captchaData;
    CaptchaAuthentication::ChallengeType type;
    int id;
};

CaptchaData::CaptchaData()
{
}

CaptchaData::CaptchaData(const CaptchaData &other)
    : QSharedData(other),
      mimeType(other.mimeType),
      label(other.label),
      captchaData(other.captchaData),
      type(other.type),
      id(other.id)
{
}

CaptchaData::~CaptchaData()
{
}

/**
 * \class Captcha
 * \ingroup client
 * \headerfile TelepathyQt/pending-captchas.h <TelepathyQt/PendingCaptchas>
 *
 * \brief The Captcha class represents a Captcha ready to be answered.
 *
 * It exposes all the parameters needed for a handler to present the user with a captcha.
 *
 * Please note this class is meant to be read-only. It is usually created by PendingCaptchas
 * once a Captcha request operation succeeds.
 *
 * Note: this class is implicitly shared
 */

/**
 * Default constructor.
 */
Captcha::Captcha()
    : mPriv(new CaptchaData)
{
}

/**
 * Copy constructor.
 */
Captcha::Captcha(const Captcha &other)
    : mPriv(other.mPriv)
{
}

Captcha::Captcha(const QString &mimeType, const QString &label,
        const QByteArray &data, CaptchaAuthentication::ChallengeType type, int id)
    : mPriv(new CaptchaData)
{
    mPriv->mimeType = mimeType;
    mPriv->label = label;
    mPriv->captchaData = data;
    mPriv->type = type;
    mPriv->id = id;
}

/**
 * Class destructor.
 */
Captcha::~Captcha()
{
}

/**
 * Return the mimetype of the captcha.
 *
 * \return The captcha's mimetype.
 * \sa data()
 */
QString Captcha::mimeType() const
{
    return mPriv->mimeType;
}

/**
 * Return the label of the captcha. For some captcha types, such as
 * CaptchaAuthentication::TextQuestionChallenge, the label is also
 * the challenge the user has to answer
 *
 * \return The captcha's label.
 * \sa data()
 *     type()
 */
QString Captcha::label() const
{
    return mPriv->label;
}

/**
 * Return the raw data of the captcha. The handler can check its type and
 * metatype to know how to parse the blob.
 *
 * \return The captcha's data.
 * \sa mimeType(), type()
 */
QByteArray Captcha::data() const
{
    return mPriv->captchaData;
}

/**
 * Return the type of the captcha.
 *
 * \return The captcha's type.
 * \sa data()
 */
CaptchaAuthentication::ChallengeType Captcha::type() const
{
    return mPriv->type;
}

/**
 * Return the id of the captcha. This parameter should be used to identify
 * the captcha when answering its challenge.
 *
 * \return The captcha's id.
 * \sa CaptchaAuthentication::answer()
 */
int Captcha::id() const
{
    return mPriv->id;
}

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
    QList<Captcha> captchas;
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
    QList<QPair<Tp::CaptchaInfo,QString> > finalList;
    Q_FOREACH (const Tp::CaptchaInfo &info, list) {
        // First of all, mimetype check
        QString mimeType;
        if (mPriv->preferredMimeTypes.isEmpty()) {
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
        // No captchas available with our preferences
        setFinishedWithError(TP_QT_ERROR_NOT_AVAILABLE,
                "No captchas matching the handler's request");
        return;
    }

    // Now, get the infos for all the required captchas in our final list.
    mPriv->captchasLeft = finalList.size();
    mPriv->multipleRequired = howManyRequired > 1 ? true : false;
    for (QList<QPair<Tp::CaptchaInfo,QString> >::const_iterator i = finalList.constBegin();
            i != finalList.constEnd(); ++i) {
        QDBusPendingCall call =
        mPriv->channel->mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->GetCaptchaData(
                    (*i).first.ID, (*i).second);

        QDBusPendingCallWatcher *dataWatcher = new QDBusPendingCallWatcher(call);
        dataWatcher->setProperty("__Tp_Qt_CaptchaID", (*i).first.ID);
        dataWatcher->setProperty("__Tp_Qt_CaptchaLabel", (*i).first.label);
        dataWatcher->setProperty("__Tp_Qt_CaptchaType",
                mPriv->stringToChallengeType((*i).first.type));

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
    Captcha captchaItem(reply.argumentAt(0).toString(),
            watcher->property("__Tp_Qt_CaptchaLabel").toString(),
            reply.argumentAt(1).toByteArray(), (CaptchaAuthentication::ChallengeType)
            watcher->property("__Tp_Qt_CaptchaType").toUInt(),
            watcher->property("__Tp_Qt_CaptchaID").toInt());

    mPriv->captchas.append(captchaItem);

    --mPriv->captchasLeft;

    if (!mPriv->captchasLeft) {
        setFinished();
    }

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
 *
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
 *
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
