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

#include "TelepathyQt4/Prototype/ContactManager.h"

#include <time.h>

#include <QCoreApplication>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDebug>
#include <QHash>
#include <QPointer>
#include <QString>

#include <TelepathyQt4/Client/Channel>

#include <TelepathyQt4/Prototype/ChatChannel.h>
#include <TelepathyQt4/Prototype/ConnectionFacade.h>
#include <TelepathyQt4/Prototype/Contact.h>
#include <TelepathyQt4/Prototype/StreamedMediaChannel.h>

// #define ENABLE_DEBUG_OUTPUT_

using namespace TpPrototype;

class TpPrototype::ContactManagerPrivate
{
public:
    ContactManagerPrivate()
    { init(); }

    ~ContactManagerPrivate()
    { }

    Telepathy::Client::ConnectionInterface* m_pInterface;
    Telepathy::Client::ChannelInterfaceGroupInterface*  m_groupSubscribedChannel;
    Telepathy::Client::ChannelInterfaceGroupInterface*  m_groupKnownChannel;
    Telepathy::Client::ChannelInterfaceGroupInterface*  m_groupPublishedChannel;
    Telepathy::Client::ChannelInterfaceGroupInterface*  m_groupDeniedChannel;
    Telepathy::Client::ChannelTypeTextInterface*  m_groupTextChannel;

    QHash<uint, QPointer<Contact> > m_members;
    QHash<uint, QPointer<Contact> > m_subscribed;
    QHash<uint, QPointer<Contact> > m_localPending;
    QHash<uint, QPointer<Contact> > m_remotePending;
    QHash<uint, QPointer<Contact> > m_known;
    QHash<uint, QPointer<Contact> > m_denied;

    bool m_isValid;

    void init()
    {
        m_pInterface = NULL;
        m_groupSubscribedChannel = NULL;
        m_groupKnownChannel = NULL;
        m_groupPublishedChannel = NULL;
        m_groupTextChannel = NULL; //leave this temporaryly here push it later to contact
        m_isValid    = false;
    }

    // Returns a list of all <i>handles</i> already <i>contained</i> in <i>list</i>
    QList<uint> handlesToLookUp( const QHash<uint, QPointer<Contact> >& list, const Telepathy::UIntList& handles, bool contained )
    {
        QList<uint> to_look_up;
        foreach(uint handle, handles)
        {
            if ( contained == list.contains(handle) )
            { to_look_up.append(handle); }
        }
        return to_look_up;
    }

    QList<uint> subscribedHandlesToLookUp(const Telepathy::UIntList& handles)
    {
        return handlesToLookUp( m_subscribed, handles, false );
    }

    QList<uint> newKnownHandlesToLookUp(const Telepathy::UIntList& handles)
    {
        return handlesToLookUp( m_members, handles, false );
    }

    QList<uint> removedHandlesToLookUp(const Telepathy::UIntList& handles)
    {
        return handlesToLookUp( m_members, handles, true );
    }

    QList<uint> localPendingHandlesToLookUp(const Telepathy::UIntList& handles)
    {
        return handlesToLookUp( m_localPending, handles, false );
    }

    QList<uint> remotePendingHandlesToLookUp(const Telepathy::UIntList& handles)
    {
        return handlesToLookUp( m_remotePending, handles, false );
    }

    QList<uint> newDeniedHandles( const Telepathy::UIntList& handles )
    {
        return handlesToLookUp( m_denied, handles, false );
    }

    QList<uint> removedDeniedHandles( const Telepathy::UIntList& handles )
    {
        return handlesToLookUp( m_denied, handles, true );
    }

    QList<QPointer<TpPrototype::Contact> > mapHashToList( QHash<uint, QPointer<Contact> > hash )
    {
        QList<QPointer<TpPrototype::Contact> > ret_list;

        foreach( const QPointer<TpPrototype::Contact>& contact, hash )
        {
            // Filter possilble null pointers
            if ( NULL == contact )
            { continue; }

            ret_list.append( contact );
        }

        return ret_list;
    }

};

ContactManager::ContactManager( Telepathy::Client::ConnectionInterface* connection,
                                QObject* parent ):
    QObject( parent ),
    d( new ContactManagerPrivate )
{
    init( connection );
}

ContactManager::~ContactManager()
{
    // Delete this here as we are a friend of contact.
    foreach (QPointer<Contact> current_contact, d->m_members)
    { delete current_contact; }    
    foreach (QPointer<Contact> current_contact, d->m_subscribed)
    { delete current_contact; }
    foreach (QPointer<Contact> current_contact, d->m_localPending)
    { delete current_contact; }
    foreach (QPointer<Contact> current_contact, d->m_remotePending)
    { delete current_contact; }

    delete d;
}

int ContactManager::count()
{ return d->m_members.size(); }

bool ContactManager::isValid()
{ return d->m_isValid; }

QList<QPointer<Contact> > ContactManager::contactList()
{ return d->m_members.values(); }

