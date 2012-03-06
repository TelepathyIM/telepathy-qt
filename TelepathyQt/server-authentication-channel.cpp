/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt/ServerAuthenticationChannel>

#include "TelepathyQt/_gen/server-authentication-channel.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/CaptchaAuthentication>
#include <TelepathyQt/captcha-authentication-internal.h>
#include <TelepathyQt/PendingVariantMap>

namespace Tp
{

struct TP_QT_NO_EXPORT ServerAuthenticationChannel::Private
{
    Private(ServerAuthenticationChannel *parent);

    static void introspectServerAuthentication(ServerAuthenticationChannel::Private *self);

    void extractServerAuthenticationProperties(const QVariantMap &props);

    // Public object
    ServerAuthenticationChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection
    Type type;
    CaptchaAuthenticationPtr captchaAuthentication;
};

ServerAuthenticationChannel::Private::Private(ServerAuthenticationChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper())
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableServerAuthentication(
            QSet<uint>() << 0,                                                          // makesSenseForStatuses
            Features() << Channel::FeatureCore,                                         // dependsOnFeatures (core)
            QStringList(),                                                              // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &ServerAuthenticationChannel::Private::introspectServerAuthentication,
            this);
    introspectables[ServerAuthenticationChannel::FeatureCore] = introspectableServerAuthentication;

    readinessHelper->addIntrospectables(introspectables);
}

void ServerAuthenticationChannel::Private::introspectServerAuthentication(ServerAuthenticationChannel::Private *self)
{
    ServerAuthenticationChannel *parent = self->parent;

    if (parent->hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION)) {
        self->type = ServerAuthenticationChannel::CaptchaType;

        self->captchaAuthentication =
                CaptchaAuthenticationPtr(new CaptchaAuthentication(ChannelPtr(parent)));

        Client::ChannelInterfaceCaptchaAuthenticationInterface *captchaAuthenticationInterface =
                parent->interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>();

        captchaAuthenticationInterface->setMonitorProperties(true);

        QObject::connect(captchaAuthenticationInterface,
                SIGNAL(propertiesChanged(QVariantMap,QStringList)),
                parent->mPriv->captchaAuthentication.data(),
                SLOT(onPropertiesChanged(QVariantMap,QStringList)));

        PendingVariantMap *pvm = captchaAuthenticationInterface->requestAllProperties();
        parent->connect(pvm,
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(gotCaptchaAuthenticationProperties(Tp::PendingOperation*)));
    } else if (parent->hasInterface(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION)) {
        self->type = ServerAuthenticationChannel::SaslType;
        // TODO: Stuff for SASL should go here
    } else {
        // This should never happen, but still
        self->readinessHelper->setIntrospectCompleted(ServerAuthenticationChannel::FeatureCore, false,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("No known interfaces are advertised by this ServerAuthenticationChannel"));
    }
}

/**
 * \class ServerAuthenticationChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/ServerAuthentication-channel.h <TelepathyQt/ServerAuthenticationChannel>
 *
 * \brief The ServerAuthenticationChannel class is a base class for all ServerAuthentication types.
 *
 * A ServerAuthentication is a mechanism for arbitrary data transfer between two or more IM users,
 * used to allow applications on the users' systems to communicate without having
 * to establish network connections themselves.
 *
 * Note that ServerAuthenticationChannel should never be instantiated directly, instead one of its
 * subclasses (e.g. IncomingStreamServerAuthenticationChannel or OutgoingStreamServerAuthenticationChannel) should be used.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * ServerAuthenticationChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * ServerAuthenticationChannel methods.
 * See specific methods documentation for more details.
 */
const Feature ServerAuthenticationChannel::FeatureCore = Feature(QLatin1String(ServerAuthenticationChannel::staticMetaObject.className()), 0);

/**
 * Create a new ServerAuthenticationChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A ServerAuthenticationChannelPtr object pointing to the newly created
 *         ServerAuthenticationChannel object.
 */
ServerAuthenticationChannelPtr ServerAuthenticationChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return ServerAuthenticationChannelPtr(new ServerAuthenticationChannel(connection, objectPath,
            immutableProperties));
}

/**
 * Construct a new ServerAuthenticationChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type, if any. The corresponding introspectable should
 *                    depend on ServerAuthenticationChannel::FeatureCore.
 */
ServerAuthenticationChannel::ServerAuthenticationChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : Channel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
ServerAuthenticationChannel::~ServerAuthenticationChannel()
{
    delete mPriv;
}

ServerAuthenticationChannel::Type ServerAuthenticationChannel::type() const
{
    if (!isReady(ServerAuthenticationChannel::FeatureCore)) {
        warning() << "ServerAuthenticationChannel::type() used with FeatureCore not ready";
        return UnknownType;
    }

    return mPriv->type;
}

/**
 * Return the CaptchaAuthentication object for this channel, if the channel implements
 * the CaptchaAuthentication interface and is a ServerAuthentication Channel.
 * Note that FeatureCore must be ready for this method to return a meaningful value.
 *
 * \return A shared pointer to the object representing the CaptchaAuthentication interface,
 *         or a null shared pointer if the feature is not ready yet.
 */
CaptchaAuthenticationPtr ServerAuthenticationChannel::captchaAuthentication() const
{
    if (!isReady(ServerAuthenticationChannel::FeatureCore)) {
        warning() << "ServerAuthenticationChannel::captchaAuthentication() used with FeatureCore not ready";
        return CaptchaAuthenticationPtr();
    }

    return mPriv->captchaAuthentication;
}

void ServerAuthenticationChannel::gotCaptchaAuthenticationProperties(Tp::PendingOperation *op)
{
    if (!op->isError()) {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap *>(op);

        mPriv->captchaAuthentication->mPriv->extractCaptchaAuthenticationProperties(pvm->result());

        debug() << "Got reply to Properties::GetAll(CaptchaAuthentication)";
        mPriv->readinessHelper->setIntrospectCompleted(ServerAuthenticationChannel::FeatureCore, true);
    } else {
        warning().nospace() << "Properties::GetAll(CaptchaAuthentication) failed "
            "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(ServerAuthenticationChannel::FeatureCore, false,
                op->errorName(), op->errorMessage());
    }
}

} // Tp
