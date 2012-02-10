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

#include "captcha-authentication-internal.h"

#include <TelepathyQt/Debug>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingVariantMap>

#include "cli-channel.h"
#include "pending-captchas.h"
#include "channel-future.h"

namespace Tp
{

// ---
PendingCaptchaAnswer::PendingCaptchaAnswer(PendingVoid *answerOperation,
        const CaptchaAuthenticationPtr &object)
    : PendingOperation(object),
      mPriv(new Private(this))
{
    mPriv->captcha = object;

    qDebug() << "Calling Captcha.Answer";
    if (answerOperation->isFinished()) {
        onAnswerFinished(answerOperation);
    } else {
        // Connect the pending void
        connect(answerOperation, SIGNAL(finished(Tp::PendingOperation*)),
                this, SLOT(onAnswerFinished(Tp::PendingOperation*)));
    }
}

PendingCaptchaAnswer::~PendingCaptchaAnswer()
{
    delete mPriv;
}

void PendingCaptchaAnswer::onAnswerFinished(Tp::PendingOperation *op)
{
    if (op->isError()) {
        qWarning().nospace() << "Captcha.Answer failed with " <<
            op->errorName() << ": " << op->errorMessage();
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    qDebug() << "Captcha.Answer returned successfully";

    // It might have been already opened - check
    if (mPriv->captcha->status() == CaptchaStatusLocalPending ||
            mPriv->captcha->status() == CaptchaStatusRemotePending) {
        qDebug() << "Awaiting captcha to be answered from server";
        // Wait until status becomes relevant
        connect(mPriv->captcha.data(),
                SIGNAL(statusChanged(Tp::CaptchaStatus)),
                SLOT(onCaptchaStatusChanged(Tp::CaptchaStatus)));
    } else {
        onCaptchaStatusChanged(mPriv->captcha->status());
    }
}

void PendingCaptchaAnswer::onCaptchaStatusChanged(Tp::CaptchaStatus status)
{
    if (status == CaptchaStatusSucceeded) {
        // yeah
        setFinished();
    } else if (status == CaptchaStatusFailed || status == CaptchaStatusTryAgain) {
        setFinishedWithError(QDBusError());
    }
}

// --

CaptchaAuthentication::Private::Private(CaptchaAuthentication *parent)
    : parent(parent)
{
}

void CaptchaAuthentication::Private::extractCaptchaAuthenticationProperties(const QVariantMap &props)
{
    canRetry = qdbus_cast<bool>(props[QLatin1String("CanRetryCaptcha")]);
    status = (CaptchaStatus)qdbus_cast<uint>(props[QLatin1String("Status")]);
}

/**
 * \class CaptchaAuthenticationChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/CaptchaAuthentication-channel.h <TelepathyQt/CaptchaAuthenticationChannel>
 *
 * \brief The CaptchaAuthenticationChannel class is a base class for all CaptchaAuthentication types.
 *
 * A CaptchaAuthentication is a mechanism for arbitrary data transfer between two or more IM users,
 * used to allow applications on the users' systems to communicate without having
 * to establish network connections themselves.
 *
 * Note that CaptchaAuthenticationChannel should never be instantiated directly, instead one of its
 * subclasses (e.g. IncomingStreamCaptchaAuthenticationChannel or OutgoingStreamCaptchaAuthenticationChannel) should be used.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Construct a new CaptchaAuthenticationChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on CaptchaAuthenticationChannel::FeatureCore.
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
 * Return the parameters associated with this CaptchaAuthentication, if any.
 *
 * The parameters are populated when an outgoing CaptchaAuthentication is offered, but they are most useful in the
 * receiving end, where the parameters passed to the offer can be extracted for the CaptchaAuthentication's entire
 * lifetime to bootstrap legacy protocols. All parameters are passed unchanged.
 *
 * This method requires CaptchaAuthenticationChannel::FeatureCore to be ready.
 *
 * \return The parameters as QVariantMap.
 */
bool CaptchaAuthentication::canRetry() const
{
    if (!mPriv->channel->isReady(Tp::ChannelFuture::FeatureCaptcha)) {
        qWarning() << "CaptchaAuthenticationChannel::canRetry() used with FeatureCore not ready";
        return false;
    }

    return mPriv->canRetry;
}

Tp::CaptchaStatus CaptchaAuthentication::status() const
{
    if (!mPriv->channel->isReady(Tp::ChannelFuture::FeatureCaptcha)) {
        qWarning() << "CaptchaAuthenticationChannel::canRetry() used with FeatureCore not ready";
        return CaptchaStatusLocalPending;
    }

    return mPriv->status;
}

QString CaptchaAuthentication::lastError() const
{
    return mPriv->error;
}

Connection::ErrorDetails CaptchaAuthentication::lastErrorDetails() const
{
    return Connection::ErrorDetails(mPriv->errorDetails);
}

void CaptchaAuthentication::onPropertiesChanged(const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);

    if (changedProperties.contains(QLatin1String("Status"))) {
        mPriv->status = (Tp::CaptchaStatus)changedProperties.value(QLatin1String("Status")).value<uint>();
        Q_EMIT statusChanged(mPriv->status);
    }
    if (changedProperties.contains(QLatin1String("CaptchaErrorDetails"))) {
        mPriv->errorDetails = changedProperties.value(QLatin1String("CaptchaErrorDetails")).toMap();
    }
    if (changedProperties.contains(QLatin1String("CaptchaError"))) {
        mPriv->error = changedProperties.value(QLatin1String("CaptchaError")).toString();
    }
}

PendingCaptchas *CaptchaAuthentication::requestCaptchas(const QStringList &preferredMimeTypes,
        ChallengeTypes preferredTypes)
{
    if (!mPriv->channel->isReady(Tp::ChannelFuture::FeatureCaptcha)) {
        qWarning() << "CaptchaAuthenticationChannel::FeatureCore must be ready before "
                "calling request";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                CaptchaAuthenticationPtr(this));
    }

