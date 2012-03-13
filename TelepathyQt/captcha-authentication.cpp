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

#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/captcha-authentication-internal.h>

#include "TelepathyQt/debug-internal.h"

#include "TelepathyQt/_gen/captcha-authentication.moc.hpp"
#include "TelepathyQt/_gen/captcha-authentication-internal.moc.hpp"

#include <TelepathyQt/Captcha>
#include <TelepathyQt/PendingCaptchas>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/WeakPtr>

namespace Tp
{

// ---
PendingCaptchaAnswer::PendingCaptchaAnswer(const QDBusPendingCall &call,
        const CaptchaAuthenticationPtr &object)
    : PendingOperation(object),
      mWatcher(new QDBusPendingCallWatcher(call, this)),
      mCaptcha(object),
      mChannel(mCaptcha->channel())
{
    debug() << "Calling Captcha.Answer";
    if (mWatcher->isFinished()) {
        onAnswerFinished();
    } else {
        // Connect the pending void
        connect(mWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(onAnswerFinished()));
    }
}

PendingCaptchaAnswer::~PendingCaptchaAnswer()
{
}

void PendingCaptchaAnswer::onAnswerFinished()
{
    QDBusReply<void> reply = mWatcher->reply();
    if (!reply.isValid()) {
        warning().nospace() << "Captcha.Answer failed with " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        return;
    }

    debug() << "Captcha.Answer returned successfully";

    // It might have been already opened - check
    if (mCaptcha->status() == CaptchaStatusLocalPending ||
            mCaptcha->status() == CaptchaStatusRemotePending) {
        debug() << "Awaiting captcha to be answered from server";
        // Wait until status becomes relevant
        connect(mCaptcha.data(),
                SIGNAL(statusChanged(Tp::CaptchaStatus)),
                SLOT(onCaptchaStatusChanged(Tp::CaptchaStatus)));
    } else {
        onCaptchaStatusChanged(mCaptcha->status());
    }
}

void PendingCaptchaAnswer::onCaptchaStatusChanged(Tp::CaptchaStatus status)
{
    if (status == CaptchaStatusSucceeded) {
        // Perfect. Close the channel now.
        connect(mChannel->requestClose(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onRequestCloseFinished(Tp::PendingOperation*)));
    } else if (status == CaptchaStatusFailed || status == CaptchaStatusTryAgain) {
        warning() << "Captcha status changed to" << status << ", failing";
        setFinishedWithError(mCaptcha->error(), mCaptcha->errorDetails().debugMessage());
    }
}

void PendingCaptchaAnswer::onRequestCloseFinished(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        // We cannot really fail just because the channel didn't close. Throw a warning instead.
        warning() << "Could not close the channel after a successful captcha answer!!" << operation->errorMessage();
    }

    setFinished();
}

PendingCaptchaCancel::PendingCaptchaCancel(const QDBusPendingCall &call,
        const CaptchaAuthenticationPtr &object)
    : PendingOperation(object),
      mWatcher(new QDBusPendingCallWatcher(call, this)),
      mCaptcha(object),
      mChannel(mCaptcha->channel())
{
    debug() << "Calling Captcha.Cancel";
    if (mWatcher->isFinished()) {
        onCancelFinished();
    } else {
        // Connect the pending void
        connect(mWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(onCancelFinished()));
    }
}

PendingCaptchaCancel::~PendingCaptchaCancel()
{
}

void PendingCaptchaCancel::onCancelFinished()
{
    QDBusReply<void> reply = mWatcher->reply();
    if (!reply.isValid()) {
        warning().nospace() << "Captcha.Answer failed with " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
        return;
    }

    debug() << "Captcha.Cancel returned successfully";

    // Perfect. Close the channel now.
    connect(mChannel->requestClose(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onRequestCloseFinished(Tp::PendingOperation*)));
}

void PendingCaptchaCancel::onRequestCloseFinished(Tp::PendingOperation *operation)
{
    if (operation->isError()) {
        // We cannot really fail just because the channel didn't close. Throw a warning instead.
        warning() << "Could not close the channel after a successful captcha cancel!!" << operation->errorMessage();
    }

    setFinished();
}

// --

CaptchaAuthentication::Private::Private(CaptchaAuthentication *parent)
    : parent(parent)
{
}

void CaptchaAuthentication::Private::extractCaptchaAuthenticationProperties(const QVariantMap &props)
{
    canRetry = qdbus_cast<bool>(props[QLatin1String("CanRetryCaptcha")]);
    status = static_cast<Tp::CaptchaStatus>(qdbus_cast<uint>(props[QLatin1String("Status")]));
}

