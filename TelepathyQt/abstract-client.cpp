/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009-2010 Nokia Corporation
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

#include <TelepathyQt/AbstractClient>

#include <QSharedData>
#include <QString>

#include <TelepathyQt/ChannelClassSpecList>

namespace Tp
{

struct TP_QT_NO_EXPORT AbstractClient::Private
{
    Private()
        : registered(false)
    {
    }

    bool registered;
};


/**
 * \class AbstractClient
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClient>
 *
 * \brief The AbstractClient class represents a Telepathy client.
 *
 * Clients are programs used to process channels, approving, handling or
 * observing them. User interface processes are the obvious example of clients,
 * but they can provide other functionality, such as address-book
 * synchronization, message logging, etc.
 *
 * Each client is either an observer, an approver, a handler, or some
 * combination of these.
 *
 * Clients can be activatable services (those with a D-Bus .service file)
 * so that they can run in response to channel creation, or non-activatable
 * services (those that do not register a D-Bus .service file
 * for their well-known name, but do request it at runtime) so
 * that they can process channels, but only if they are already
 * running - for instance, a full-screen media center application might do this.
 *
 * As an optimization, service-activatable clients should install a file
 * $XDG_DATA_DIRS/telepathy/clients/clientname.client containing a cached version
 * of their immutable properties. The syntax of these files is documented in the <a
 * href="http://telepathy.freedesktop.org/spec/org.freedesktop.Telepathy.Client.html">
 * Telepathy specification</a>.
 *
 * Non-activatable clients may install a .client file, but there's not much
 * point in them doing so.
 *
 * This is a base class and should not be used directly, use the
 * specialized classes AbstractClientObserver, AbstractClientApprover and
 * AbstractClientHandler instead.
 *
 * If the same process wants to be either a mix of observer, approver and
 * handler, or a combination of those it can multiple inherit the specialized
 * abstract classes.
 *
 * \sa AbstractClientObserver, AbstractClientApprover, AbstractClientHandler
 */

/**
 * Construct a new AbstractClient object.
 *
 * Note that this is a base class and should not be used directly, use the
 * specialized classes AbstractClientObserver, AbstractClientApprover and
 * AbstractClientHandler instead.
 */
AbstractClient::AbstractClient()
    : mPriv(new Private())
{
}

/**
 * Class destructor.
 */
AbstractClient::~AbstractClient()
{
    delete mPriv;
}

/**
 * Return whether this client is registered.
 *
 * \return \c true if registered, \c false otherwise.
 */
bool AbstractClient::isRegistered() const
{
    return mPriv->registered;
}

void AbstractClient::setRegistered(bool registered)
{
    mPriv->registered = registered;
}

struct TP_QT_NO_EXPORT AbstractClientObserver::Private
{
    Private(const ChannelClassList &channelFilter, bool shouldRecover)
        : channelFilter(channelFilter),
          shouldRecover(shouldRecover)
    {
    }