    // The captcha should be either LocalPending or TryAgain
    if (status() != CaptchaStatusLocalPending && status() != CaptchaStatusTryAgain) {
        qWarning() << "Status must be local pending or try again";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"), CaptchaAuthenticationPtr(this));
    }

    return new PendingCaptchas(
            mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->GetCaptchas(),
            preferredMimeTypes,
            preferredTypes,
            CaptchaAuthenticationPtr(this));
}

Tp::PendingOperation *CaptchaAuthentication::answer(uint id, const QString &response)
{
    QMap<uint, QString> answers;
    answers.insert(id, response);
    return answer(answers);
}

Tp::PendingOperation *CaptchaAuthentication::answer(const Tp::CaptchaAnswers &response)
{
    if (!mPriv->channel->isReady(Tp::ChannelFuture::FeatureCaptcha)) {
        qWarning() << "CaptchaAuthenticationChannel::FeatureCore must be ready before "
                "calling answer";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel not ready"),
                CaptchaAuthenticationPtr(this));
    }

    // The captcha should be LocalPending or TryAgain
    if (status() != CaptchaStatusLocalPending) {
        qWarning() << "Status must be local pending";
        return new PendingCaptchas(TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Channel busy"), CaptchaAuthenticationPtr(this));
    }

    PendingVoid *pv = new PendingVoid(
                mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->AnswerCaptchas(response),
                CaptchaAuthenticationPtr(this));

    return new PendingCaptchaAnswer(pv, CaptchaAuthenticationPtr(this));
}

Tp::PendingOperation *CaptchaAuthentication::cancel(CaptchaCancelReason reason,
        const QString &message)
{
    return new PendingVoid(
            mPriv->channel->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>()->CancelCaptcha(
                    reason, message),
            CaptchaAuthenticationPtr(this));
}

} // Tp