/**
 * \class CaptchaAuthentication
 * \ingroup clientchannel
 * \headerfile TelepathyQt/captcha-authentication.h <TelepathyQt/CaptchaAuthentication>
 *
 * \brief The CaptchaAuthentication class exposes CaptchaAuthentication's features for channels implementing it.
 *
 * A ServerAuthentication channel can implement a CaptchaAuthentication interface: this class exposes all the features
 * this interface provides in a high-level fashion. It is a mechanism for retrieving a captcha challenge from
 * a connection manager and answering it.
 *
 * This class is meant to be used just during authentication phase. It is useful just for platform-level handlers
 * which are meant to handle authentication - if you are implementing a client which is meant to live in a
 * Telepathy-aware platform, you probably won't need to handle this unless you have very special needs.
 *
 * Note that CaptchaAuthentication cannot be instantiated directly, instead the accessor method from
 * ServerAuthenticationChannel (ServerAuthenticationChannel::captchaAuthentication) should be used.
 *
 * See \ref async_model, \ref shared_ptr
 */

CaptchaAuthentication::CaptchaAuthentication(const ChannelPtr &channel)
    : Object(),
      mPriv(new Private(this))
{
    mPriv->channel = channel;
}

/**
 * Class destructor.
 */
CaptchaAuthentication::~CaptchaAuthentication()
{
    delete mPriv;
}

/**
 * Return the channel associated with this captcha.
 *
 * CaptchaAuthentication is just a representation of an interface which can be implemented by
 * a ServerAuthentication channel. This function will return the channel implementing the interface
 * represented by this instance.
 *
 * Note that it is currently guaranteed the ChannelPtr returned by this function will be a ServerAuthenticationChannel.
 *
 * \return The channel implementing the CaptchaAuthentication interface represented by this instance.
 */
Tp::ChannelPtr CaptchaAuthentication::channel() const
{
    return ChannelPtr(mPriv->channel);
}

/**
 * Return whether this channel supports updating its captchas or not.
 *
 * Some protocols allow their captchas to be reloaded providing new data to the user; for example, in case
 * the image provided is not easily readable. This function checks if this instance supports such a feature.
 *
 * Note that in case this function returns \c true, requestCaptchas can be called safely after a failed answer attempt.
 *
 * \return \c true if a new captcha can be fetched from this channel, \c false otherwise.
 */
bool CaptchaAuthentication::canRetry() const
{
    return mPriv->canRetry;
}

/**
 * Return the current status of the captcha.
 *
 * \return The current status of the captcha.
 */
Tp::CaptchaStatus CaptchaAuthentication::status() const
{
    return mPriv->status;
}

/**
 * Return the code of the last error happened on the interface.
 *
 * \return An error code describing the last error occurred.
 * \sa errorDetails
 */
QString CaptchaAuthentication::error() const
{
    return mPriv->error;
}

/**
 * Return the details of the last error happened on the interface.
 *
 * \return Further details describing the last error occurred.
 * \sa error
 */
Connection::ErrorDetails CaptchaAuthentication::errorDetails() const
{
    return Connection::ErrorDetails(mPriv->errorDetails);
}

/**
 * Request captcha challenges from the connection manager.
 *
 * Even if most protocols usually provide a single captcha challenge (OCR), for a variety
 * of reasons some of them could provide a number of different challenge types, requiring
 * one or more of them to be answered.
 *
 * This method initiates a request to the connection manager for obtaining the most compatible captcha
 * challenges available. It allows to supply a number of supported mimetypes and types, so that the
 * request will fail if the CM is unable to provide a challenge compatible with what the handler supports,
 * or will provide the best one available otherwise.
 *
 * Please note that all the challenges returned by this request must be answered in order for the authentication
 * to succeed.
 *
 * Note that if the CM supports retrying the captcha, this function can also be used to load a new set of captchas.
 * In general, if canRetry returns true, one can expect this function to always return a different set
 * of challenges which invalidates any other obtained previously.
 *
 * \param preferredMimeTypes A list of mimetypes supported by the handler, or an empty list if every
 *                           mimetype can be supported.
 * \param preferredTypes A list of challenge types supported by the handler.
 * \return A PendingCaptchas which will emit PendingCaptchas::finished when the request has been completed and all the payloads
 *         have been downloaded.
 * \sa canRetry
 * \sa cancel
 * \sa answer
 */
