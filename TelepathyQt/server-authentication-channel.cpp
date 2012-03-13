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

    static void introspectMain(ServerAuthenticationChannel::Private *self);

    void extractServerAuthenticationProperties(const QVariantMap &props);

    // Public object
    ServerAuthenticationChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection
    QString authMethod;
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
            (ReadinessHelper::IntrospectFunc) &ServerAuthenticationChannel::Private::introspectMain,
            this);
    introspectables[ServerAuthenticationChannel::FeatureCore] = introspectableServerAuthentication;

    readinessHelper->addIntrospectables(introspectables);
}

void ServerAuthenticationChannel::Private::introspectMain(ServerAuthenticationChannel::Private *self)
{
    ServerAuthenticationChannel *parent = self->parent;

    Client::ChannelTypeServerAuthenticationInterface *serverAuthenticationInterface =
                parent->interface<Client::ChannelTypeServerAuthenticationInterface>();

    PendingVariantMap *pvm = serverAuthenticationInterface->requestAllProperties();
    parent->connect(pvm,
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(gotServerAuthenticationProperties(Tp::PendingOperation*)));
}

/**
 * \class ServerAuthenticationChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt/server-authentication-channel.h <TelepathyQt/ServerAuthenticationChannel>
 *
 * \brief The ServerAuthenticationChannel class is a base class for all ServerAuthentication types.
 *
 * A ServerAuthentication is a mechanism for a connection to perform an authentication operation.
 * Such an authentication can happen in several ways (at the moment, only Captcha is supported) - this
 * channel will expose a high-level object representing the requested method, allowing a handler to carry on
 * the authentication procedure.
 *
 * Note that when an authentication procedure succeeds, you can expect this channel to be closed automatically.
 * Please refer to the methods' implementation docs for more details about this.
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

/**
 * Return whether this ServerAuthenticationChannel implements Captcha as its authentication mechanism.
 * Should this be true, captchaAuthentication() can be safely accessed.
 *
 * This method requires ServerAuthenticationChannel::FeatureCore to be ready.
 *
 * \return \c true if this channel implements the Captcha interface, \c false otherwise.
 * \sa captchaAuthentication
 */
bool ServerAuthenticationChannel::hasCaptchaInterface() const
{
    if (!isReady(ServerAuthenticationChannel::FeatureCore)) {
        warning() << "ServerAuthenticationChannel::hasCaptchaInterface() used with FeatureCore not ready";
        return false;
    }

    return mPriv->authMethod == TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION;
}

/*
 * Return whether this ServerAuthenticationChannel implements Sasl as its authentication mechanism.
 *
 * This method requires ServerAuthenticationChannel::FeatureCore to be ready.
 *
 * \return \c true if this channel implements the Sasl interface, \c false otherwise.
 *
 * TODO: Enable when high-level SASL Support will be in place
bool ServerAuthenticationChannel::hasSaslInterface() const
{
    if (!isReady(ServerAuthenticationChannel::FeatureCore)) {
        warning() << "ServerAuthenticationChannel::hasSaslInterface() used with FeatureCore not ready";
        return false;
    }

    return mPriv->authMethod == TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION;
}*/

/**
 * Return the CaptchaAuthentication object for this channel, if the channel implements
 * the CaptchaAuthentication interface and is a ServerAuthentication Channel.
 *
 * Note that this method will return a meaningful value only if hasCaptchaInterface()
 * returns \c true.
 *
 * This method requires ServerAuthenticationChannel::FeatureCore to be ready.
 *
 * \return A shared pointer to the object representing the CaptchaAuthentication interface,
 *         or a null shared pointer if the feature is not ready yet or the channel does not
 *         implement Captcha interface.
 * \sa hasCaptchaInterface
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

void ServerAuthenticationChannel::gotServerAuthenticationProperties(Tp::PendingOperation *op)
{
    if (!op->isError()) {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap *>(op);

        debug() << "Got reply to Properties::GetAll(ServerAuthentication)";
        mPriv->authMethod = qdbus_cast<QString>(pvm->result()[QLatin1String("AuthenticationMethod")]);

        if (mPriv->authMethod == TP_QT_IFACE_CHANNEL_INTERFACE_CAPTCHA_AUTHENTICATION) {
            mPriv->captchaAuthentication =
                    CaptchaAuthenticationPtr(new CaptchaAuthentication(ChannelPtr(this)));

            Client::ChannelInterfaceCaptchaAuthenticationInterface *captchaAuthenticationInterface =
                    interface<Client::ChannelInterfaceCaptchaAuthenticationInterface>();

            captchaAuthenticationInterface->setMonitorProperties(true);

            QObject::connect(captchaAuthenticationInterface,
                    SIGNAL(propertiesChanged(QVariantMap,QStringList)),
                    mPriv->captchaAuthentication.data(),
                    SLOT(onPropertiesChanged(QVariantMap,QStringList)));

            PendingVariantMap *pvm = captchaAuthenticationInterface->requestAllProperties();
            connect(pvm,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(gotCaptchaAuthenticationProperties(Tp::PendingOperation*)));
        } else {
            // We have nothing else to do here at the moment
            mPriv->readinessHelper->setIntrospectCompleted(ServerAuthenticationChannel::FeatureCore, true);
        }
    } else {
        warning().nospace() << "Properties::GetAll(ServerAuthentication) failed "
            "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(ServerAuthenticationChannel::FeatureCore, false,
                op->errorName(), op->errorMessage());
    }
}

} // Tp