QList<QPointer<Contact> > ContactManager::toAuthorizeList()
{ return d->m_localPending.values();}

QList<QPointer<Contact> > ContactManager::remoteAuthorizationPendingList()
{ return d->m_remotePending.values();}

QList<QPointer<Contact> > ContactManager::blockedContacts()
{ return d->m_denied.values(); }

void ContactManager::init( Telepathy::Client::ConnectionInterface* connection )
{
    Q_ASSERT(0 != connection);
    Telepathy::registerTypes();
    d->m_pInterface = connection;
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager up and running... waiting for signals.";
#endif//
    d->m_isValid = true;
}

bool ContactManager::requestContact( const QString& id )
{
    QStringList contact_ids;
    contact_ids.append(id);
    
    QList<uint> contact_handles=d->m_pInterface->RequestHandles( Telepathy::HandleTypeContact,contact_ids);
    if (!contact_handles.empty())
    {
        if ( d->m_groupSubscribedChannel)
            d->m_groupSubscribedChannel->AddMembers(contact_handles,"Contact Request");
        return true;
    }
    return false;
}

bool ContactManager::authorizeContact( const Contact* contact )
{
    if ( !contact )
    { return false; }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager Try to authorize a contact";
#endif
    QList<uint> toauthorizelist;
    toauthorizelist.append(contact->telepathyHandle());
    if ( d->m_groupPublishedChannel)
        d->m_groupPublishedChannel->AddMembers(toauthorizelist,"Add");
    return true;
}


bool ContactManager::removeContact( const Contact* contact_toremove )
{
    if ( !contact_toremove )
    { return false; }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager Try to remove a contact ..";
#endif
    QList<uint> toremovelist;
    toremovelist.append(contact_toremove->telepathyHandle());
    
    if ( d->m_groupSubscribedChannel)
        d->m_groupSubscribedChannel->RemoveMembers(toremovelist,"Remove");
    if ( d->m_groupPublishedChannel)
        d->m_groupPublishedChannel->RemoveMembers(toremovelist,"Remove");
    if ( d->m_groupKnownChannel)
        d->m_groupKnownChannel->RemoveMembers(toremovelist,"Remove");

    return true;
}

bool ContactManager::blockContact( const Contact* contactToBlock )
{
    if ( !contactToBlock || !d->m_groupDeniedChannel )
    { return false; }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager: Try to block a contact";
#endif
    QList<uint> handle_list;
    handle_list.append( contactToBlock->telepathyHandle() );

    d->m_groupDeniedChannel->AddMembers( handle_list, "Block" );

    return true;
    // The list of blocked users (d->m_denied()) will be updated by a signal after this point
}

bool ContactManager::unblockContact( const Contact* contactToUnblock )
{
    if ( !contactToUnblock || !d->m_groupDeniedChannel )
    { return false; }

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager: Try to unblock a contact";
#endif
    QList<uint> handle_list;
    handle_list.append( contactToUnblock->telepathyHandle() );

    d->m_groupDeniedChannel->RemoveMembers( handle_list, "Unblock" );

    return true;
    // The list of blocked users (d->m_denied()) will be updated by a signal after this point
}

QPointer<TpPrototype::Contact> ContactManager::contactForHandle( uint handle )
{
    return d->m_members.value( handle );
}

uint ContactManager::localHandle()
{
    return ConnectionFacade::instance()->selfHandleForConnectionInterface( d->m_pInterface );
}


/*
    inline QDBusPendingReply<> AcknowledgePendingMessages(const Telepathy::UIntList& IDs)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(IDs);
    return asyncCallWithArgumentList(QLatin1String("AcknowledgePendingMessages"), argumentList);
}

    inline QDBusPendingReply<Telepathy::UIntList> GetMessageTypes()
{
    return asyncCall(QLatin1String("GetMessageTypes"));
}

*/


void ContactManager::openSubscribedContactsChannel(uint handle, const QDBusObjectPath& channelPath, const QString& channelType)
{
    QString channel_service_name(d->m_pInterface->service());
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager Channel Services Name" << channel_service_name;
    qDebug() << "ContactManager Channel Path" << channelPath.path();
#endif
    // This channel may never be closed!
    d->m_groupSubscribedChannel = new Telepathy::Client::ChannelInterfaceGroupInterface(channel_service_name,channelPath.path(),
            this);
    if (!d->m_groupSubscribedChannel->isValid())
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Failed to connect Group channel interface class to D-Bus object.";
#endif
        delete d->m_groupSubscribedChannel;
        d->m_groupSubscribedChannel = 0;
        return;
    }
    QDBusPendingReply<Telepathy::UIntList, Telepathy::UIntList, Telepathy::UIntList> reply2=d->m_groupSubscribedChannel->GetAllMembers();
    reply2.waitForFinished();

    const Telepathy::UIntList mlist1= QList< quint32 > (reply2.argumentAt<0>());
    const Telepathy::UIntList mlist2;
    const Telepathy::UIntList mlist3= QList< quint32 > (reply2.argumentAt<1>());
    const Telepathy::UIntList mlist4= QList< quint32 > (reply2.argumentAt<2>());
