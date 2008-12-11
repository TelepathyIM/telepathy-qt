/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 basysKom GmbH
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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
#include "TelepathyQt4/Prototype/StreamedMediaChannel.h"

#include <QDebug>
#include <QMetaProperty>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/Channel>

// ChannelHandlerInterface and StreamEngineInterface from stream-engine
// 0.5.x, which is currently used as the only media streaming backend.
// These interfaces are not really part of the Telepathy spec
#include <TelepathyQt4/Prototype/Client/ChannelHandler>
#include <TelepathyQt4/Prototype/Client/StreamEngine>

#include <TelepathyQt4/Prototype/ConnectionFacade.h>
#include <TelepathyQt4/Prototype/Constants>
#include <TelepathyQt4/Prototype/Contact.h>
#include <TelepathyQt4/Prototype/ContactManager.h>

#define ENABLE_DEBUG_OUTPUT_

using namespace TpPrototype;

class TpPrototype::StreamedMediaChannelPrivate
{
public:
    StreamedMediaChannelPrivate()
    { init(); }
    
    QPointer<TpPrototype::Contact>                                 m_pContact;
    QPointer<Telepathy::Client::ConnectionInterface>             m_pConnectionInterface;
    Telepathy::Client::ChannelTypeStreamedMediaInterface*        m_pStreamedMediaInterface;
    Telepathy::Client::ChannelInterfaceGroupInterface*           m_pGroupInterface;
    Telepathy::Client::ChannelInterfaceMediaSignallingInterface* m_pMediaSignallingInterface;
    Telepathy::Client::ChannelInterfaceCallStateInterface*       m_pCallStateInterface;

    // Interfaces of the telepathy stream engine:
    TpPrototype::Client::ChannelHandlerInterface*                m_pStreamEngineHandlerInterface;
    TpPrototype::Client::StreamEngineInterface*                  m_pStreamEngineInterface;
    
    bool m_isValid;

    uint localHandle()
    {
        int self_handle = ConnectionFacade::instance()->selfHandleForConnectionInterface( m_pConnectionInterface );
        Q_ASSERT( self_handle >= 0 );
        return (uint) self_handle;
    }

    QList<uint> handleListForContacts( QList<QPointer<TpPrototype::Contact> > contacts )
    {
        QList<uint> handle_list;

        foreach( TpPrototype::Contact* contact, contacts )
        {
            if ( !contact )
            { continue; }

            handle_list.append( contact->telepathyHandle() );
        }

        return handle_list ;
    }

    bool isStreamEngineAvailable()
    {
        if ( !m_pStreamEngineInterface )
        {
            qWarning() << "No " << TpPrototype::Client::StreamEngineInterface::staticInterfaceName() << "found!";
            return false;
        }
        return true;
    }

    void checkDBusCall( QDBusPendingReply<>& reply, const QString& methodName )
    {
        reply.waitForFinished();
        if ( !reply.isValid() )
        {
            QDBusError error = reply.error();
            qWarning() << methodName << ": error type:" << error.type()
                    << "error name:" << error.name()
                    << "error message:" << error.message();
        
        }
        return;
    }

    void cleanup()
    {
        delete m_pStreamedMediaInterface;
        m_pStreamedMediaInterface       = NULL;
        delete m_pCallStateInterface;
        m_pCallStateInterface           = NULL;
        delete m_pMediaSignallingInterface;
        m_pMediaSignallingInterface     = NULL;
        delete m_pStreamEngineHandlerInterface;
        m_pStreamEngineHandlerInterface = NULL;
        delete m_pStreamEngineInterface;
        m_pStreamEngineInterface        = NULL;
        delete m_pGroupInterface;
        m_pGroupInterface               = NULL;
        m_isValid                       = true;
    }

    
private:
    void init()
    {
        m_pContact                      = NULL;
        m_pConnectionInterface          = NULL;
        m_pStreamedMediaInterface       = NULL;
        m_pCallStateInterface           = NULL;
        m_pMediaSignallingInterface     = NULL;
        m_pStreamEngineHandlerInterface = NULL;
        m_pStreamEngineInterface        = NULL;
        m_pGroupInterface               = NULL;
        m_isValid                       = true;
    }
    

};


