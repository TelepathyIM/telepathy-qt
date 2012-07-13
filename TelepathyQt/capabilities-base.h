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

#ifndef _TelepathyQt_capabilities_base_h_HEADER_GUARD_
#define _TelepathyQt_capabilities_base_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/Types>

namespace Tp
{

class TP_QT_EXPORT CapabilitiesBase
{
public:
    CapabilitiesBase();
    CapabilitiesBase(const CapabilitiesBase &other);
    virtual ~CapabilitiesBase();

    CapabilitiesBase &operator=(const CapabilitiesBase &other);

    RequestableChannelClassSpecList allClassSpecs() const;

    bool isSpecificToContact() const;

    bool textChats() const;

    bool audioCalls() const;
    bool videoCalls() const;
    bool videoCallsWithAudio() const;
    bool upgradingCalls() const;

    TP_QT_DEPRECATED bool streamedMediaCalls() const;
    TP_QT_DEPRECATED bool streamedMediaAudioCalls() const;
    TP_QT_DEPRECATED bool streamedMediaVideoCalls() const;
    TP_QT_DEPRECATED bool streamedMediaVideoCallsWithAudio() const;
    TP_QT_DEPRECATED bool upgradingStreamedMediaCalls() const;

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
