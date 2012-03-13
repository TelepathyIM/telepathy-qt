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

#ifndef _TelepathyQt_captcha_authentication_h_HEADER_GUARD_
#define _TelepathyQt_captcha_authentication_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>
#include <TelepathyQt/Connection>

#include <TelepathyQt/Global>
#include <TelepathyQt/SharedPtr>

namespace Tp
{

class PendingCaptchaAnswer;
class PendingCaptchaCancel;
class PendingCaptchas;

class TP_QT_EXPORT CaptchaAuthentication : public Tp::Object
{
    Q_OBJECT
    Q_DISABLE_COPY(CaptchaAuthentication)

public:
    enum ChallengeType {
        NoChallenge = 0,
        OCRChallenge = 1,
        AudioRecognitionChallenge = 2,
        PictureQuestionChallenge = 4,
        PictureRecognitionChallenge = 8,
        TextQuestionChallenge = 16,
        SpeechQuestionChallenge = 32,
        SpeechRecognitionChallenge = 64,
        VideoQuestionChallenge = 128,
        VideoRecognitionChallenge = 256,
        UnknownChallenge = 32768
    };
    Q_DECLARE_FLAGS(ChallengeTypes, ChallengeType)

    virtual ~CaptchaAuthentication();

    ChannelPtr channel() const;

    bool canRetry() const;
    Tp::CaptchaStatus status() const;

    QString error() const;
    Connection::ErrorDetails errorDetails() const;

    Tp::PendingCaptchas *requestCaptchas(const QStringList &preferredMimeTypes = QStringList(),
            ChallengeTypes preferredTypes = ~ChallengeTypes(NoChallenge));
    Tp::PendingOperation *answer(uint id, const QString &answer);
    Tp::PendingOperation *answer(const Tp::CaptchaAnswers &response);

Q_SIGNALS:
    void statusChanged(Tp::CaptchaStatus status);

public Q_SLOTS:
    Tp::PendingOperation *cancel(Tp::CaptchaCancelReason reason,
            const QString &message = QString());

private Q_SLOTS:
    TP_QT_NO_EXPORT void onPropertiesChanged(const QVariantMap &changedProperties,
            const QStringList &invalidatedProperties);

private:
    TP_QT_NO_EXPORT CaptchaAuthentication(const ChannelPtr &parent);

    friend class ServerAuthenticationChannel;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // namespace Tp

Q_DECLARE_OPERATORS_FOR_FLAGS(Tp::CaptchaAuthentication::ChallengeTypes)

#endif
