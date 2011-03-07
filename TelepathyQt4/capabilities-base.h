/**
 * This file is part of TelepathyQt4
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

#ifndef _TelepathyQt4_capabilities_base_h_HEADER_GUARD_
#define _TelepathyQt4_capabilities_base_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/RequestableChannelClassSpec>
#include <TelepathyQt4/Types>

namespace Tp
{

class TELEPATHY_QT4_EXPORT CapabilitiesBase
{
public:
    CapabilitiesBase();
    CapabilitiesBase(const CapabilitiesBase &other);
    virtual ~CapabilitiesBase();

    CapabilitiesBase &operator=(const CapabilitiesBase &other);

    RequestableChannelClassSpecList allClassSpecs() const;

    bool isSpecificToContact() const;

    bool textChats() const;

    bool streamedMediaCalls() const;
    bool streamedMediaAudioCalls() const;
    bool streamedMediaVideoCalls() const;
    bool streamedMediaVideoCallsWithAudio() const;
    bool upgradingStreamedMediaCalls() const;

    bool fileTransfers() const;

    // later: FIXME TODO why not now?
    // QList<FileHashType> fileTransfersRequireHash() const;

protected:
    CapabilitiesBase(bool specificToContact);
    CapabilitiesBase(const RequestableChannelClassList &rccs,
            bool specificToContact);
    CapabilitiesBase(const RequestableChannelClassSpecList &rccSpecs,
            bool specificToContact);

    virtual void updateRequestableChannelClasses(
            const RequestableChannelClassList &rccs);

private:
    friend class Connection;
    friend class Contact;

    struct Private;
    friend struct Private;
    QSharedDataPointer<Private> mPriv;
};

} // Tp

Q_DECLARE_METATYPE(Tp::CapabilitiesBase);

#endif