    ChannelClassList channelFilter;
    bool shouldRecover;
};

/**
 * \class AbstractClientObserver
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientObserver>
 *
 * \brief The AbstractClientObserver class represents a Telepathy observer.
 *
 * Observers are clients that monitor the creation of new channels.
 * This functionality can be used for things like message logging.
 *
 * Observers should not modify the state of a channel except via user
 * interaction.
 *
 * Observers must not carry out actions that exactly one process must take
 * responsibility for (e.g. acknowledging text messages, or carrying out
 * the actual file transfer), since arbitrarily many observers can be
 * activated for each channel. The handler is responsible for such tasks.
 *
 * Handlers may, of course, delegate responsibility for these tasks to other
 * clients (including those run as observers), but this must be done
 * explicitly via a request from the handler to the observer.
 *
 * Whenever a collection of new channels is signalled, the channel dispatcher
 * will notify all running or activatable observers whose filter indicates that
 * they are interested in some of the channels.
 *
 * Observers are activated for all channels in which they have registered an
 * interest - incoming, outgoing or automatically created - although of course
 * the filter property can be set to filter specific channels.
 *
 * To become an observer one should inherit AbstractClientObserver and
 * implement the pure virtual observeChannels() method. After that the object
 * representing the observer must be registered using
 * ClientRegistrar::registerClient().
 *
 * When new channels in which the observer has registered an interest are
 * announced, the method observeChannels() is invoked. All observers are
 * notified simultaneously.
 *
 * \section observer_usage_sec Usage
 *
 * \subsection observer_create_sec Implementing an observer
 *
 * \code
 *
 * class MyObserver : public AbstractClientObserver
 * {
 * public:
 *     MyObserver(const ChannelClassSpecList &channelFilter);
 *     ~MyObserver() { }
 *
 *     void observeChannels(const MethodInvocationContextPtr<> &context,
 *             const AccountPtr &account,
 *             const ConnectionPtr &connection,
 *             const QList<ChannelPtr> &channels,
 *             const ChannelDispatchOperationPtr &dispatchOperation,
 *             const QList<ChannelRequestPtr> &requestsSatisfied,
 *             const AbstractClientObserver::ObserverInfo &observerInfo);
 * };
 *
 * MyObserver::MyObserver(const ChannelClassSpecList &channelFilter)
 *     : AbstractClientObserver(channelFilter)
 * {
 * }
 *
 * void MyObserver::observeChannels(const MethodInvocationContextPtr<> &context,
 *         const AccountPtr &account,
 *         const ConnectionPtr &connection,
 *         const QList<ChannelPtr> &channels,
 *         const ChannelDispatchOperationPtr &dispatchOperation,
 *         const QList<ChannelRequestPtr> &requestsSatisfied,
 *         const AbstractClientObserver::ObserverInfo &observerInfo)
 * {
 *     // do something, log messages, ...
 *
 *     context->setFinished();
 * }
 *
 * \endcode
 *
 * \subsection observer_register_sec Registering an observer
 *
 * \code
 *
 * ClientRegistrar registrar = ClientRegistrar::create();
 * AbstractClientPtr observer = AbstractClientPtr::dynamicCast(
 *         SharedPtr<MyObserver>(new MyObserver(
 *             ChannelClassSpecList() << ChannelClassSpec::textChat())));
 * registrar->registerClient(observer, "myobserver");
 *
 * \endcode
 *
 * \sa AbstractClient
 */

/**
 * \class AbstractClientObserver::ObserverInfo
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientObserver>
 *
 * \brief The AbstractClientObserver::ObserverInfo class provides a wrapper
 * around the additional info about the channels passed to observeChannels().
 *
 * \sa AbstractClientObserver
 */

struct TP_QT_NO_EXPORT AbstractClientObserver::ObserverInfo::Private : public QSharedData
{
    Private(const QVariantMap &info)
        : info(info) {}

