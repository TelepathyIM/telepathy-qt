/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
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

#ifndef _TelepathyQt4_incoming_stream_tube_channel_h_HEADER_GUARD_
#define _TelepathyQt4_incoming_stream_tube_channel_h_HEADER_GUARD_

#ifndef IN_TELEPATHY_QT4_HEADER
#error IN_TELEPATHY_QT4_HEADER
#endif

#include <TelepathyQt4/StreamTubeChannel>
#include <TelepathyQt4/PendingOperation>

class QHostAddress;
class QIODevice;
namespace Tp {

class PendingVariant;

class PendingStreamTubeConnectionPrivate;
class TELEPATHY_QT4_EXPORT PendingStreamTubeConnection : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingStreamTubeConnection)

public:
    virtual ~PendingStreamTubeConnection();

    QIODevice *device();

    SocketAddressType addressType() const;

    QPair< QHostAddress, quint16 > ipAddress() const;
    QByteArray localAddress() const;

private:
    PendingStreamTubeConnection(PendingVariant *variant,
            SocketAddressType type, const SharedPtr<RefCounted> &object);
    PendingStreamTubeConnection(const QString &errorName,
            const QString &errorMessage, const SharedPtr<RefCounted> &object);

    friend class PendingStreamTubeConnectionPrivate;
    PendingStreamTubeConnectionPrivate *mPriv;

    friend class IncomingStreamTubeChannel;

// private slots:
    Q_PRIVATE_SLOT(mPriv, void __k__onAcceptFinished(Tp::PendingOperation*))
    Q_PRIVATE_SLOT(mPriv, void __k__onTubeStateChanged(Tp::TubeChannelState))
    Q_PRIVATE_SLOT(mPriv, void __k__onDeviceConnected())
    Q_PRIVATE_SLOT(mPriv, void __k__onAbstractSocketError(QAbstractSocket::SocketError))
    Q_PRIVATE_SLOT(mPriv, void __k__onLocalSocketError(QLocalSocket::LocalSocketError))
};


class IncomingStreamTubeChannelPrivate;
class TELEPATHY_QT4_EXPORT IncomingStreamTubeChannel : public StreamTubeChannel
{
    Q_OBJECT
    Q_DISABLE_COPY(IncomingStreamTubeChannel)
    Q_DECLARE_PRIVATE(IncomingStreamTubeChannel)

// private slots:
    Q_PRIVATE_SLOT(d_func(), void __k__onAcceptTubeFinished(Tp::PendingOperation*))
    Q_PRIVATE_SLOT(d_func(), void __k__onNewLocalConnection(uint))

    friend class PendingStreamTubeConnectionPrivate;

public:
    static IncomingStreamTubeChannelPtr create(const ConnectionPtr &connection,
            const QString &objectPath, const QVariantMap &immutableProperties);

    virtual ~IncomingStreamTubeChannel();

    QPair< QHostAddress, quint16 > allowedAddress() const;
    void unsetAllowedAddress();
    void setAllowedAddress(const QHostAddress &address, quint16 port);

    PendingStreamTubeConnection *acceptTube(SocketAccessControl accessControl = SocketAccessControlLocalhost);

    QIODevice *device();

protected:
    IncomingStreamTubeChannel(const ConnectionPtr &connection,
            const QString &objectPath,
            const QVariantMap &immutableProperties,
            const Feature &coreFeature = StreamTubeChannel::FeatureStreamTube);

    virtual void connectNotify(const char* signal);

Q_SIGNALS:
    void newLocalConnection(uint connectionId);

};

}

#endif