StreamedMediaChannel::StreamedMediaChannel( Contact* contact, Telepathy::Client::ConnectionInterface* connectionInterface, QObject* parent ):
        QObject( parent ),
        d(new StreamedMediaChannelPrivate())
{
    Telepathy::registerTypes();

    d->m_pContact = contact;
    d->m_pConnectionInterface = connectionInterface;

}

StreamedMediaChannel::~StreamedMediaChannel()
        { delete d; }

bool StreamedMediaChannel::isValid() const
{ return d->m_isValid; }

// This functions adds the local handle from the group of local pending members into the group of members.
bool StreamedMediaChannel::acceptIncomingStream()
{
    QList<uint> accept_handles;
    accept_handles << d->localHandle();

    return addMembers( accept_handles );
}

bool StreamedMediaChannel::rejectIncomingStream()
{
    QList<uint> reject_handles;
    reject_handles << d->localHandle();

    return removeMembers( reject_handles );
}

bool StreamedMediaChannel::requestChannel( QList<Telepathy::MediaStreamType> types )
{
    Q_ASSERT( d->m_pConnectionInterface );
    Q_ASSERT( d->m_pContact );
    if ( !d->m_pConnectionInterface
          || !d->m_pContact )
    {
        return false;
    }

    QDBusPendingReply<QDBusObjectPath> request_channel_reply = d->m_pConnectionInterface->RequestChannel( Telepathy::Client::ChannelTypeStreamedMediaInterface::staticInterfaceName(),
                                          Telepathy::HandleTypeContact,
                                          d->m_pContact->telepathyHandle(),
                                          true );
    request_channel_reply.waitForFinished();

    if ( !request_channel_reply.isValid() )
    {
        QDBusError error = request_channel_reply.error();

        qWarning() << "RequestChannel: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();

        return false;
    }


    if ( d->m_pStreamedMediaInterface )
    {
        delete d->m_pStreamedMediaInterface;
        d->m_pStreamedMediaInterface = NULL;
    }

    QDBusObjectPath channel_path=request_channel_reply.value();
    d->m_pStreamedMediaInterface = new Telepathy::Client::ChannelTypeStreamedMediaInterface( d->m_pConnectionInterface->service(), channel_path.path(), this );
    connectSignals();
    
    if ( !d->m_pStreamedMediaInterface )
    {
        return false;
    }

    
    QList<uint> stream_types;
    foreach( uint type, types )
    {
        stream_types << type;
    }

    
    QDBusPendingReply<Telepathy::MediaStreamInfoList> request_streams_reply = d->m_pStreamedMediaInterface->RequestStreams( d->m_pContact->telepathyHandle(),
            stream_types );
    request_streams_reply.waitForFinished();

    if ( !request_streams_reply.isValid() )
    {
        QDBusError error = request_streams_reply.error();

        qWarning() << "RequestStreams: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();

        return false;
    }
    
    // Fall through..
    return true;
}

bool StreamedMediaChannel::addContactsToGroup( QList<QPointer<TpPrototype::Contact> > contacts )
{
    QList<uint> handle_list = d->handleListForContacts( contacts );

    return addMembers( handle_list );
}

bool StreamedMediaChannel::removeContactsFromGroup( QList<QPointer<TpPrototype::Contact> > contacts )
{
    QList<uint> handle_list = d->handleListForContacts( contacts );
    
    return removeMembers( handle_list );
}

QList<QPointer<TpPrototype::Contact> > StreamedMediaChannel::localPendingContacts()
{
    QList<QPointer<TpPrototype::Contact> > local_pending_members;
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Local Pending members : ";
#endif
    foreach( Telepathy::LocalPendingInfo local_pending_info, d->m_pGroupInterface->LocalPendingMembers() )
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "To be added: " << local_pending_info.toBeAdded;
        qDebug() << "Actor      : " << local_pending_info.actor;
        qDebug() << "Reason     : " << local_pending_info.reason;
        qDebug() << "Message    : " << local_pending_info.message;
#endif
        local_pending_members.append( d->m_pContact->contactManager()->contactForHandle( local_pending_info.toBeAdded ) );
    }
    return local_pending_members;  
}

QList<QPointer<TpPrototype::Contact> > StreamedMediaChannel::members()
{
    Telepathy::UIntList member_handles = d->m_pGroupInterface->Members();

    QList<QPointer<TpPrototype::Contact> > member_list;

    foreach( uint member_handle, member_handles )
    {
        member_list.append( d->m_pContact->contactManager()->contactForHandle( member_handle ) );
    }
    
    return member_list;
}

