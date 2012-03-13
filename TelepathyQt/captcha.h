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

#ifndef _TelepathyQt_captcha_h_HEADER_GUARD_
#define _TelepathyQt_captcha_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/CaptchaAuthentication>

namespace Tp
{

class PendingCaptchas;

class TP_QT_EXPORT Captcha {
public:
    Captcha();
    Captcha(const Captcha &other);
    ~Captcha();

    bool isValid() const { return mPriv.constData() != 0; }

    Captcha &operator=(const Captcha &rhs);

    QString mimeType() const;
    QString label() const;
    QByteArray data() const;
    CaptchaAuthentication::ChallengeType type() const;
    uint id() const;

private:
    struct Private;
    friend struct Private;
    friend class PendingCaptchas;

    Captcha(const QString &mimeType, const QString &label, const QByteArray &data,
            CaptchaAuthentication::ChallengeType type, uint id);

    QSharedDataPointer<Captcha::Private> mPriv;
};

}

#endif
