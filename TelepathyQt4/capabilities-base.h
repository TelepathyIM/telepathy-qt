/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_capabilities_base_h_HEADER_GUARD_
#define _TelepathyQt4_capabilities_base_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT CapabilitiesBase
{
public:
    virtual ~CapabilitiesBase();

    // FIXME: (API/ABI break) Use high-level class for requestableChannelClass
    RequestableChannelClassList requestableChannelClasses() const;

    bool isSpecificToContact() const;

    // FIXME: (API/ABI break) Rename all supportsXXX methods to just XXX
    bool supportsTextChats() const;

    bool supportsMediaCalls() const;
    bool supportsAudioCalls() const;
    bool supportsVideoCalls(bool withAudio = true) const;
    bool supportsUpgradingCalls() const;

    // later: FIXME TODO why not now‽
    // bool supportsFileTransfers() const;
    // QList<FileHashType> fileTransfersRequireHash() const;
    //
    // bool supportsStreamTubes() const;
    // bool supportsDBusTubes() const;

protected:
    CapabilitiesBase(bool specificToContact);
    CapabilitiesBase(const RequestableChannelClassList &classes,
            bool specificToContact);

    virtual void updateRequestableChannelClasses(
            const RequestableChannelClassList &classes);

private:
    friend class Connection;
    friend class Contact;

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