    QVariantMap info;
};

AbstractClientObserver::ObserverInfo::ObserverInfo(const QVariantMap &info)
    : mPriv(new Private(info))
{
}

AbstractClientObserver::ObserverInfo::ObserverInfo(const ObserverInfo &other)
    : mPriv(other.mPriv)
{
}

AbstractClientObserver::ObserverInfo::~ObserverInfo()
{
}

AbstractClientObserver::ObserverInfo &AbstractClientObserver::ObserverInfo::operator=(
        const ObserverInfo &other)
{
    if (this == &other) {
        return *this;
    }

    mPriv = other.mPriv;
    return *this;
}

QVariantMap AbstractClientObserver::ObserverInfo::allInfo() const
{
    return mPriv->info;
}

/**
 * Construct a new AbstractClientObserver object.
 *
 * \param channelFilter A specification of the channels in which this observer
 *                      is interested.
 * \param shouldRecover Whether upon the startup of this observer,
 *                      observeChannels() will be called for every already
 *                      existing channel matching its observerChannelFilter().
 */
AbstractClientObserver::AbstractClientObserver(
        const ChannelClassSpecList &channelFilter,
        bool shouldRecover)
    : mPriv(new Private(channelFilter.bareClasses(), shouldRecover))
      // The channel filter is converted here to the low-level class so that any warnings are
      // emitted immediately rather than only when the CD introspects this Client
{
}

/**
 * Class destructor.
 */
AbstractClientObserver::~AbstractClientObserver()
{
    delete mPriv;
}

/**
 * Return the property containing a specification of the channels that this
 * channel observer is interested. The observeChannels() method should be called
 * by the channel dispatcher whenever any of the newly created channels match
 * this description.
 *
 * See <a
 * href="http://telepathy.freedesktop.org/spec/org.freedesktop.Telepathy.Client.Observer.html#org.freedesktop.Telepathy.Client.Observer.ObserverChannelFilter">
 * the Telepathy specification</a> for documentation about the allowed
 * types and how to define filters.
 *
 * This property never changes while the observer process owns its client bus
 * name. If an observer wants to add extra channels to its list of interests at
 * runtime, it can register an additional client bus name using
 * ClientRegistrar::registerClient().
 * To remove those filters, it can release the bus name using
 * ClientRegistrar::unregisterClient().
 *
 * The same principle is applied to approvers and handlers.
 *
 * \return A specification of the channels that this channel observer is
 *         interested as a list of ChannelClassSpec objects.
 * \sa observeChannels()
 */
ChannelClassSpecList AbstractClientObserver::observerFilter() const
{
    return ChannelClassSpecList(mPriv->channelFilter);
}

/**
 * Return whether upon the startup of this observer, observeChannels()
 * will be called for every already existing channel matching its
 * observerChannelFilter().
 *
 * \param \c true if this observer observerChannels() will be called for every
 *        already existing channel matching its observerChannelFilter(),
 *        \c false otherwise.
 */
bool AbstractClientObserver::shouldRecover() const
{
    return mPriv->shouldRecover;
}

/**
 * \fn void AbstractClientObserver::observeChannels(
 *                  const MethodInvocationContextPtr<> &context,
 *                  const AccountPtr &account,
 *                  const ConnectionPtr &connection,
 *                  const QList<ChannelPtr> &channels,
 *                  const ChannelDispatchOperationPtr &dispatchOperation,
 *                  const QList<ChannelRequestPtr> &requestsSatisfied,
 *                  const ObserverInfo &observerInfo);
 *
 * Called by the channel dispatcher when channels in which the observer has
 * registered an interest are announced.
 *
 * If the announced channels contains channels that match the
 * observerChannelFilter(), and some that do not, then only a subset of the
 * channels (those that do match the filter) are passed to this method.
 *
 * If the channel dispatcher will split up the channels from a single
 * announcement and dispatch them separately (for instance because no
 * installed handler can handle all of them), it will call this method
 * several times.
 *
 * The observer must not call MethodInvocationContext::setFinished() until it
 * is ready for a handler for the channel to run (which may change the
 * channel's state). For instance the received \a context object should be
 * stored until this method is finished processing and then
 * MethodInvocationContext::setFinished() or
 * MethodInvocationContext::setFinishedWithError() should be called on the
 * received \a context object.
 *
 * Specialized observers must reimplement this method.
 *
 * \param context A MethodInvocationContextPtr object that must be used to
 *                indicate whether this method finished processing.
 * \param account The account with which the channels are associated.
 * \param connection The connection with which the channels are associated.
 * \param channels The channels to be observed.
 * \param dispatchOperation The dispatch operation for these channels.
 *                          The object will be invalid (DBusProxy::isValid()
 *                          will be false) if there is no dispatch
 *                          operation in place (because the channels were
 *                          requested, not incoming).
 *                          If the Observer calls
 *                          ChannelDispatchOperation::claim() or
 *                          ChannelDispatchOperation::handleWith() on this
 *                          object, it must be careful to avoid deadlock, since
 *                          these methods cannot return until the observer has
 *                          returned from observeChannels().
 * \param requestsSatisfied The requests satisfied by these channels.
 * \param observerInfo Additional information about these channels.
 */

struct TP_QT_NO_EXPORT AbstractClientApprover::Private
{
    Private(const ChannelClassList &channelFilter)
        : channelFilter(channelFilter)
    {
    }