#ifdef ENABLE_DEBUG_OUTPUT_    
    qDebug() << "Number of current members" << mlist1.size();
    qDebug() << "Number of local pending members" << mlist2.size();
    qDebug() << "Number of remote pending members" << mlist3.size();
#endif      
    if (( mlist1.size()>0) || ( mlist3.size()>0) || ( mlist4.size()>0))   
        slotSubscribedMembersChanged("",
                        mlist1,
                        mlist2,
                        mlist3,
                        mlist4,
                        0, 0);

    // All lists are empty when the channel is created, so it suffices to connect
    // the MembersChanged signal.
    connect(d->m_groupSubscribedChannel, SIGNAL(MembersChanged(const QString&,
            const Telepathy::UIntList&, // added to members list
            const Telepathy::UIntList&, // removed from members list
            const Telepathy::UIntList&, // local pending list
            const Telepathy::UIntList&, // remote pending list
            uint, uint)),
            this, SLOT(slotSubscribedMembersChanged(const QString&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       uint, uint)));
   
}

void ContactManager::openKnownContactsChannel(uint handle, const QDBusObjectPath& channelPath, const QString& channelType)
{
    QString channel_service_name(d->m_pInterface->service());
#ifdef ENABLE_DEBUG_OUTPUT_   
    qDebug() << "ContactManager Channel Services Name" << channel_service_name;
    qDebug() << "ContactManager Channel Path" << channelPath.path();
#endif         
    // This channel may never be closed!
    d->m_groupKnownChannel = new Telepathy::Client::ChannelInterfaceGroupInterface(channel_service_name,channelPath.path(),
                               this);
    if (!d->m_groupKnownChannel->isValid())
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Failed to connect Group channel interface class to D-Bus object.";
#endif
        delete d->m_groupKnownChannel;
        d->m_groupKnownChannel = 0;
        return;
    }
    QDBusPendingReply<Telepathy::UIntList, Telepathy::UIntList, Telepathy::UIntList> reply2=d->m_groupKnownChannel->GetAllMembers();
    reply2.waitForFinished();

    const Telepathy::UIntList mlist1= QList< quint32 > (reply2.argumentAt<0>());
    const Telepathy::UIntList mlist2;
    const Telepathy::UIntList mlist3= QList< quint32 > (reply2.argumentAt<1>());
    const Telepathy::UIntList mlist4= QList< quint32 > (reply2.argumentAt<2>());
#ifdef ENABLE_DEBUG_OUTPUT_    
    qDebug() << "Number of current members" << mlist1.size();
    qDebug() << "Number of local pending members" << mlist2.size();
    qDebug() << "Number of remote pending members" << mlist3.size();
#endif     
    if (( mlist1.size()>0) || ( mlist3.size()>0) || ( mlist4.size()>0))
        slotKnownMembersChanged("",
                        mlist1,
                        mlist2,
                        mlist3,
                        mlist4,
                        0, 0);

    // All lists are empty when the channel is created, so it suffices to connect
    // the MembersChanged signal.
    connect(d->m_groupKnownChannel, SIGNAL(MembersChanged(const QString&,
            const Telepathy::UIntList&, // added to members list
            const Telepathy::UIntList&, // removed from members list
            const Telepathy::UIntList&, // local pending list
            const Telepathy::UIntList&, // remote pending list
            uint, uint)),
            this, SLOT(slotKnownMembersChanged(const QString&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       uint, uint)));

}

void ContactManager::openDenyContactsChannel(uint handle, const QDBusObjectPath& channelPath, const QString& channelType)
{
    QString channel_service_name(d->m_pInterface->service());
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager Channel Services Name" << channel_service_name;
    qDebug() << "ContactManager Channel Path" << channelPath.path();
#endif
    // This channel may never be closed!
    d->m_groupDeniedChannel = new Telepathy::Client::ChannelInterfaceGroupInterface(channel_service_name,channelPath.path(),
                                                                                    this);
    if (!d->m_groupDeniedChannel->isValid())
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Failed to connect Group channel interface class to D-Bus object.";
#endif
        delete d->m_groupDeniedChannel;
        d->m_groupDeniedChannel = NULL;
        return;
    }

    QDBusPendingReply<Telepathy::UIntList, Telepathy::UIntList, Telepathy::UIntList> reply2=d->m_groupKnownChannel->GetAllMembers();
    reply2.waitForFinished();

    const Telepathy::UIntList current        = QList< quint32 > (reply2.argumentAt<0>());
    const Telepathy::UIntList local_pending  = QList< quint32 > (reply2.argumentAt<1>());
    const Telepathy::UIntList remote_pending = QList< quint32 > (reply2.argumentAt<2>());
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Denied-Members: Number of current members" << current.size();
    qDebug() << "Denied-Members: Number of local pending members" << local_pending.size();
    qDebug() << "Denied-Members: Number of remote pending members" << remote_pending.size();