bool StreamedMediaChannel::removeStreams( const QList<uint>& streamIds )
{
    QDBusPendingReply<> remove_streams_reply = d->m_pStreamedMediaInterface->RemoveStreams( streamIds );
    remove_streams_reply.waitForFinished();
    if ( !remove_streams_reply.isValid() )
    {
        QDBusError error = remove_streams_reply.error();
        qWarning() << "RemoveStreams: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();
        
        return false;
    }

    return true;
}

Telepathy::MediaStreamInfoList StreamedMediaChannel::requestStreams( QList<Telepathy::MediaStreamType> _types )
{
    Telepathy::UIntList types;

    foreach( uint type, _types )
    {
        types.append( type );
    }
    
    QDBusPendingReply<Telepathy::MediaStreamInfoList> request_streams_reply = d->m_pStreamedMediaInterface->RequestStreams( d->m_pContact->telepathyHandle(), types);
    
    request_streams_reply.waitForFinished();
    if ( !request_streams_reply.isValid() )
    {
        QDBusError error = request_streams_reply.error();
        qWarning() << "RequestStreams: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();
        
        return QDBusPendingReply<Telepathy::MediaStreamInfoList>();
    }

    return request_streams_reply.value();
}

Telepathy::MediaStreamInfoList StreamedMediaChannel::listStreams()
{
    if ( !d->m_pStreamedMediaInterface )
    { return Telepathy::MediaStreamInfoList();}

    QDBusPendingReply<Telepathy::MediaStreamInfoList> reply = d->m_pStreamedMediaInterface->ListStreams();
    reply.waitForFinished();
    if ( !reply.isValid() )
    {
        QDBusError error = reply.error();
        qWarning() << "ListStreams: error type:" << error.type()
                << "error name:" << error.name()
                << "error message:" << error.message();
        
        return Telepathy::MediaStreamInfoList();
    }
    
    return reply.value();
}



// public Slots
void StreamedMediaChannel::slotSetOutputVolume( uint streamId, uint volume )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->SetOutputVolume( QDBusObjectPath( d->m_pStreamedMediaInterface->path() ), streamId, volume );

    d->checkDBusCall( reply, "SetOutputVolume" );
}

void StreamedMediaChannel::slotMuteInput( uint streamId, bool muteState )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->MuteInput( QDBusObjectPath( d->m_pStreamedMediaInterface->path() ), streamId, muteState );

    d->checkDBusCall( reply, "MuteInput" );
}

void StreamedMediaChannel::slotMuteOutput( uint streamId, bool muteState )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->MuteOutput( QDBusObjectPath( d->m_pStreamedMediaInterface->path() ), streamId, muteState );

    d->checkDBusCall( reply, "MuteOutput" );
}

void StreamedMediaChannel::slotSetOutputWindow( uint streamId, uint windowId )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->SetOutputWindow( QDBusObjectPath( d->m_pStreamedMediaInterface->path() ), streamId, windowId );

    d->checkDBusCall( reply, "SetOutputWindow" );
}

void StreamedMediaChannel::slotAddPreviewWindow( uint windowId )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->AddPreviewWindow( windowId );

    d->checkDBusCall( reply, "AddPreviewWindow" );
}

void StreamedMediaChannel::slotRemovePreviewWindow( uint windowId )
{
    if ( !d->isStreamEngineAvailable() )
    { return;}

    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->RemovePreviewWindow( windowId );

    d->checkDBusCall( reply, "RemovePreviewWindow" );
}


void StreamedMediaChannel::slotShutDown()
{
    if ( !d->isStreamEngineAvailable() )
    { return;}
    
    QDBusPendingReply<> reply = d->m_pStreamEngineInterface->Shutdown();

    d->checkDBusCall( reply, "Shutdown" );

    // Cleanup locally
    d->cleanup();
}

// Protected functions