    ChannelClassList channelFilter;
};

/**
 * \class AbstractClientApprover
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientApprover>
 *
 * \brief The AbstractClientApprover class represents a Telepathy approver.
 *
 * Approvers are clients that notify the user that new channels have been
 * created, and allow the user to accept or reject those channels.
 *
 * Approvers can also select which channel handler will be used for the channel,
 * for instance by offering the user a list of possible handlers rather than
 * just an accept/reject choice. However, the channel dispatcher must be able to
 * prioritize possible handlers on its own using some reasonable heuristic,
 * probably based on user configuration.
 *
 * It is possible (and useful) to have an approver and a channel handler in the
 * same process; this is particularly useful if a channel handler wants to claim
 * responsibility for particular channels itself.
 *
 * All approvers are notified simultaneously. For instance, in a desktop system,
 * there might be one approver that displays a notification-area icon, one that
 * is part of a contact list window and highlights contacts there, and one that
 * is part of a full-screen media player.
 *
 * Any approver can approve the handling of a channel dispatch operation with a
 * particular channel handler by calling the
 * ChannelDispatchOperation::handleWith() method. Approvers can also attempt to
 * claim channels by calling ChannelDispatchOperation::claim(). If this
 * succeeds, the approver may handle the channels itself (if it is also a
 * handler), or close the channels in order to reject them.
 *
 * Approvers wishing to reject channels should call the
 * ChannelDispatchOperation::claim() method, then (if it succeeds) close the
 * channels in any way they see fit.
 *
 * The first approver to reply gets its decision acted on; any other approvers
 * that reply at approximately the same time will get an error, indicating that
 * the channel has already been dealt with.
 *
 * Approvers should usually prompt the user and ask for confirmation, rather
 * than dispatching the channel to a handler straight away.
 *
 * To become an approver one should inherit AbstractClientApprover and
 * implement the pure virtual addDispatchOperation() method. After that the
 * object representing the approver must be registered using
 * ClientRegistrar::registerClient().
 *
 * When new channels in which the approver has registered an interest are
 * ready to be dispatched, the method addDispatchOperation() is invoked.
 * The new channels are represented by a ChannelDispatchOperation object, which
 * is passed to the addDispatchOperation() method.
 * All approvers are notified simultaneously.
 *
 * \section approver_usage_sec Usage
 *
 * \subsection approver_create_sec Implementing an approver
 *
 * \code
 *
 * class MyApprover : public AbstractClientApprover
 * {
 * public:
 *     MyApprover(const ChannelClassSpecSpecList &channelFilter);
 *     ~MyApprover() { }
 *
 *     void addDispatchOperation(const MethodInvocationContextPtr<> &context,
 *             const ChannelDispatchOperationPtr &dispatchOperation);
 * };
 *
 * MyApprover::MyApprover(const ChannelClassSpecList &channelFilter)
 *     : AbstractClientApprover(channelFilter)
 * {
 * }
 *
 * void MyApprover::addDispatchOperation(
 *         const MethodInvocationContextPtr<> &context,
 *         const ChannelDispatchOperationPtr &dispatchOperation)
 * {
 *     // do something with dispatchOperation
 *
 *     context->setFinished();
 * }
 *
 * \endcode
 *
 * \subsection approver_register_sec Registering an approver
 *
 * \code
 *
 * ClientRegistrar registrar = ClientRegistrar::create();
 * AbstractClientPtr approver = AbstractClientPtr::dynamicCast(
 *         SharedPtr<MyApprover>(new MyApprover(
 *             ChannelClassSpecList() << ChannelClassSpec::textChat())));
 * registrar->registerClient(approver, "myapprover");
 *
 * \endcode
 *
 * \sa AbstractClient
 */

/**
 * Construct a new AbstractClientApprover object.
 *
 * \param channelFilter A specification of the channels in which this approver
 *                      is interested.
 */
AbstractClientApprover::AbstractClientApprover(
        const ChannelClassSpecList &channelFilter)
    : mPriv(new Private(channelFilter.bareClasses()))
{
}

/**
 * Class destructor.
 */
AbstractClientApprover::~AbstractClientApprover()
{
    delete mPriv;
}

/**
 * Return the property containing a specification of the channels that this
 * channel approver is interested. The addDispatchOperation() method should be
 * called by the channel dispatcher whenever at least one of the channels in
 * a channel dispatch operation matches this description.
 *
 * This method works in exactly the same way as the
 * AbstractClientObserver::observerChannelFilter() method. In particular, the
 * returned value cannot change while the handler process continues to own the
 * corresponding client bus name.
 *
 * In the .client file, represented in the same way as observer channel
 * filter, the group is #TP_QT_IFACE_CLIENT_APPROVER followed by
 * ApproverChannelFilter instead.
 *
 * \return A specification of the channels that this channel approver is
 *         interested as a list of ChannelClassSpec objects.
 * \sa addDispatchOperation()
 */
ChannelClassSpecList AbstractClientApprover::approverFilter() const
{
    return ChannelClassSpecList(mPriv->channelFilter);
}

/**
 * \fn void AbstractClientApprover::addDispatchOperation(
 *                  const MethodInvocationContextPtr<> &context,
 *                  const ChannelDispatchOperationPtr &dispatchOperation);
 *
 * Called by the channel dispatcher when a dispatch operation in which the
 * approver has registered an interest is created, or when the approver starts
 * up while such channel dispatch operations already exist.
 *
 * The received \a context object should be stored until this
 * method is finished processing and then MethodInvocationContext::setFinished()
 * or MethodInvocationContext::setFinishedWithError() should be called on the
 * received \a context object.
 *
 * Specialized approvers must reimplement this method.
 *
 * \param context A MethodInvocationContextPtr object that must be used to
 *                indicate whether this method finished processing.
 * \param dispatchOperation The dispatch operation to be processed.
 */

struct TP_QT_NO_EXPORT AbstractClientHandler::Private
{
    Private(const ChannelClassList &channelFilter,
            const Capabilities &capabilities,
            bool wantsRequestNotification)
        : channelFilter(channelFilter),
          capabilities(capabilities),
          wantsRequestNotification(wantsRequestNotification)
    {
    }