#endif
    if (( current.size()>0) || ( local_pending.size()>0) || ( remote_pending.size()>0))
        slotDeniedMembersChanged("",
                                current,
                                Telepathy::UIntList(),
                                local_pending,
                                remote_pending,
                                0, 0);

    // All lists are empty when the channel is created, so it suffices to connect
    // the MembersChanged signal.
    connect(d->m_groupDeniedChannel, SIGNAL(MembersChanged(const QString&,
                                                            const Telepathy::UIntList&, // added to members list
                                                            const Telepathy::UIntList&, // removed from members list
                                                            const Telepathy::UIntList&, // local pending list
                                                            const Telepathy::UIntList&, // remote pending list
                                                            uint,                       // actor
                                                            uint)),                     // reason
            this, SLOT(slotDeniedMembersChanged(const QString&,
                                                const Telepathy::UIntList&,
                                                const Telepathy::UIntList&,
                                                const Telepathy::UIntList&,
                                                const Telepathy::UIntList&,
                                                uint, uint)));

}

void ContactManager::openTextChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelTyp )
{
    Contact* contact = d->m_members.value( handle );

    if ( !contact )
    {
        qWarning() << "ContactManager::openTextChannel: Tried to open a text channel but there was no receiving contact found!";
        return;
    }
    if ( !contact->chatChannel() )
    {
        qWarning() << "ContactManager::openTextChannel: Requesting a valid text channel object failed!";
        return;
    }
    
    if ( contact && contact->chatChannel() )
    {
        contact->chatChannel()->openTextChannel( handle, handleType, channelPath, channelTyp );
        emit signalTextChannelOpenedForContact( contact );
    }
}

void ContactManager::openStreamedMediaChannel( uint handle, uint handleType, const QString& channelPath, const QString& channelType )
{
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "ContactManager::openStreamedMediaChannel handle: " << handle
             << "handleType:" << handleType
             << "channelPath: " << channelPath
             << "channelType: " << channelType;
#endif

    // Outbound calls have no handle and don't need to be handled here.
    if ( handle == 0 )
    { return; }

    
    Contact* contact = d->m_members.value( handle );

    if ( !contact )
    {
        qWarning() << "ContactManager::openStreamedMediaChannel: Tried to open a media stream channel but there was no receiving contact found!";
        return;
    }
    if ( !contact->streamedMediaChannel() )
    {
        qWarning() << "ContactManager::openStreamedMediaChannel: Requesting a valid streamed media channel object failed!";
        return;
    }

    if ( contact && contact->streamedMediaChannel() )
    {
        disconnect( contact->streamedMediaChannel(), SIGNAL( signalIncomingChannel( TpPrototype::Contact* ) ), this, 0 );
        connect( contact->streamedMediaChannel(), SIGNAL( signalIncomingChannel( TpPrototype::Contact* ) ),
                 this, SIGNAL( signalStreamedMediaChannelOpenedForContact( TpPrototype::Contact* ) ) );
        contact->streamedMediaChannel()->openStreamedMediaChannel( handle, handleType, channelPath, channelType );
    }
}

void ContactManager::openPublishContactsChannel(uint handle, const QDBusObjectPath& channelPath, const QString& channelType)
{
    QString channel_service_name(d->m_pInterface->service());
#ifdef ENABLE_DEBUG_OUTPUT_    
    qDebug() << "ContactManager Channel Services Name" << channel_service_name;
    qDebug() << "ContactManager Channel Path" << channelPath.path();
#endif        
    // This channel may never be closed!
    d->m_groupPublishedChannel = new Telepathy::Client::ChannelInterfaceGroupInterface(channel_service_name,channelPath.path(),
            this);
    if (!d->m_groupPublishedChannel->isValid())
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Failed to connect Group channel interface class to D-Bus object.";
#endif
        delete d->m_groupPublishedChannel;
        d->m_groupPublishedChannel = 0;
        return;
    }
    QDBusPendingReply<Telepathy::UIntList, Telepathy::UIntList, Telepathy::UIntList> reply2=d->m_groupPublishedChannel->GetAllMembers();
    reply2.waitForFinished();

    const Telepathy::UIntList mlist1= QList< quint32 > (reply2.argumentAt<0>());
    const Telepathy::UIntList mlist2;
    const Telepathy::UIntList mlist3= QList< quint32 > (reply2.argumentAt<1>());
    const Telepathy::UIntList mlist4= QList< quint32 > (reply2.argumentAt<2>());
#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Number of current members" << mlist1.size();
    qDebug() << "Number of local pending members" << mlist2.size();
    qDebug() << "Number of remote pending members" << mlist3.size();
