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

#ifndef _TelepathyQt_captcha_authentication_internal_h_HEADER_GUARD_
#define _TelepathyQt_captcha_authentication_internal_h_HEADER_GUARD_

#include "captcha-authentication.h"
#include <TelepathyQt/PendingOperation>

namespace Tp
{

class PendingVoid;

class TP_QT_NO_EXPORT PendingCaptchaAnswer : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingCaptchaAnswer)

public:
    PendingCaptchaAnswer(PendingVoid *offerOperation,
            const CaptchaAuthenticationPtr &object);
    ~PendingCaptchaAnswer();

private Q_SLOTS:
    void onCaptchaStatusChanged(Tp::CaptchaStatus status);
    void onAnswerFinished(Tp::PendingOperation *operation);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

struct TP_QT_NO_EXPORT PendingCaptchaAnswer::Private
{
    Private(PendingCaptchaAnswer *parent) : parent(parent) {}

    // Public object
    PendingCaptchaAnswer *parent;

    CaptchaAuthenticationPtr captcha;
};

}

#endif
