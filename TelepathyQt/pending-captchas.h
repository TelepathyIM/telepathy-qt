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

#ifndef _TelepathyQt_pending_captcha_h_HEADER_GUARD_
#define _TelepathyQt_pending_captcha_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/PendingOperation>

#include <TelepathyQt/CaptchaAuthentication>

#include <QPair>

namespace Tp
{

class Captcha;

class TP_QT_EXPORT PendingCaptchas : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCaptchas)

public:
    virtual ~PendingCaptchas();

    Captcha captcha() const;
    QList<Captcha> captchaList() const;
    bool requiresMultipleCaptchas() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onChannelInvalidated(Tp::DBusProxy *proxy,
            const QString &errorName, const QString &errorMessage);
    TP_QT_NO_EXPORT void onGetCaptchasWatcherFinished(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onGetCaptchaDataWatcherFinished(QDBusPendingCallWatcher *watcher);

private:
    TP_QT_NO_EXPORT PendingCaptchas(const QDBusPendingCall &call,
            const QStringList &preferredMimeTypes,
            CaptchaAuthentication::ChallengeTypes preferredTypes,
            const CaptchaAuthenticationPtr &channel);
    TP_QT_NO_EXPORT PendingCaptchas(
            const QString &errorName, const QString &errorMessage,
            const CaptchaAuthenticationPtr &channel);

    struct Private;
    friend class CaptchaAuthentication;
    friend struct Private;
    Private *mPriv;
};

}

#endif // TP_PENDING_CAPTCHA_H