PendingCaptchas *CaptchaAuthentication::requestCaptchas(const QStringList &preferredMimeTypes,
        ChallengeTypes preferredTypes)
{
    // The captcha should be either LocalPending or TryAgain
    if (status() != CaptchaStatusLocalPending && status() != CaptchaStatusTryAgain) {
        warning() << "Status must be local pending or try again";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"), CaptchaAuthenticationPtr(this));
    }

    ChannelPtr serverAuthChannel = ChannelPtr(mPriv->channel);

    return new PendingCaptchas(
            serverAuthChannel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->GetCaptchas(),
            preferredMimeTypes,
            preferredTypes,
            CaptchaAuthenticationPtr(this));
}

/**
 * Overloaded function. Convenience method when just a single captcha requires to be answered.
 *
 * Note that you need to answer only the last set of challenges returned, in case requestCaptchas
 * was invoked multiple times.
 *
 * Please note that if this operation succeeds, the channel will be closed right after.
 *
 * \param id The id of the challenge being answered.
 * \param response The answer of this challenge.
 * \return A PendingOperation which will emit PendingOperation::finished upon the outcome of the answer procedure.
 *         Upon success, the operation will complete once the channel is closed.
 * \sa requestCaptchas
 * \sa answer
 */
Tp::PendingOperation *CaptchaAuthentication::answer(uint id, const QString &response)
{
    QMap<uint, QString> answers;
    answers.insert(id, response);
    return answer(answers);
}

/**
 * Answer a set of challenges.
 *
 * Challenges obtained with requestCaptchas should be answered using this method. Note that
 * every challenge returned by the last invocation of requestCaptchas must be answered
 * in order for the operation to succeed.
 *
 * Usually, most protocols will require just a single challenge to be answered: if that is the
 * case, you can use the convenience overload.
 *
 * Note that you need to answer only the last set of challenges returned, in case requestCaptchas
 * was invoked multiple times.
 *
 * Please note that if this operation succeeds, the channel will be closed right after.
 *
 * \param response A set of answers mapped by their id to the challenges obtained previously
 * \return A PendingOperation which will emit PendingOperation::finished upon the outcome of the answer procedure.
 *         Upon success, the operation will complete once the channel is closed.
 * \sa requestCaptchas
 * \sa answer
 */
Tp::PendingOperation *CaptchaAuthentication::answer(const Tp::CaptchaAnswers &response)
{
    // The captcha should be LocalPending or TryAgain
    if (status() != CaptchaStatusLocalPending) {
        warning() << "Status must be local pending";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"), CaptchaAuthenticationPtr(this));
    }

    ChannelPtr serverAuthChannel = ChannelPtr(mPriv->channel);

    return new PendingCaptchaAnswer(serverAuthChannel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->AnswerCaptchas(response),
            CaptchaAuthenticationPtr(this));
}

/**
 * Cancel the current challenge.
 *
 * Please note that if this operation succeeds, the channel will be closed right after.
 *
 * Note that this function has not the same semantics as retry. The status of the CaptchaAuthentication
 * will change to Failed even if the channel supports retrying. This function should be called
 * only if the user refuses to answer any challenge. Instead, if the user wishes to retry,
 * you should just call requestCaptchas one more time.
 *
 * \param reason The reason why the challenge has been cancelled.
 * \param message A message detailing the cancel reason.
 * \return A PendingOperation which will emit PendingOperation::finished upon the outcome of the cancel procedure.
 *         Upon success, the operation will complete once the channel is closed.
 * \sa requestCaptchas
 */
Tp::PendingOperation *CaptchaAuthentication::cancel(CaptchaCancelReason reason,
        const QString &message)
{
    ChannelPtr serverAuthChannel = ChannelPtr(mPriv->channel);

    return new PendingCaptchaCancel(serverAuthChannel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->CancelCaptcha(
                    reason, message),
            CaptchaAuthenticationPtr(this));
}

void CaptchaAuthentication::onPropertiesChanged(const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);

    if (changedProperties.contains(QLatin1String("CaptchaStatus"))) {
        mPriv->status = static_cast<Tp::CaptchaStatus>(changedProperties.value(QLatin1String("CaptchaStatus")).value<uint>());
        emit statusChanged(mPriv->status);
    }
    if (changedProperties.contains(QLatin1String("CaptchaErrorDetails"))) {
        mPriv->errorDetails = changedProperties.value(QLatin1String("CaptchaErrorDetails")).toMap();
    }
    if (changedProperties.contains(QLatin1String("CaptchaError"))) {
        mPriv->error = changedProperties.value(QLatin1String("CaptchaError")).toString();
    }
}

/**
 * \fn void CaptchaAuthentication::statusChanged(Tp::CaptchaStatus status)
 *
 * Emitted when the value of status() changes.
 *
 * \sa status The new status of this CaptchaAuthentication.
 */

} // Tp