    ChannelClassList channelFilter;
    Capabilities capabilities;
    bool wantsRequestNotification;
};

/**
 * \class AbstractClientHandler
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientHandler>
 *
 * \brief The AbstractClientHandler class represents a Telepathy handler.
 *
 * Handlers are the user interface for a channel. They turn an abstract
 * channel into something the user wants to see, like a text message
 * stream or an audio and/or video call.
 *
 * For its entire lifetime, each channel on a connection known to the channel
 * dispatcher is either being processed by the channel dispatcher, or being
 * handled by precisely one handler.
 *
 * Because each channel is only handled by one handler, handlers may perform
 * actions that only make sense to do once, such as acknowledging text messages,
 * transferring the file, etc.
 *
 * When a new incoming channel is offered to approvers by the channel
 * dispatcher, it also offers the approvers a list of all the running or
 * activatable handlers whose filter indicates that they are able to handle
 * the channel. The approvers can choose one of those channel handlers to
 * handle the channel.
 *
 * When a new outgoing channel appears, the channel dispatcher passes it to
 * an appropriate channel handler automatically.
 *
 * To become an handler one should inherit AbstractClientHandler and
 * implement the pure virtual bypassApproval() and handleChannels() methods.
 * After that the object representing the handler must be registered using
 * ClientRegistrar::registerClient().
 *
 * When new channels in which the approver has registered an interest are
 * ready to be handled, the method handleChannels() is invoked.
 *
 * \section handler_usage_sec Usage
 *
 * \subsection handler_create_sec Implementing a handler
 *
 * \code
 *
 * class MyHandler : public AbstractClientHandler
 * {
 * public:
 *     MyHandler(const ChannelClassSpecList &channelFilter);
 *     ~MyHandler() { }
 *
 *     void bypassApproval() const;
 *
 *     void handleChannels(const MethodInvocationContextPtr<> &context,
 *             const AccountPtr &account,
 *             const ConnectionPtr &connection,
 *             const QList<ChannelPtr> &channels,
 *             const QList<ChannelRequestPtr> &requestsSatisfied,
 *             const QDateTime &userActionTime,
 *             const AbstractClientHandler::HandlerInfo &handlerInfo);
 * };
 *
 * MyHandler::MyHandler(const ChannelClassSpecList &channelFilter)
 *     : AbstractClientHandler(channelFilter)
 * {
 * }
 *
 * void MyHandler::bypassApproval() const
 * {
 *     return false;
 * }
 *
 * void MyHandler::handleChannels(const MethodInvocationContextPtr<> &context,
 *         const AccountPtr &account,
 *         const ConnectionPtr &connection,
 *         const QList<ChannelPtr> &channels,
 *         const QList<ChannelRequestPtr> &requestsSatisfied,
 *         const QDateTime &userActionTime,
 *         const AbstractClientHandler::HandlerInfo &handlerInfo)
 * {
 *     // do something
 *
 *     context->setFinished();
 * }
 *
 * \endcode
 *
 * \subsection handler_register_sec Registering a handler
 *
 * \code
 *
 * ClientRegistrar registrar = ClientRegistrar::create();
 * AbstractClientPtr handler = AbstractClientPtr::dynamicCast(
 *         SharedPtr<MyHandler>(new MyHandler(
 *             ChannelClassSpecList() << ChannelClassSpec::textChat())));
 * registrar->registerClient(handler, "myhandler");
 *
 * \endcode
 *
 * \sa AbstractClient
 */

/**
 * \class AbstractClientHandler::Capabilities
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientHandler>
 *
 * \brief The AbstractClientHandler::Capabilities class provides a wrapper
 * around the capabilities of a handler.
 *
 * \sa AbstractClientHandler
 */

/**
 * \class AbstractClientHandler::HandlerInfo
 * \ingroup clientclient
 * \headerfile TelepathyQt/abstract-client.h <TelepathyQt/AbstractClientHandler>
 *
 * \brief The AbstractClientHandler::HandlerInfo class provides a wrapper
 * around the additional info about the channels passed to handleChannels().
 *
 * \sa AbstractClientHandler
 */

struct TP_QT_NO_EXPORT AbstractClientHandler::Capabilities::Private : public QSharedData
{
    Private(const QStringList &tokens)
        : tokens(QSet<QString>::fromList(tokens)) {}

