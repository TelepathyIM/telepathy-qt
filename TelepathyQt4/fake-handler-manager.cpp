#include "fake-handler-manager.h"

#include "TelepathyQt4/_gen/fake-handler-manager.moc.hpp"

#include <TelepathyQt4/Channel>

using namespace Tp;

FakeHandler::FakeHandler()
: QObject(0)
, mNumChannels(0)
{
}

void FakeHandler::addChannel(const ChannelPtr &channel,
                             const ClientRegistrarPtr &registrar)
{
    if (mNumChannels <= 0) {
        mRegistrar = registrar;
    }
    mNumChannels++;
    connect(channel.data(), SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            this, SLOT(onChannelInvalidated()));
}

void FakeHandler::onChannelInvalidated()
{
    mNumChannels--;
    if (mNumChannels <= 0) {
        mRegistrar.reset();
    }
}

FakeHandlerManager::FakeHandlerManager()
: QObject(0)
{
}

void FakeHandlerManager::registerHandler(const QString &connectionName,
                                         const ChannelPtr &channel,
                                         const ClientRegistrarPtr &registrar)
{
    FakeHandler *handler = mFakeHandlers.value(connectionName, 0);
    if (!handler) {
        handler = new FakeHandler;
        mFakeHandlers.insert(connectionName, handler);
    }
    handler->addChannel(channel, registrar);
}

FakeHandlerManager *FakeHandlerManager::instance()
{
    static FakeHandlerManager sInstance;
    return &sInstance;
}