// Called if a new media channel shall be established.
void StreamedMediaChannel::requestStreamedMediaChannel( uint handle )
{
    // Ignore this call if the media channel is already available
    if ( d->m_pStreamedMediaInterface )
    {
        delete d->m_pStreamedMediaInterface;
        d->m_pStreamedMediaInterface = NULL;
    }
    
    QDBusPendingReply<QDBusObjectPath> reply0 = d->m_pConnectionInterface->RequestChannel( "org.freedesktop.Telepathy.Channel.Type.StreamedMedia",
                                                                                           Telepathy::HandleTypeContact,
                                                                                           handle,
                                                                                           true );
    reply0.waitForFinished();
    if (!reply0.isValid())
    {
        QDBusError error = reply0.error();
        qWarning() << "RequestChannel (Type: StreamedMedia): error type:" << error.type()
                                                                          << "error name:" << error.name()
                                                                          << "error message:" << error.message();
        d->m_isValid = false;
        return;
    }
    QDBusObjectPath channel_path=reply0.value();

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "StreamedMediaChannel new ChannelTypeStreamedMediaInterface(): Path:" << channel_path.path();
#endif
    d->m_pStreamedMediaInterface = new Telepathy::Client::ChannelTypeStreamedMediaInterface( d->m_pConnectionInterface->service(), channel_path.path(), this );
    connectSignals();
}


// Called if a new streamed media channel was notified by the connection
void StreamedMediaChannel::openStreamedMediaChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelType )
{
    Q_ASSERT( d->m_pConnectionInterface );
    
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "StreamedMediaChannel::openStreamedMediaChannel(): handle:" << handle
             << "handleType:"    << handleType
             << "channel path: " << channelPath
             << "Channel Type:"  << channelType;
#endif

    QString channel_service_name = d->m_pConnectionInterface->service();

    if ( !d->m_pStreamedMediaInterface )
    {
        delete d->m_pStreamedMediaInterface;
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Create new ChannelTypeStreamedMediaInterface";
#endif
        d->m_pStreamedMediaInterface = new Telepathy::Client::ChannelTypeStreamedMediaInterface( channel_service_name, channelPath, this );
        connectSignals();
    }
    if (!d->m_pStreamedMediaInterface->isValid())
    {
        qWarning() << "Failed to connect streamed media interface class to D-Bus object.";
        delete d->m_pStreamedMediaInterface;
        d->m_pStreamedMediaInterface = NULL;
        d->m_isValid = false;
        return;
    }
    else
    {
        // Obtain the group and callstate interfaces.
        QString streamed_media_service_name = d->m_pStreamedMediaInterface->service();

        // I don't know why, but I have to reinitialize the group channel every time..
        if ( d->m_pGroupInterface )
        {
            delete d->m_pGroupInterface;
            d->m_pGroupInterface = NULL;
        }
    
        if ( !d->m_pGroupInterface )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Initialize ChannelInterfaceGroupInterface..";
#endif
            d->m_pGroupInterface = new Telepathy::Client::ChannelInterfaceGroupInterface( streamed_media_service_name,
                                                                                          channelPath );

            connect( d->m_pGroupInterface, SIGNAL( MembersChanged(const QString& ,
                                                                  const Telepathy::UIntList& ,
                                                                  const Telepathy::UIntList& ,
                                                                  const Telepathy::UIntList& ,
                                                                  const Telepathy::UIntList& ,
                                                                  uint ,
                                                                  uint )
                                                 ),
                     this, SLOT( slotMembersChanged(const QString& ,
                                 const Telepathy::UIntList& ,
                                 const Telepathy::UIntList& ,
                                 const Telepathy::UIntList& ,
                                 const Telepathy::UIntList& ,
                                 uint ,
                                 uint )
                               )
                   );                       
                                                                          

        }
        
        if ( !d->m_pCallStateInterface )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Initialize ChannelInterfaceCallStateInterface..";
#endif
            d->m_pCallStateInterface = new Telepathy::Client::ChannelInterfaceCallStateInterface( streamed_media_service_name,
                                                                                                  channelPath );
        }

        if ( !d->m_pMediaSignallingInterface )
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Initialize ChannelInterfaceMediaSignallingInterface..";
#endif
            d->m_pMediaSignallingInterface = new Telepathy::Client::ChannelInterfaceMediaSignallingInterface( streamed_media_service_name, channelPath );
        }
        
        if ( !d->m_pStreamEngineInterface )
        {

            // FIXME: Think whether we really need this Maemo variant of StreamEngineInterface. The default telepathy version should
            //        be enough and avoids platform dependency.
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Initialize StreamEngineInterface..";
#endif
            d->m_pStreamEngineInterface = new TpPrototype::Client::StreamEngineInterface( "org.freedesktop.Telepathy.StreamEngine",
                                                                                    "/org/freedesktop/Telepathy/StreamEngine",
                                                                                    this );
            
        }

        if ( !d->m_pStreamEngineHandlerInterface )
        {
            // The following interface is exported by the stream engine as well.
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Initialize ChannelHandlerInterface..";
#endif
            d->m_pStreamEngineHandlerInterface = new TpPrototype::Client::ChannelHandlerInterface( "org.freedesktop.Telepathy.StreamEngine",
                                                                                                 "/org/freedesktop/Telepathy/StreamEngine",
                                                                                                 this );
        }

        Q_ASSERT( d->m_pCallStateInterface->isValid() );
        Q_ASSERT( d->m_pMediaSignallingInterface->isValid() );

        // Cleanup if we were unable to establish interfaces..
        if ( d->m_pGroupInterface && !d->m_pGroupInterface->isValid() )
        {
            qWarning() << "Could not establish interface:" << Telepathy::Client::ChannelInterfaceGroupInterface::staticInterfaceName();
            delete d->m_pGroupInterface;
            d->m_pGroupInterface = NULL;
        }

        if ( d->m_pCallStateInterface && !d->m_pCallStateInterface->isValid() )
        {
            qWarning() << "Could not establish interface:" << Telepathy::Client::ChannelInterfaceCallStateInterface::staticInterfaceName();
            delete d->m_pCallStateInterface;
            d->m_pCallStateInterface = NULL;
        }

        if ( d->m_pMediaSignallingInterface && !d->m_pMediaSignallingInterface->isValid() )
        {
            qWarning() << "Could not establish interface:" << Telepathy::Client::ChannelInterfaceMediaSignallingInterface::staticInterfaceName();
            delete d->m_pMediaSignallingInterface;
            d->m_pMediaSignallingInterface = NULL;
        }

        // Now use the streaming engine to handle this media channel
        if ( !d->m_pStreamEngineHandlerInterface || !d->m_pStreamEngineInterface )
        {
            qWarning() << "Could not establish interface:" << TpPrototype::Client::ChannelHandlerInterface::staticInterfaceName();
            qWarning() << "or interface:" << TpPrototype::Client::StreamEngineInterface::staticInterfaceName();
            
            delete d->m_pStreamEngineHandlerInterface;
            d->m_pStreamEngineHandlerInterface = NULL;
            
            delete d->m_pStreamEngineInterface;
            d->m_pStreamEngineInterface = NULL;

            // This is fatal, cause we need this interface
            d->m_isValid = false;
            qWarning() << "The interface:" << TpPrototype::Client::StreamEngineInterface::staticInterfaceName() << "is required! We will be unable to handle this call!";
        }
        else
        {
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Now delegate channel to stream-engine by calling HandleChannel()";
#endif
            QDBusPendingReply<> handle_channel_reply = d->m_pStreamEngineHandlerInterface->HandleChannel( d->m_pConnectionInterface->service(),
                                                                                                          QDBusObjectPath( d->m_pConnectionInterface->path() ),
                                                                                                          channelType,
                                                                                                          QDBusObjectPath( channelPath ),
                                                                                                          handleType,
                                                                                                          handle );

            handle_channel_reply.waitForFinished();

            if ( !handle_channel_reply.isValid() )
            {
                QDBusError error = handle_channel_reply.error();

                qWarning() << "HandleChannel: error type:" << error.type()
                           << "error name:" << error.name()
                           << "error message:" << error.message();

                d->m_isValid = false;
                return ;
            }
        }
        
    }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Telling the world about new channel (signalIncomingChannel())";