    QSet<QString> tokens;
};

AbstractClientHandler::Capabilities::Capabilities(const QStringList &tokens)
    : mPriv(new Private(tokens))
{
}

AbstractClientHandler::Capabilities::Capabilities(const Capabilities &other)
    : mPriv(other.mPriv)
{
}

AbstractClientHandler::Capabilities::~Capabilities()
{
}

AbstractClientHandler::Capabilities &AbstractClientHandler::Capabilities::operator=(
        const Capabilities &other)
{
    if (this == &other) {
        return *this;
    }

    mPriv = other.mPriv;
    return *this;
}

bool AbstractClientHandler::Capabilities::hasToken(const QString &token) const
{
    return mPriv->tokens.contains(token);
}

void AbstractClientHandler::Capabilities::setToken(const QString &token)
{
    mPriv->tokens.insert(token);
}

void AbstractClientHandler::Capabilities::unsetToken(const QString &token)
{
    mPriv->tokens.remove(token);
}

QStringList AbstractClientHandler::Capabilities::allTokens() const
{
    return mPriv->tokens.toList();
}

struct TP_QT_NO_EXPORT AbstractClientHandler::HandlerInfo::Private : public QSharedData
{
    Private(const QVariantMap &info)
        : info(info) {}

    QVariantMap info;
};

AbstractClientHandler::HandlerInfo::HandlerInfo(const QVariantMap &info)
    : mPriv(new Private(info))
{
}

AbstractClientHandler::HandlerInfo::HandlerInfo(const HandlerInfo &other)
    : mPriv(other.mPriv)
{
}

AbstractClientHandler::HandlerInfo::~HandlerInfo()
{
}

AbstractClientHandler::HandlerInfo &AbstractClientHandler::HandlerInfo::operator=(
        const HandlerInfo &other)
{
    if (this == &other) {
        return *this;
    }

    mPriv = other.mPriv;
    return *this;
}

QVariantMap AbstractClientHandler::HandlerInfo::allInfo() const
{
    return mPriv->info;
}

/**
 * Construct a new AbstractClientHandler object.
 *
 * \param channelFilter A specification of the channels in which this observer
 *                      is interested.
 * \param wantsRequestNotification Whether this handler wants to receive channel
 *                                 requests notification via addRequest() and
 *                                 removeRequest().
 * \param capabilities The set of additional capabilities supported by this
 *                     handler.
 */
AbstractClientHandler::AbstractClientHandler(const ChannelClassSpecList &channelFilter,
        const Capabilities &capabilities,
        bool wantsRequestNotification)
    : mPriv(new Private(channelFilter.bareClasses(), capabilities, wantsRequestNotification))
{
}

/**
 * Class destructor.
 */
AbstractClientHandler::~AbstractClientHandler()
{
    delete mPriv;
}

/**
 * Return the property containing a specification of the channels that this
 * channel handler can deal with. It will be offered to approvers as a potential
 * channel handler for bundles that contain only suitable channels, or for
 * suitable channels that must be handled separately.
 *
 * This method works in exactly the same way as the
 * AbstractClientObserver::observerChannelFilter() method. In particular, the
 * returned value cannot change while the handler process continues to own the
 * corresponding client bus name.
 *
 * In the .client file, represented in the same way as observer channel
 * filter, the group is #TP_QT_IFACE_CLIENT_HANDLER suffixed
 * by HandlerChannelFilter instead.
 *
 * \return A specification of the channels that this channel handler can deal
 *         with as a list of ChannelClassSpec objects.
 */
ChannelClassSpecList AbstractClientHandler::handlerFilter() const
{
    return ChannelClassSpecList(mPriv->channelFilter);
}

/**
 * Return the set of additional capabilities supported by this handler.
 *
 * \return The capabilities as an AbstractClientHandler::Capabilities object.
 */
AbstractClientHandler::Capabilities AbstractClientHandler::handlerCapabilities() const
{
    return mPriv->capabilities;
}

/**
 * \fn bool AbstractClientHandler::bypassApproval() const;
 *
 * Return whether channels destined for this handler are automatically
 * handled, without invoking approvers.
 *
 * \return \c true if automatically handled, \c false otherwise.
 */

/**
 * \fn void AbstractClientHandler::handleChannels(
 *                  const MethodInvocationContextPtr<> &context,
 *                  const AccountPtr &account,
 *                  const ConnectionPtr &connection,
 *                  const QList<ChannelPtr> &channels,
 *                  const QList<ChannelRequestPtr> &requestsSatisfied,
 *                  const QDateTime &userActionTime,
 *                  const HandlerInfo &handlerInfo);
 *
 * Called by the channel dispatcher when this handler should handle these
 * channels, or when this handler should present channels that it is already
 * handling to the user (e.g. bring them into the foreground).
 *
 * Clients are expected to know what channels they're already handling, and
 * which channel object corresponds to which window or tab.
 *
 * After handleChannels() replies successfully by calling
 * MethodInvocationContext::setFinished(), the client process is considered
 * to be responsible for the channel until it its unique name disappears from
 * the bus.
 *
 * If a process has multiple client bus names - some temporary and some
 * long-lived - and drops one of the temporary bus names in order to reduce the
 * set of channels that it will handle, any channels that it is already handling
 * will remain unaffected.
 *
 * The received \a context object should be stored until this
 * method is finished processing and then MethodInvocationContext::setFinished()
 * or MethodInvocationContext::setFinishedWithError() should be called on the
 * received \a context object.
 *
 * Specialized handlers must reimplement this method.
 *
 * \param context A MethodInvocationContextPtr object that must be used to
 *                indicate whether this method finished processing.
 * \param account The account with which the channels are associated.
 * \param connection The connection with which the channels are associated.
 * \param channels The channels to be handled.
 * \param requestsSatisfied The requests satisfied by these channels.
 * \param userActionTime The time at which user action occurred, or 0 if this
 *                       channel is to be handled for some reason not involving
 *                       user action. Handlers should use this for
 *                       focus-stealing prevention, if applicable.
 * \param handlerInfo Additional information about these channels.
 */

/**
 * Return whether this handler wants to receive notification of channel requests
 * via addRequest() and removeRequest().
 *
 * This property is set by the constructor and cannot be changed after that.
 *
 * \return \c true if receiving channel requests notification is desired,
 *         \c false otherwise.
 */
bool AbstractClientHandler::wantsRequestNotification() const
{
    return mPriv->wantsRequestNotification;
}

/**
 * Called by the channel dispatcher to indicate that channels have been
 * requested, and that if the request is successful, they will probably be
 * handled by this handler.
 *
 * This allows the UI to start preparing to handle the channels in advance
 * (e.g. render a window with an "in progress" message), improving perceived
 * responsiveness.
 *
 * If the request succeeds and is given to the expected handler, the
 * requestsSatisfied parameter to handleChannels() can be used to match the
 * channel to a previous addRequest() call.
 *
 * This lets the UI direct the channels to the window that it already opened.
 *
 * If the request fails, the expected handler is notified by the channel
 * dispatcher calling its removeRequest() method.
 *
 * This lets the UI close the window or display the error.
 *
 * The channel dispatcher will attempt to ensure that handleChannels() is called
 * on the same handler that received addRequest(). If that isn't possible,
 * removeRequest() will be called on the handler that previously received
 * addRequest(), with the special error #TP_QT_ERROR_NOT_YOURS, which
 * indicates that some other handler received the channel instead.
 *
 * Expected handling is for the UI to close the window it previously opened.
 *
 * Specialized handlers that want to be notified of newly requested channel
 * should reimplement this method.
 *
 * \param channelRequest The newly created channel request.
 * \sa removeRequest()
 */
void AbstractClientHandler::addRequest(
        const ChannelRequestPtr &channelRequest)
{
    // do nothing, subclasses that want to listen requests should reimplement
    // this method
}

/**
 * Called by the ChannelDispatcher to indicate that a request previously passed
 * to addRequest() has failed and should be disregarded.
 *
 * Specialized handlers that want to be notified of removed channel requests
 * should reimplement this method.
 *
 * \param channelRequest The channel request that failed.
 * \param errorName The name of the D-Bus error with which the request failed.
 *                  If this is #TP_QT_ERROR_NOT_YOURS, this indicates that
 *                  the request succeeded, but all the resulting channels were
 *                  given to some other handler.
 * \param errorMessage Any message supplied with the D-Bus error.
 */
void AbstractClientHandler::removeRequest(
        const ChannelRequestPtr &channelRequest,
        const QString &errorName, const QString &errorMessage)
{
    // do nothing, subclasses that want to listen requests should reimplement
    // this method
}

} // Tp