#endif    
    if (( mlist1.size()>0) || ( mlist3.size()>0) || ( mlist4.size()>0))
        slotPublishedMembersChanged("",
                        mlist1,
                        mlist2,
                        mlist3,
                        mlist4,
                        0, 0);
   // qDebug() << "Number of current members" << mlist1.size();
   // qDebug() << "Number of local pending members" << mlist2.size();
   // qDebug() << "Number of remote pending members" << mlist3.size();
    
    // All lists are empty when the channel is created, so it suffices to connect
    // the MembersChanged signal.
    connect(d->m_groupPublishedChannel, SIGNAL(MembersChanged(const QString&,
            const Telepathy::UIntList&, // added to members list
            const Telepathy::UIntList&, // removed from members list
            const Telepathy::UIntList&, // local pending list
            const Telepathy::UIntList&, // remote pending list
            uint, uint)),
            this, SLOT(slotPublishedMembersChanged(const QString&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       const Telepathy::UIntList&,
                       uint, uint)));

}
  


void ContactManager::slotKnownMembersChanged(const QString& message,
                                        const Telepathy::UIntList& members_added,
                                        const Telepathy::UIntList& members_removed,
                                        const Telepathy::UIntList& local_pending,
                                        const Telepathy::UIntList& remote_pending,
                                        uint actor, uint reason)
{
    slotMembersChanged("Known",
                       members_added,
                       members_removed,
                       local_pending,
                       remote_pending,
                       actor, reason);
}
void ContactManager::slotPublishedMembersChanged(const QString& message,
                                             const Telepathy::UIntList& members_added,
                                             const Telepathy::UIntList& members_removed,
                                             const Telepathy::UIntList& local_pending,
                                             const Telepathy::UIntList& remote_pending,
                                             uint actor, uint reason)
{
    slotMembersChanged("Published",
                       members_added,
                       members_removed,
                       local_pending,
                       remote_pending,
                       actor, reason);
}

void ContactManager::slotSubscribedMembersChanged(const QString& message,
                                             const Telepathy::UIntList& members_added,
                                             const Telepathy::UIntList& members_removed,
                                             const Telepathy::UIntList& local_pending,
                                             const Telepathy::UIntList& remote_pending,
                                             uint actor, uint reason)
{
    slotMembersChanged("Subscribed",
                       members_added,
                       members_removed,
                       local_pending,
                       remote_pending,
                       actor, reason);
}

void ContactManager::slotDeniedMembersChanged(const QString& message,
                                                const Telepathy::UIntList& members_added,
                                                const Telepathy::UIntList& members_removed,
                                                const Telepathy::UIntList& local_pending,
                                                const Telepathy::UIntList& remote_pending,
                                                uint actor,
                                                uint reason)
{
    QList<uint> added_members   = d->newDeniedHandles( members_added );        // members added to the list of denied.
    QList<uint> removed_members = d->removedDeniedHandles( members_removed );  // members removed from the list of denied.
    QList<uint> new_members     = d->newKnownHandlesToLookUp( members_added ); // new members that are not yet in the list of members

    // Remove all removed members from the denied list
    foreach( uint member_handle, removed_members )
    {
        Contact* contact = d->m_denied.value( member_handle );
        if ( !contact )
        { continue; }
        contact->setType( Contact::CT_Known );
        d->m_denied.remove( member_handle );
    }

    // Members that are not already known (in list of members): create new contact.
    QDBusPendingReply<QStringList> handle_names_reply = d->m_pInterface->InspectHandles( Telepathy::HandleTypeContact, new_members );
    handle_names_reply.waitForFinished();
    if ( !handle_names_reply.isValid() )
    {
        QDBusError error = handle_names_reply.error();

        qWarning() << "InspectHandles (new denied handles=: error type:" << error.type()
                   << "error name:" << error.name()
                   << "error message:" << error.message();
        return;
    }
    Q_ASSERT( handle_names_reply.value().size() == new_members.size() );

    // Create Contacts for the looked up handles:
    for (int i = 0; i < new_members.size(); ++i)
    {
        QPointer<Contact> contact = new Contact( new_members.at(i), handle_names_reply.value().at(i), Contact::CT_Blocked, d->m_pInterface, this );
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Create contact for known Handle" << handle_names_reply.value().at(i);
#endif
        Q_ASSERT(contact->isValid());
        d->m_members.insert( new_members.at( i ), contact );
    }
    // end: create new contact

    // Now update list of blocked members and emit signals.
    foreach( uint member_handle, added_members )
    {
        Contact* contact = d->m_members.value( member_handle );
        if ( !contact )
        { continue; }
        contact->setType( Contact::CT_Blocked );
        d->m_denied.insert( member_handle, contact );

        emit signalContactBlocked( this, contact );
    }
    // Emit signals for the removed members
    foreach( uint member_handle, removed_members )
    {

        emit signalContactUnblocked( this, d->m_members.value( member_handle ) );
    }
}


