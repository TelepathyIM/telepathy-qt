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

#ifndef _TelepathyQt_server_authentication_channel_h_HEADER_GUARD_
#define _TelepathyQt_server_authentication_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/Channel>

namespace Tp
{

class TP_QT_EXPORT ServerAuthenticationChannel : public Channel
{
    Q_OBJECT
    Q_DISABLE_COPY(ServerAuthenticationChannel)

public:
    static const Feature FeatureCore;

    static ServerAuthenticationChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~ServerAuthenticationChannel();

    CaptchaAuthenticationPtr captchaAuthentication() const;

    // TODO: Add something for SASL here as well

    bool hasCaptchaInterface() const;
    // TODO: Enable when SASL high-level support is in
    // bool hasSaslInterface() const;

protected:
    ServerAuthenticationChannel(const ConnectionPtr &connection, const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = ServerAuthenticationChannel::FeatureCore);

private Q_SLOTS:
    TP_QT_NO_EXPORT void gotCaptchaAuthenticationProperties(Tp::PendingOperation *op);
    TP_QT_NO_EXPORT void gotServerAuthenticationProperties(Tp::PendingOperation *op);

private:
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}

#endif