#endif
    emit signalIncomingChannel( d->m_pContact );

}

void StreamedMediaChannel::connectSignals()
{
    connect( d->m_pStreamedMediaInterface, SIGNAL( StreamAdded(uint , uint , uint ) ),
             this, SLOT( slotStreamAdded(uint , uint , uint ) ) ); 
    connect( d->m_pStreamedMediaInterface, SIGNAL( StreamDirectionChanged(uint , uint , uint ) ),
             this, SLOT( slotStreamDirectionChanged(uint , uint , uint ) ) );
    connect( d->m_pStreamedMediaInterface, SIGNAL( StreamError(uint , uint , const QString& ) ),
             this, SLOT( slotStreamError(uint , uint , const QString& ) ) );
    connect( d->m_pStreamedMediaInterface, SIGNAL( StreamRemoved(uint) ),
             this, SLOT( slotStreamRemoved(uint) ) );
    connect( d->m_pStreamedMediaInterface, SIGNAL( StreamStateChanged(uint , uint ) ),
             this, SLOT( slotStreamStateChanged(uint , uint ) ) );
}

bool StreamedMediaChannel::addMembers( QList<uint> handles )
{
    if ( !d->m_pGroupInterface )
    {
        qWarning() << "StreamedMediaChannel::addMembers: No group channel found. Ignore call ..";
        return false;
    }

    QDBusPendingReply<> add_members_reply = d->m_pGroupInterface->AddMembers( handles, "Welcome!" );
    add_members_reply.waitForFinished();

    if ( !add_members_reply.isValid() )
    {
        QDBusError error = add_members_reply.error();

        qWarning() << "AddMembers: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();

        return false;
    }
    return true;
}

