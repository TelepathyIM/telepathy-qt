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

#include <TelepathyQt/Captcha>

namespace Tp
{

struct TP_QT_NO_EXPORT Captcha::Private : public QSharedData
{
    Private();
    Private(const Captcha::Private &other);
    ~Private();

    QString mimeType;
    QString label;
    QByteArray captchaData;
    CaptchaAuthentication::ChallengeType type;
    uint id;
};

Captcha::Private::Private()
    : type(CaptchaAuthentication::NoChallenge),
      id(0)
{
}

Captcha::Private::Private(const Private &other)
    : QSharedData(other),
      mimeType(other.mimeType),
      label(other.label),
      captchaData(other.captchaData),
      type(other.type),
      id(other.id)
{
}

Captcha::Private::~Private()
{
}

/**
 * \class Captcha
 * \ingroup client
 * \headerfile TelepathyQt/captcha.h <TelepathyQt/Captcha>
 *
 * \brief The Captcha class represents a Captcha ready to be answered.
 *
 * It exposes all the parameters needed for a handler to present the user with a captcha.
 *
 * Please note this class is meant to be read-only. It is usually created by PendingCaptchas
 * once a Captcha request operation succeeds.
 *
 * Please note that this class is implicitly shared.
 */

/**
 * Default constructor.
 */
Captcha::Captcha()
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
        const QByteArray &data, CaptchaAuthentication::ChallengeType type, uint id)
    : mPriv(new Captcha::Private)
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

Captcha &Captcha::operator=(const Captcha &rhs)
{
    if (this==&rhs) {
        //Protect against self-assignment
        return *this;
    }

    mPriv = rhs.mPriv;
    return *this;
}

/**
 * Return the mimetype of the captcha.
 *
 * \return The captcha's mimetype.
 * \sa data()
 */
QString Captcha::mimeType() const
{
    if (!isValid()) {
        return QString();
    }

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
    if (!isValid()) {
        return QString();
    }

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
    if (!isValid()) {
        return QByteArray();
    }

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
    if (!isValid()) {
        return CaptchaAuthentication::NoChallenge;
    }

    return mPriv->type;
}

/**
 * Return the id of the captcha. This parameter should be used to identify
 * the captcha when answering its challenge.
 *
 * \return The captcha's id.
 * \sa CaptchaAuthentication::answer()
 */
uint Captcha::id() const
{
    if (!isValid()) {
        return 0;
    }

    return mPriv->id;
}

}
