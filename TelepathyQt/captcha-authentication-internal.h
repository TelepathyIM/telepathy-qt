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

#ifndef _TelepathyQt_captcha_authentication_internal_h_HEADER_GUARD_
#define _TelepathyQt_captcha_authentication_internal_h_HEADER_GUARD_

#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/PendingOperation>

namespace Tp
{

class PendingVoid;

class TP_QT_NO_EXPORT PendingCaptchaAnswer : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCaptchaAnswer)

public:
    PendingCaptchaAnswer(const QDBusPendingCall &call,
            const CaptchaAuthenticationPtr &object);
    ~PendingCaptchaAnswer();

private Q_SLOTS:
    void onCaptchaStatusChanged(Tp::CaptchaStatus status);
    void onAnswerFinished();
    void onRequestCloseFinished(Tp::PendingOperation *operation);

private:
    // Public object
    PendingCaptchaAnswer *mParent;

    QDBusPendingCallWatcher *mWatcher;
    CaptchaAuthenticationPtr mCaptcha;
    ChannelPtr mChannel;
};

class TP_QT_NO_EXPORT PendingCaptchaCancel : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCaptchaCancel)

public:
    PendingCaptchaCancel(const QDBusPendingCall &call,
            const CaptchaAuthenticationPtr &object);
    ~PendingCaptchaCancel();

private Q_SLOTS:
    void onCancelFinished();
    void onRequestCloseFinished(Tp::PendingOperation *operation);

private:
    // Public object
    PendingCaptchaCancel *mParent;

    QDBusPendingCallWatcher *mWatcher;
    CaptchaAuthenticationPtr mCaptcha;
    ChannelPtr mChannel;
};

struct TP_QT_NO_EXPORT CaptchaAuthentication::Private
{
    Private(CaptchaAuthentication *parent);

    void extractCaptchaAuthenticationProperties(const QVariantMap &props);

    // Public object
    CaptchaAuthentication *parent;
    WeakPtr<Channel> channel;

    // Introspection
    bool canRetry;
    CaptchaStatus status;
    QString error;
    QVariantMap errorDetails;
};

}

#endif