// TODO: This function is a real beast. It should be split into separate functions. Rethink whether it is a good idea
//       to map all slots to this functions!?
void ContactManager::slotMembersChanged(const QString& message,
                                        const Telepathy::UIntList& members_added,
                                        const Telepathy::UIntList& members_removed,
                                        const Telepathy::UIntList& local_pending,
                                        const Telepathy::UIntList& remote_pending,
                                        uint actor, uint reason)
{
    Q_UNUSED(actor);
    Q_UNUSED(reason);

#ifdef ENABLE_DEBUG_OUTPUT_
    qDebug() << "Members Changed" << message << "Actor" << actor << "Reason" <<reason;
    qDebug() << "Members Added" << members_added.size();
    qDebug() << "Members Removed" << members_removed.size();
    qDebug() << "Members Local Pending" << local_pending.size();
    qDebug() << "Members Remote Pending" << remote_pending.size();
#endif
    // Figure out handles that we need to look up.
    // Batch them up to minimize roundtrips.
    


    
    if (members_added.size()!=0)
    {
        if (message==QString("Known"))
        {        
            QList<uint> known_to_look_up(d->newKnownHandlesToLookUp(members_added));
        // look up 'missing' handles:
            QDBusPendingReply<QStringList> handle_names =
                    d->m_pInterface->InspectHandles(Telepathy::HandleTypeContact, known_to_look_up);
            handle_names.waitForFinished();
            if (!handle_names.isValid())
            {
                QDBusError error = handle_names.error();

                qWarning() << "InspectHandles: error type:" << error.type()
                           << "error name:" << error.name();
                return;
            }
            Q_ASSERT(handle_names.value().size() == known_to_look_up.size());
#ifdef ENABLE_DEBUG_OUTPUT_            
            qDebug() << "Known Handle" << handle_names.value().size() <<"Look Up Size" << known_to_look_up.size();
#endif            
        // Create Contacts for the looked up handles:
            QHash<uint, QPointer<Contact> > tmp_list;
            for (int i = 0; i < known_to_look_up.size(); ++i)
            {
                

                QPointer<Contact> contact = new Contact(known_to_look_up.at(i), handle_names.value().at(i), Contact::CT_Known, d->m_pInterface, this );
#ifdef ENABLE_DEBUG_OUTPUT_                
                qDebug() << "Known Handle" << handle_names.value().at(i);
#endif                
                Q_ASSERT(contact->isValid());
                tmp_list.insert(known_to_look_up.at(i), contact);

            }
            known_to_look_up.clear();
            if (tmp_list.size()>0)
            {
                foreach (uint handle, members_added)
                {
                //Q_ASSERT(!d->m_subscribed.contains(handle)); // We do not yet know that handle.
                // FIXME: Check where the handle comes from to figure out whether it was
                //        Getting authorized, etc.
                    QPointer<Contact> current_contact = tmp_list.value(handle);
                // Detect mixups when writing into the hash.
                    if (current_contact)
                    {
                        Q_ASSERT(current_contact->telepathyHandle() == handle);
                        tmp_list.remove(handle);
                        if (!d->m_subscribed.contains(handle))
                        {
#ifdef ENABLE_DEBUG_OUTPUT_                            
                            qDebug() << "Added Handle to Known List";
#endif                            
                        }
                        else
                        {
#ifdef ENABLE_DEBUG_OUTPUT_
                            qDebug() << "Known Contact already in contactlist";
#endif                            
                            d->m_members.remove(handle);
                            d->m_subscribed.remove(handle);
                        }
                        d->m_members.insert(handle, current_contact);
                        d->m_known.insert(handle, current_contact);
                        emit signalContactKnown( this, current_contact );
                    }
                }
            }
        }
        else //message!="Known"
        {
           
            QList<uint> subscribed_to_look_up(d->subscribedHandlesToLookUp(members_added));
    
        // look up 'missing' handles:
            QDBusPendingReply<QStringList> handle_names =
                    d->m_pInterface->InspectHandles(Telepathy::HandleTypeContact, subscribed_to_look_up);
            handle_names.waitForFinished();
            if (!handle_names.isValid())
            {
                QDBusError error = handle_names.error();

                qWarning() << "InspectHandles: error type:" << error.type()
                           << "error name:" << error.name();
                return;
            }
            Q_ASSERT(handle_names.value().size() == subscribed_to_look_up.size());
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Subscribed  Handle" << handle_names.value().size() <<"Look Up Size" << subscribed_to_look_up.size()<< "Mmebers Added Size "<< members_added.size();
#endif            
        // Create Contacts for the looked up handles:
            QHash<uint, QPointer<Contact> > tmp_list;
            for (int i = 0; i < subscribed_to_look_up.size(); ++i)
            {
                
                QPointer<Contact> contact = new Contact(subscribed_to_look_up.at(i), handle_names.value().at(i), Contact::CT_Subscribed, d->m_pInterface, this );
#ifdef ENABLE_DEBUG_OUTPUT_
                qDebug() << "Subscribed Handle" << handle_names.value().at(i);
#endif                
                Q_ASSERT(contact->isValid());
                tmp_list.insert(subscribed_to_look_up.at(i), contact);
            }
            if (tmp_list.size()>0)
            {
                QPointer<Contact> current_contact;
                foreach (uint handle, members_added)
                {
                //Q_ASSERT(!d->m_subscribed.contains(handle)); // We do not yet know that handle.
                // FIXME: Check where the handle comes from to figure out whether it was
                //        Getting authorized, etc.
                    current_contact = tmp_list.value(handle);

                    if (current_contact)
                    {
                    // Detect mixups when writing into the hash.
                        Q_ASSERT(current_contact->telepathyHandle() == handle);
                    
                        tmp_list.remove(handle);
                        if (!d->m_members.contains(handle))
                        {
#ifdef ENABLE_DEBUG_OUTPUT_                      
                            qDebug() << "Added Handle to Subscribed List"<< handle;
#endif
                            Q_ASSERT( current_contact->type() == Contact::CT_Subscribed );
                            d->m_members.insert(handle, current_contact);
                            //d->m_subscribed.insert(handle, current_contact);
                        }
                        else
                        {
#ifdef ENABLE_DEBUG_OUTPUT_                            
                            qDebug() << "Subscribed Contact already in contactlist"<< handle;
#endif
#if 1 // Don't understand this code.
                            
                            if (d->m_members[handle]->type() == Contact::CT_LocalPending)
                            {
#ifdef ENABLE_DEBUG_OUTPUT_
                                qDebug() << "Changed Subscribed Contact to local pending:"<< d->m_members[handle]->name();
                                qDebug() << "Current type: " << d->m_members[handle]->type();
#endif
                                d->m_members[handle]->setType(Contact::CT_LocalPending);

                                // TODO: Signal?
                            }
                            else
                            {
#endif
                                d->m_members[handle]->setType(Contact::CT_Subscribed);
                                if (d->m_remotePending.contains(handle))
                                    d->m_remotePending.remove(handle);
                                if (d->m_localPending.contains(handle))
                                    d->m_localPending.remove(handle);
#ifdef ENABLE_DEBUG_OUTPUT_
                                qDebug() << "Signal for Subscribed Contact:"<< d->m_members[handle]->name();
#endif
                                emit signalContactSubscribed( this,  d->m_members[handle] );
#if 1
                            }
#endif

#ifdef ENABLE_DEBUG_OUTPUT_
                            qDebug() << "delete Contact object later:" << current_contact->name();;
#endif
                            current_contact->deleteLater();
                        }

                    }
                }
            }
        }
    }
    if (local_pending.size()!=0)
    {
        QList<uint> localPending_to_look_up(d->localPendingHandlesToLookUp(local_pending));
    // look up 'missing' handles:
        QDBusPendingReply<QStringList> handle_names =
                d->m_pInterface->InspectHandles(Telepathy::HandleTypeContact, localPending_to_look_up);
        handle_names.waitForFinished();
        if (!handle_names.isValid())
        {
            QDBusError error = handle_names.error();
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "InspectHandles: error type:" << error.type()
                    << "error name:" << error.name();
#endif
            return;
        }
        Q_ASSERT(handle_names.value().size() == localPending_to_look_up.size());
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Local Pending Size" << handle_names.value().size() <<"Look Up Size" << localPending_to_look_up.size();
#endif
        
    // Create Contacts for the looked up handles:
        QHash<uint, QPointer<Contact> > tmp_list;
        for (int i = 0; i < localPending_to_look_up.size(); ++i)
        {
            QPointer<Contact> contact = new Contact(localPending_to_look_up.at(i), handle_names.value().at(i), Contact::CT_LocalPending, d->m_pInterface, this );
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Local Pending Handle" << handle_names.value().at(i);
#endif
            Q_ASSERT(contact->isValid());
            tmp_list.insert(localPending_to_look_up.at(i), contact);
        }
        if (tmp_list.size()>0)
        {
            foreach (uint handle, local_pending)
            {
                QPointer<Contact> current_contact = tmp_list.value(handle);
                if (current_contact)
                {
                    Q_ASSERT(current_contact->telepathyHandle() == handle);
                
                    tmp_list.remove(handle);
                    if (!d->m_members.contains(handle))
                    {
#ifdef ENABLE_DEBUG_OUTPUT_                        
                        qDebug() << "Added Handle to Local Pending List";
#endif
                        d->m_members.insert(handle, current_contact);
                        d->m_localPending.insert(handle, current_contact);
                    }
                    else
                    {
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Local Pending Contact already in contactlist";
#endif
                        d->m_members[handle]->setType(Contact::CT_LocalPending);
                        if (d->m_remotePending.contains(handle))
                            d->m_remotePending.remove(handle);
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "delete current contact"  << current_contact->name();
#endif
                        delete current_contact;
                    }

                    emit signalContactLocalPending( this,  d->m_members[handle] );
                }            
                               
            }
        }
    }
        
    if (remote_pending.size()!=0)
    {
        QList<uint> remotePending_to_look_up(d->remotePendingHandlesToLookUp(remote_pending));

    // look up 'missing' handles:
        QDBusPendingReply<QStringList> handle_names =
                d->m_pInterface->InspectHandles(Telepathy::HandleTypeContact, remotePending_to_look_up);
        handle_names.waitForFinished();

        if (!handle_names.isValid())
        {
            QDBusError error = handle_names.error();
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "InspectHandles: error type:" << error.type()
                    << "error name:" << error.name();
#endif
            return;
        }
        Q_ASSERT(handle_names.value().size() == remotePending_to_look_up.size());
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "Remote Pending Handle Size" << handle_names.value().size() <<"Look Up Size" << remotePending_to_look_up.size();
#endif
   // Create Contacts for the looked up handles:
        QHash<uint, QPointer<Contact> > tmp_list;
        for (int i = 0; i < remotePending_to_look_up.size(); ++i)
        {
            QPointer<Contact> contact = new Contact(remotePending_to_look_up.at(i), handle_names.value().at(i), Contact::CT_RemotePending, d->m_pInterface, this );
#ifdef ENABLE_DEBUG_OUTPUT_
            qDebug() << "Remote Pending Handle " << handle_names.value().at(i);
#endif
            Q_ASSERT(contact->isValid());
            tmp_list.insert(remotePending_to_look_up.at(i), contact);
        }
        if (tmp_list.size()>0)
        {
            foreach (uint handle, remote_pending)
            {

                QPointer<Contact> current_contact = tmp_list.value(handle);
                if (current_contact)
                {
                // Detect mixups when writing into the hash.
                    Q_ASSERT(current_contact->telepathyHandle() == handle);
                    tmp_list.remove(handle);
                    if (!d->m_members.contains(handle))
                    {
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Added Handle to Remote Pending List";
#endif
                        d->m_remotePending.insert(handle, current_contact);
                        d->m_members.insert(handle, current_contact);
                    }
                    else
                    {
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Remote Pending Contact already in contactlist";
#endif                        
                        d->m_members[handle]->setType(Contact::CT_RemotePending);

                        if (d->m_localPending.contains(handle))
                            d->m_localPending.remove(handle);
                        
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Delete current contact:" << current_contact->name();
#endif
                        delete current_contact;
                    }

                    emit signalContactRemotePending( this,  d->m_members[handle] );
                }
                                           
            }
        }
    }
    
    if (members_removed.size()!=0)
    {
#ifdef ENABLE_DEBUG_OUTPUT_
        qDebug() << "SlotMembers Changed Removed Called";
#endif
        QList<uint> removed_to_look_up(d->removedHandlesToLookUp(members_removed));
    // look up 'missing' handles:
        QDBusPendingReply<QStringList> handle_names =
                d->m_pInterface->InspectHandles(Telepathy::HandleTypeContact, removed_to_look_up);
        handle_names.waitForFinished();
        if (!handle_names.isValid())
        {
            QDBusError error = handle_names.error();
            qWarning() << "InspectHandles: error type:" << error.type()
                       << "error name:" << error.name();
            return;
        }
        Q_ASSERT(handle_names.value().size() == removed_to_look_up.size());
    // Create Contacts for the looked up handles:
        QHash<uint, QPointer<Contact> > tmp_list;
        for (int i = 0; i < removed_to_look_up.size(); ++i)
        {
            QPointer<Contact> contact = new Contact(removed_to_look_up.at(i), handle_names.value().at(i), Contact::CT_Known, d->m_pInterface, this );
            Q_ASSERT(contact->isValid());
            tmp_list.insert(removed_to_look_up.at(i), contact);
        }
              
        if (tmp_list.size()>0)
        {
            foreach (uint handle, members_removed)
            {

                QPointer<Contact> current_contact = tmp_list.value(handle);
                if (current_contact)
                {
                // Detect mixups when writing into the hash.
                    Q_ASSERT(current_contact->telepathyHandle() == handle);
                
                    tmp_list.remove(handle);
                    if (d->m_members.contains(handle))
                    {
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Remove Contact";
#endif                        
                        emit signalContactRemoved( this, current_contact );
                        
                        d->m_members.remove(handle);
                        d->m_subscribed.remove(handle);
                        d->m_remotePending.remove(handle);
                        d->m_localPending.remove(handle);
                        d->m_known.remove(handle);

#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "delete Contact object: " << current_contact->name();
#endif
                        delete current_contact;
                    }
                    else
                    {                 
#ifdef ENABLE_DEBUG_OUTPUT_
                        qDebug() << "Removed Contact not in contactlist: " << current_contact->name();
#endif
                        delete current_contact;
                    }
                }
            }
        }
    }
    emit signalMembersChanged( this,
                               message,
                               d->mapHashToList( d->m_members ),
                               d->mapHashToList( d->m_localPending ),
                               d->mapHashToList( d->m_remotePending ),
                               contactForHandle( actor ),
                               static_cast<Telepathy::ChannelGroupChangeReason>( reason ) );
}

#include "_gen/ContactManager.h.moc"