bool StreamedMediaChannel::removeMembers( QList<uint> handles )
{
    if ( !d->m_pGroupInterface )
    {
        qWarning() << "StreamedMediaChannel::removeMembers: No group channel found. Ignore call ..";
        return false;
    }

    QDBusPendingReply<> remove_members_reply = d->m_pGroupInterface->RemoveMembers( handles, "Bye-bye!!" );
    remove_members_reply.waitForFinished();

    if ( !remove_members_reply.isValid() )
    {
        QDBusError error = remove_members_reply.error();

        qWarning() << "RemoveMembers: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();

        return false;
    }
    return true;
}


void StreamedMediaChannel::slotStreamAdded(uint streamID, uint contactHandle, uint streamType)
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__ << "streamID:" << streamID << "contactHandle: " << contactHandle << "streamType:" << streamType;
#endif
    emit signalStreamAdded( this, streamID, static_cast<Telepathy::MediaStreamType>(streamType) );
}

void StreamedMediaChannel::slotStreamDirectionChanged(uint streamID, uint streamDirection, uint pendingFlags)
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__ << "streamID:" << streamID << "streamDirection: " << streamDirection << "pendingFlags:" << pendingFlags;
#endif
    emit signalStreamDirectionChanged( this, streamID, streamDirection, pendingFlags );
}

void StreamedMediaChannel::slotStreamError(uint streamID, uint errorCode, const QString& message)
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__ << "streamID:" << streamID << "errorCode: " << errorCode << "message:" << message;
#endif
    qWarning() << "Stream error: streamId: " << streamID << "ErrorCode: " << errorCode << "Message:" << message;
    emit signalStreamError( this, streamID, errorCode, message );
}

void StreamedMediaChannel::slotStreamRemoved(uint streamID)
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__ << "streamID:" << streamID;
#endif
    emit signalStreamRemoved( this, streamID );
}

void StreamedMediaChannel::slotStreamStateChanged(uint streamID, uint streamState)
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__ << "streamID:" << streamID << "streamState: " << streamState;
#endif
    emit signalStreamStateChanged( this, streamID, static_cast<Telepathy::MediaStreamState>(streamState) );
}

void StreamedMediaChannel::slotMembersChanged( const QString& message,
                                               const Telepathy::UIntList& added,
                                               const Telepathy::UIntList& removed,
                                               const Telepathy::UIntList& localPending,
                                               const Telepathy::UIntList& remotePending,
                                               uint actor,
                                               uint reason )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << __PRETTY_FUNCTION__
             << "message:" << message
             << "added:" << added
             << "removed:" << removed
             << "localPending:" << localPending
             << "remotePending:" << remotePending
             << "actor:" << actor
             << "reason:" << reason;
#endif
    uint local_handle = d->localHandle();
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "local handle: " << local_handle;
#endif    
    if ( !added.isEmpty() )
    {
        foreach ( uint handle, added )
        {
            if ( handle == local_handle )
            {
                // I was added to the group. That means that an invitation was accepted.
                emit signalLocalInvitationAccepted( this );
                continue;
            }
            emit signalContactAdded( this, d->m_pContact->contactManager()->contactForHandle( handle ) );
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "signalContactAdded: " << handle;
#endif
        }
    }

    if ( !removed.isEmpty() )
    {
        foreach ( uint handle, removed )
        {
            if ( handle == local_handle )
            { continue; }
            emit signalContactRemoved( this, d->m_pContact->contactManager()->contactForHandle( handle ) );
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "signalContactRemoved: " << handle;
#endif
        }
    }
}

#include "TelepathyQt4/Prototype/_gen/StreamedMediaChannel.h.moc"
