/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
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

#include <tests/lib/test.h>
#include <tests/lib/test-thread-helper.h>

#define TP_QT_ENABLE_LOWLEVEL_API

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/BaseProtocol>
#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/IODevice>

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionCapabilities>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/ConnectionManager>
#include <TelepathyQt/ConnectionManagerLowlevel>
#include <TelepathyQt/ContactCapabilities>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/DBusError>
#include <TelepathyQt/PendingChannel>
#include <TelepathyQt/PendingConnection>
#include <TelepathyQt/PendingContacts>
#include <TelepathyQt/PendingReady>

#include <TelepathyQt/FileTransferChannelCreationProperties>
#include <TelepathyQt/IncomingFileTransferChannel>
#include <TelepathyQt/OutgoingFileTransferChannel>

static const int c_defaultTimeout = 500;

Tp::RequestableChannelClass createRequestableChannelClassFileTransfer()
{
    Tp::RequestableChannelClass fileTransfer;
    fileTransfer.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER;
    fileTransfer.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".ContentHashType"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI"));
    return fileTransfer;
}

QByteArray generateFileContent(int size)
{
    QByteArray result;
    result.reserve(size);

    int sequenceSize = 64;

    if (size < sequenceSize) {
        sequenceSize = 1;
    }

    char c = 'a';
    while (size > 0) {
        const QString marker = QString(QLatin1String("|0x%1|")).arg(result.size(), 0, 16);

        int bytesToAppend = size < sequenceSize ? size : sequenceSize;
        if (marker.size() > bytesToAppend) {
            result.append(QByteArray(bytesToAppend, c));
        } else {
            result.append(marker.toLatin1());
            result.append(QByteArray(bytesToAppend - marker.size(), c));
        }

        size -= bytesToAppend;
        ++c;
        if (c > 'z') {
            c = 'a';
        }
    }

    return result;
}

enum CancelCondition {
    NoCancel,
    CancelBeforeAccept,
    CancelBeforeProvide,
    CancelBeforeData,
    CancelBeforeComplete,
};

namespace TestFileTransferCM // The namespace is needed to avoid class name collisions with other tests and examples
{

class Connection;
typedef Tp::SharedPtr<Connection> ConnectionPtr;

}

static const Tp::RequestableChannelClass c_requestableChannelClassFileTransfer = createRequestableChannelClassFileTransfer();

static const QString c_fileContentType(QLatin1String("text/plain"));
static const QDateTime c_fileTimestamp = QDateTime::currentDateTimeUtc();

static TestFileTransferCM::ConnectionPtr g_connection;
static Tp::BaseChannelPtr g_channel;

namespace TestFileTransferCM // The namespace is needed to avoid class name collisions with other tests and examples
{

class ClientFileTransferStateSpy : public QObject, public QList<QList<QVariant> >
{
    Q_OBJECT
public:
    explicit ClientFileTransferStateSpy(QObject *parent = 0) : QObject(parent), QList<QList<QVariant> >()  { }

public Q_SLOTS:
    void trigger(Tp::FileTransferState state,Tp::FileTransferStateChangeReason reason) {
        QList<QVariant> list;
        list << state;
        list << reason;
        append(list);
    }
};

class Connection : public Tp::BaseConnection
{
    Q_OBJECT
public:
    Connection(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters) :
        Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters)
    {
        g_connection = ConnectionPtr(this);

        /* Connection.Interface.Contacts */
        m_contactsIface = Tp::BaseConnectionContactsInterface::create();
        m_contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &Connection::getContactAttributes));
        m_contactsIface->setContactAttributeInterfaces(QStringList()
                                                       << TP_QT_IFACE_CONNECTION
                                                       << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES
                                                       << TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS
                                                       );
        plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactsIface));

        /* Connection.Interface.ContactCapabilities */
        m_contactCapabilitiesIface = Tp::BaseConnectionContactCapabilitiesInterface::create();
        m_contactCapabilitiesIface->setGetContactCapabilitiesCallback(Tp::memFun(this, &Connection::getContactCapabilities));
        plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactCapabilitiesIface));

        /* Connection.Interface.Requests */
        m_requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
        m_requestsIface->requestableChannelClasses << c_requestableChannelClassFileTransfer;
        plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_requestsIface));

        setConnectCallback(Tp::memFun(this, &Connection::connectCB));
        setCreateChannelCallback(Tp::memFun(this, &Connection::createChannelCB));
        setInspectHandlesCallback(Tp::memFun(this, &Connection::inspectHandles));
        setRequestHandlesCallback(Tp::memFun(this, &Connection::requestHandles));

        mContactHandles.insert(1, QLatin1String("selfContact"));
        mContactHandles.insert(2, QLatin1String("ftContact"));

        setSelfContact(1, QLatin1String("selfContact"));
    }
    virtual ~Connection() { }

    Tp::BaseChannelPtr receiveFile(const Tp::FileTransferChannelCreationProperties &properties, uint initiatorHandle)
    {
        if (!mContactHandles.contains(initiatorHandle)) {
            return Tp::BaseChannelPtr();
        }

        Tp::DBusError error;

        QVariantMap request = properties.createRequest();
        request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = selfHandle();
        request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = initiatorHandle;

        Tp::BaseChannelPtr channel = createChannel(request, /* suppressHandler */ false, &error);

        if (error.isValid()) {
            qDebug() << error.message();
            return Tp::BaseChannelPtr();
        }

        return channel;
    }

protected:
    void connectCB(Tp::DBusError *error)
    {
        setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);
        Q_UNUSED(error)
    }

    Tp::BaseChannelPtr createChannelCB(const QVariantMap &request, Tp::DBusError *error)
    {
        const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
        uint targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
        uint targetHandle = 0;
        QString targetID;

        if (channelType != TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unexpected channel type"));
            return Tp::BaseChannelPtr();
        }

        switch (targetHandleType) {
        case Tp::HandleTypeContact:
            if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
                targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
                targetID = mContactHandles.value(targetHandle);
            } else if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
                targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
                targetHandle = mContactHandles.key(targetID);
            }
            break;
        default:
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unexpected target handle type"));
            return Tp::BaseChannelPtr();
            break;
        }

        if (targetID.isEmpty()) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Unexpected target (unknown handle/ID)."));
            return Tp::BaseChannelPtr();
        }

        Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);
        Tp::BaseChannelFileTransferTypePtr fileTransferChannel = Tp::BaseChannelFileTransferType::create(request);
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(fileTransferChannel));
        baseChannel->setTargetID(targetID);

        g_channel = baseChannel;

        return baseChannel;
    }

    QStringList inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
    {
        if (status() != Tp::ConnectionStatusConnected) {
            error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
            return QStringList();
        }

        if (handleType != Tp::HandleTypeContact) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unexpected handle type"));
            return QStringList();
        }

        QStringList result;

        Q_FOREACH (uint handle, handles) {
            if (!mContactHandles.contains(handle)) {
                error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Unknown handle"));
                return QStringList();
            }

            result << mContactHandles.value(handle);
        }

        return result;
    }

    Tp::UIntList requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error)
    {
        Tp::UIntList result;

        if (handleType != Tp::HandleTypeContact) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("requestHandles: Invalid handle type."));
            return result;
        }

        Q_FOREACH (const QString &identifier, identifiers) {
            uint handle = mContactHandles.key(identifier, 0);
            if (!handle) {
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, QString(QLatin1String("requestHandles: Unexpected identifier (%1).")).arg(identifier));
                break;
            }
            result << handle;
        }

        return result;
    }

    Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error)
    {
        Tp::ContactAttributesMap contactAttributes;

        Q_FOREACH (uint handle, handles) {
            if (!mContactHandles.contains(handle)) {
                break;
            }

            QVariantMap attributes;
            attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = mContactHandles.value(handle);

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES + QLatin1String("/capabilities")] =
                        QVariant::fromValue(getContactCapabilities(Tp::UIntList() << handle, error).value(handle));
            }

            contactAttributes[handle] = attributes;
        }

        return contactAttributes;
    }

    Tp::ContactCapabilitiesMap getContactCapabilities(const Tp::UIntList &handles, Tp::DBusError *error)
    {
        Tp::ContactCapabilitiesMap capabilities;

        Q_FOREACH (uint handle, handles) {
            if (!mContactHandles.contains(handle)) {
                error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("getContactCapabilities: Unexpected handle."));
                return Tp::ContactCapabilitiesMap();
            }

            capabilities[handle] = Tp::RequestableChannelClassList() << c_requestableChannelClassFileTransfer;
        }

        return capabilities;
    }

protected:
    Tp::BaseConnectionContactsInterfacePtr m_contactsIface;
    Tp::BaseConnectionContactCapabilitiesInterfacePtr m_contactCapabilitiesIface;
    Tp::BaseConnectionRequestsInterfacePtr m_requestsIface;

    QMap<uint,QString> mContactHandles;

};

} // namespace FTTest

using namespace TestFileTransferCM;

class TestBaseFileTranfserChannel : public Test
{
    Q_OBJECT
public:
    TestBaseFileTranfserChannel(QObject *parent = 0)
        : Test(parent)
    { }

protected Q_SLOTS:
    void onSendFileSvcInputBytesWritten(qint64 bytes);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testConnectionCapability();
    void testContactCapability();
    void testSendFile();
    void testSendFile_data();
    void testReceiveFile();
    void testReceiveFile_data();

    void cleanup();
    void cleanupTestCase();

private:
    Tp::BaseConnectionPtr createConnectionCb(const QVariantMap &parameters, Tp::DBusError *error)
    {
        Q_UNUSED(error)
        return Tp::BaseConnection::create<Connection>(mConnectionManager->name(), mProtocol->name(), parameters);
    }

    int requestCloseCliChannel(Tp::ChannelPtr cliChannel)
    {
        Tp::PendingOperation *pendingChannelClose = cliChannel->requestClose();
        connect(pendingChannelClose, SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
        return mLoop->exec();
    }

    Tp::BaseProtocolPtr mProtocol;
    Tp::BaseConnectionManagerPtr mConnectionManager;

    Tp::ConnectionPtr mCliConnection;
    Tp::ContactPtr mCliContact;
};

void TestBaseFileTranfserChannel::onSendFileSvcInputBytesWritten(qint64 bytes)
{
    Tp::BaseChannelFileTransferTypePtr svcTransferChannel = Tp::BaseChannelFileTransferTypePtr::dynamicCast(g_channel->interface(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER));
    if (svcTransferChannel.isNull()) {
        return;
    }

    svcTransferChannel->setTransferredBytes(svcTransferChannel->transferredBytes() + bytes);
}

void TestBaseFileTranfserChannel::initTestCase()
{
    initTestCaseImpl();

    mProtocol = Tp::BaseProtocol::create(QLatin1String("AlphaProtocol"));
    mProtocol->setRequestableChannelClasses(Tp::RequestableChannelClassSpecList() << c_requestableChannelClassFileTransfer);
    mProtocol->setCreateConnectionCallback(Tp::memFun(this, &TestBaseFileTranfserChannel::createConnectionCb));

    mConnectionManager = Tp::BaseConnectionManager::create(QLatin1String("AlphaCM"));
    mConnectionManager->addProtocol(mProtocol);

    Tp::DBusError err;
    QVERIFY(mConnectionManager->registerObject(&err));
    QVERIFY(!err.isValid());
    QVERIFY(mConnectionManager->isRegistered());
}

void TestBaseFileTranfserChannel::init()
{
    initImpl();
}

void TestBaseFileTranfserChannel::testConnectionCapability()
{
    Tp::ConnectionManagerPtr cliCM = Tp::ConnectionManager::create(mConnectionManager->name());
    Tp::PendingReady *pr = cliCM->becomeReady(Tp::ConnectionManager::FeatureCore);
    connect(pr, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    Tp::ProtocolInfo protocolInfo = cliCM->protocol(mProtocol->name());
    QVERIFY(protocolInfo.isValid());
    QCOMPARE(protocolInfo.name(), mProtocol->name());
    QCOMPARE(protocolInfo.capabilities().fileTransfers(), true);

    Tp::PendingConnection *pendingConnection = cliCM->lowlevel()->requestConnection(mProtocol->name(), QVariantMap());

    connect(pendingConnection, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    mCliConnection = pendingConnection->connection();

    Tp::PendingReady *pendingConnectionReady = mCliConnection->lowlevel()->requestConnect();
    connect(pendingConnectionReady, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(mCliConnection->status(), Tp::ConnectionStatusConnected);

    QVERIFY(mCliConnection->capabilities().fileTransfers());
}

void TestBaseFileTranfserChannel::testContactCapability()
{
    QCOMPARE(mCliConnection->status(), Tp::ConnectionStatusConnected);

    Tp::ContactManagerPtr cliContactManager = mCliConnection->contactManager();
    Tp::PendingContacts *pendingContacts = cliContactManager->contactsForIdentifiers(QStringList() << QLatin1String("ftContact"), Tp::Contact::FeatureCapabilities);
    connect(pendingContacts, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    const QList<Tp::ContactPtr> contacts = pendingContacts->contacts();
    QCOMPARE(contacts.count(), 1);
    QVERIFY(contacts.first()->capabilities().fileTransfers());
    mCliContact = contacts.first();
}

void TestBaseFileTranfserChannel::testSendFile()
{
    QFETCH(int, fileSize);
    QFETCH(int, initialOffset);
    QFETCH(int, cancelCondition);
    QFETCH(bool, useSequentialDevice);

    QCOMPARE(mCliConnection->status(), Tp::ConnectionStatusConnected);
    QVERIFY(!mCliContact.isNull());

    const QByteArray fileContent = generateFileContent(fileSize);
    QCOMPARE(fileContent.size(), fileSize);

    QTemporaryFile file;
    file.setFileTemplate(QLatin1String("file-transfer-test-XXXXXX.txt"));
    QVERIFY2(file.open(), "Unable to create a file for the test");

    Tp::FileTransferChannelCreationProperties fileTransferProperties(file.fileName(), c_fileContentType, fileContent.size());
    fileTransferProperties.setUri(QUrl::fromLocalFile(file.fileName()).toString());
    fileTransferProperties.setLastModificationTime(c_fileTimestamp);

    Tp::PendingChannel *pendingChannel = mCliConnection->lowlevel()->createChannel(fileTransferProperties.createRequest(mCliContact->handle().first()));
    connect(pendingChannel, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    Tp::ChannelPtr cliChannel = pendingChannel->channel();
    Tp::OutgoingFileTransferChannelPtr cliTransferChannel = Tp::OutgoingFileTransferChannelPtr::qObjectCast(cliChannel);
    QVERIFY(cliTransferChannel);

    Tp::PendingReady *pendingChannelReady = cliTransferChannel->becomeReady(Tp::OutgoingFileTransferChannel::FeatureCore);
    connect(pendingChannelReady, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliTransferChannel->channelType(), TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    QVERIFY(cliTransferChannel->isRequested());

    QCOMPARE(cliTransferChannel->state(), Tp::FileTransferStatePending);

    QCOMPARE(g_channel->channelType(), TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    QVERIFY(g_channel->requested());

    Tp::BaseChannelFileTransferTypePtr svcTransferChannel = Tp::BaseChannelFileTransferTypePtr::dynamicCast(g_channel->interface(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER));
    QVERIFY(!svcTransferChannel.isNull());

    QCOMPARE(int(svcTransferChannel->state()), int(Tp::FileTransferStatePending));

    ClientFileTransferStateSpy spyCliState;
    connect(cliTransferChannel.data(), SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)), &spyCliState, SLOT(trigger(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));
    QSignalSpy spySvcState(svcTransferChannel.data(), SIGNAL(stateChanged(uint,uint)));

    if (cancelCondition == CancelBeforeAccept) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        if (spySvcState.isEmpty()) {
            spySvcState.wait();
        }

        QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.count(), 1);
        QCOMPARE(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped));
        return;
    }

    Tp::IODevice svcInputDevice;
    svcInputDevice.open(QIODevice::ReadWrite);
    svcTransferChannel->remoteAcceptFile(&svcInputDevice, initialOffset);

    connect(&svcInputDevice, SIGNAL(bytesWritten(qint64)), this, SLOT(onSendFileSvcInputBytesWritten(qint64)));

    if (spySvcState.isEmpty()) {
        spySvcState.wait();
    }

    QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateAccepted));
    QCOMPARE(spySvcState.count(), 1);
    QCOMPARE(spySvcState.first().at(0).toUInt(), uint(Tp::FileTransferStateAccepted));

    QTRY_COMPARE_WITH_TIMEOUT(spyCliState.count(), 1, 200);
    QCOMPARE(spyCliState.first().at(0).toUInt(), uint(Tp::FileTransferStateAccepted));

    spySvcState.clear();
    spyCliState.clear();

    QSignalSpy spyClientTransferredBytes(cliTransferChannel.data(), SIGNAL(transferredBytesChanged(qulonglong)));

    QIODevice *cliOutputDevice;

    Tp::IODevice cliOutputDeviceSequential;
    QBuffer cliOutputDeviceRandomAccess;

    if (useSequentialDevice) {
        cliOutputDeviceSequential.open(QIODevice::ReadWrite);
        cliOutputDevice = &cliOutputDeviceSequential;
    } else {
        cliOutputDeviceRandomAccess.setData(fileContent);
        cliOutputDevice = &cliOutputDeviceRandomAccess;
    }

    if (cancelCondition == CancelBeforeProvide) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        if (spySvcState.isEmpty()) {
            spySvcState.wait();
        }

        QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.count(), 1);
        QCOMPARE(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped));
        return;
    }

    Tp::PendingOperation *provideFileOperation = cliTransferChannel->provideFile(cliOutputDevice);
    connect(provideFileOperation, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    if (spySvcState.isEmpty()) {
        spySvcState.wait();
    }

    QCOMPARE(int(svcTransferChannel->initialOffset()), initialOffset);
    QCOMPARE(int(cliTransferChannel->initialOffset()), initialOffset);

    if (useSequentialDevice) {
        QVERIFY(spySvcState.count() == 1);
        QTRY_VERIFY_WITH_TIMEOUT(spyCliState.count() == 1, c_defaultTimeout);
        QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateOpen));
        QCOMPARE(uint(cliTransferChannel->state()), uint(Tp::FileTransferStateOpen));
    } else {
        QVERIFY(spySvcState.count() >= 1);
        QTRY_VERIFY_WITH_TIMEOUT(spyCliState.count() >= 1, c_defaultTimeout);
    }

    QCOMPARE(spySvcState.first().at(0).toUInt(), uint(Tp::FileTransferStateOpen));
    QCOMPARE(spyCliState.first().at(0).toUInt(), uint(Tp::FileTransferStateOpen));

    if (initialOffset) {
        QTRY_VERIFY_WITH_TIMEOUT(!spyClientTransferredBytes.isEmpty(), c_defaultTimeout);
    }

    if (useSequentialDevice) {
        QVERIFY(int(cliTransferChannel->transferredBytes()) == initialOffset);
        if (initialOffset) {
            QTRY_VERIFY_WITH_TIMEOUT(spyClientTransferredBytes.last().first().toInt() == initialOffset, c_defaultTimeout);
        }
    } else {
        QVERIFY(int(cliTransferChannel->transferredBytes()) >= initialOffset);
        if (initialOffset) {
            QTRY_VERIFY_WITH_TIMEOUT(spyClientTransferredBytes.last().first().toInt() >= initialOffset, c_defaultTimeout);
        }
    }

    if (cancelCondition == CancelBeforeData) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        QTRY_COMPARE_WITH_TIMEOUT(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
        QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
        QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped), c_defaultTimeout);
        return;
    }

    if (useSequentialDevice) {
        int writtenBytes = cliOutputDeviceSequential.write(fileContent.mid(initialOffset, (fileSize - initialOffset) / 2));

        if (writtenBytes > initialOffset) {
            QTRY_VERIFY_WITH_TIMEOUT(!spyClientTransferredBytes.isEmpty(), c_defaultTimeout);
            QTRY_COMPARE_WITH_TIMEOUT(spyClientTransferredBytes.last().at(0).toInt(), writtenBytes, c_defaultTimeout);
            spyClientTransferredBytes.clear();
        }

        if (cancelCondition == CancelBeforeComplete) {
            QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

            if (spySvcState.isEmpty()) {
                spySvcState.wait();
            }

            QTRY_COMPARE_WITH_TIMEOUT(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
            QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
            QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped), c_defaultTimeout);
            return;
        }

        writtenBytes += cliOutputDeviceSequential.write(fileContent.mid(initialOffset + writtenBytes));
    }

    QTRY_VERIFY_WITH_TIMEOUT(!spyClientTransferredBytes.isEmpty(), c_defaultTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(spyClientTransferredBytes.last().at(0).toInt(), fileSize, c_defaultTimeout);
    spyClientTransferredBytes.clear();

    QTRY_COMPARE_WITH_TIMEOUT(spySvcState.count(), 2, c_defaultTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(spyCliState.count(), 2, c_defaultTimeout);
    QCOMPARE(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCompleted));
    QCOMPARE(spyCliState.last().at(0).toUInt(), uint(Tp::FileTransferStateCompleted));

    QTRY_COMPARE(uint(cliTransferChannel->state()), uint(Tp::FileTransferStateCompleted));

    QByteArray svcData = svcInputDevice.readAll();
    QCOMPARE(svcData, fileContent.mid(initialOffset));
}

void TestBaseFileTranfserChannel::testSendFile_data()
{
    QTest::addColumn<int>("fileSize");
    QTest::addColumn<int>("initialOffset");
    QTest::addColumn<int>("cancelCondition");
    QTest::addColumn<bool>("useSequentialDevice");

    QTest::newRow("Complete (sequential)")                   << 2048 << 0    << int(NoCancel) << true;
    QTest::newRow("Complete (random-access)")                << 2048 << 0    << int(NoCancel) << false;
    QTest::newRow("Complete with an offset (sequential)")    << 2048 << 1000 << int(NoCancel) << true;
    QTest::newRow("Complete with an offset (random-access)") << 2048 << 1000 << int(NoCancel) << false;

    // It makes no sense to use random-access device in follow tests, because we either don't use the device
    QTest::newRow("Cancel before accept")             << 2048 << 0 << int(CancelBeforeAccept)  << true;
    QTest::newRow("Cancel before provide")            << 2048 << 0 << int(CancelBeforeProvide) << true;
    // or need sequential device to control data flow
    QTest::newRow("Cancel before the data")           << 2048 << 0 << int(CancelBeforeData)    << true;
    QTest::newRow("Cancel in the middle of the data") << 2048 << 0 << int(CancelBeforeComplete)<< true;
}

void TestBaseFileTranfserChannel::testReceiveFile()
{
    QFETCH(int, fileSize);
    QFETCH(int, initialOffset);
    QFETCH(int, cancelCondition);
    QFETCH(bool, useSequentialDevice);
    QFETCH(bool, useAutoSkip);

    QCOMPARE(mCliConnection->status(), Tp::ConnectionStatusConnected);
    QVERIFY(!mCliContact.isNull());

    const QByteArray fileContent = generateFileContent(fileSize);
    QCOMPARE(fileContent.size(), fileSize);

    Tp::FileTransferChannelCreationProperties fileTransferProperties(QLatin1String("file-transfer-test-incoming.txt"), c_fileContentType, fileContent.size());
    fileTransferProperties.setUri(QLatin1String("file:///tmp/file-transfer-test-incoming.txt"));
    fileTransferProperties.setLastModificationTime(c_fileTimestamp);

    Tp::BaseChannelPtr svcTransferBaseChannel = g_connection->receiveFile(fileTransferProperties, mCliContact->handle().first());
    QVERIFY(!svcTransferBaseChannel.isNull());

    QVERIFY(!svcTransferBaseChannel->requested());
    QCOMPARE(svcTransferBaseChannel->channelType(), TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);

    Tp::IncomingFileTransferChannelPtr cliTransferChannel = Tp::IncomingFileTransferChannel::create(mCliConnection, svcTransferBaseChannel->objectPath(), svcTransferBaseChannel->immutableProperties());

    Tp::PendingReady *pendingChannelReady = cliTransferChannel->becomeReady(Tp::IncomingFileTransferChannel::FeatureCore);
    connect(pendingChannelReady, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    QCOMPARE(cliTransferChannel->channelType(), TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER);
    QVERIFY(!cliTransferChannel->isRequested());

    QCOMPARE(cliTransferChannel->state(), Tp::FileTransferStatePending);
    QCOMPARE(cliTransferChannel->contentType(), c_fileContentType);
    QCOMPARE(int(cliTransferChannel->size()), fileSize);

    Tp::BaseChannelFileTransferTypePtr svcTransferChannel = Tp::BaseChannelFileTransferTypePtr::dynamicCast(svcTransferBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER));

    Tp::IODevice cliInputDevice;
    cliInputDevice.open(QIODevice::ReadWrite);

    ClientFileTransferStateSpy spyClientState;
    connect(cliTransferChannel.data(), SIGNAL(stateChanged(Tp::FileTransferState,Tp::FileTransferStateChangeReason)), &spyClientState, SLOT(trigger(Tp::FileTransferState,Tp::FileTransferStateChangeReason)));
    QSignalSpy spySvcState(svcTransferChannel.data(), SIGNAL(stateChanged(uint,uint)));

    if (cancelCondition == CancelBeforeAccept) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        if (spySvcState.isEmpty()) {
            spySvcState.wait();
        }

        QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.count(), 1);
        QCOMPARE(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped));
        return;
    }

    Tp::PendingOperation *acceptFileOperation = cliTransferChannel->acceptFile(initialOffset, &cliInputDevice);
    connect(acceptFileOperation, SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(expectSuccessfulCall(Tp::PendingOperation*)));
    QCOMPARE(mLoop->exec(), 0);

    if (spySvcState.isEmpty()) {
        spySvcState.wait();
    }

    QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateAccepted));
    QCOMPARE(spySvcState.count(), 1);
    QCOMPARE(spySvcState.first().at(0).toUInt(), uint(Tp::FileTransferStateAccepted));

    QTRY_COMPARE_WITH_TIMEOUT(spyClientState.count(), 1, c_defaultTimeout);
    QCOMPARE(uint(cliTransferChannel->state()), uint(Tp::FileTransferStateAccepted));

    spySvcState.clear();
    spyClientState.clear();

    Tp::IODevice svcOutputDeviceSequential;
    QBuffer svcOutputDeviceRandomAccess;

    QSignalSpy spyClientTransferredBytes(cliTransferChannel.data(), SIGNAL(transferredBytesChanged(qulonglong)));

    if (cancelCondition == CancelBeforeProvide) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        if (spySvcState.isEmpty()) {
            spySvcState.wait();
        }

        QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.count(), 1);
        QCOMPARE(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled));
        QCOMPARE(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped));
        return;
    }

    if (useSequentialDevice) {
        svcOutputDeviceSequential.open(QIODevice::ReadWrite);
        if (useAutoSkip) {
            svcTransferChannel->remoteProvideFile(&svcOutputDeviceSequential);
        } else {
            svcTransferChannel->remoteProvideFile(&svcOutputDeviceSequential, initialOffset);
        }
    } else {
        svcOutputDeviceRandomAccess.setData(fileContent);
        svcTransferChannel->remoteProvideFile(&svcOutputDeviceRandomAccess); // Use auto seek for random-access device
    }

    if (spySvcState.isEmpty()) {
        spySvcState.wait();
    }

    QCOMPARE(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateOpen));
    QCOMPARE(spySvcState.count(), 1);
    QCOMPARE(spySvcState.first().at(0).toUInt(), uint(Tp::FileTransferStateOpen));

    if (useSequentialDevice) {
        QTRY_VERIFY_WITH_TIMEOUT(spyClientState.count() == 1, c_defaultTimeout);
        QCOMPARE(uint(cliTransferChannel->state()), uint(Tp::FileTransferStateOpen));
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(spyClientState.count() >= 1, c_defaultTimeout);
        uint currentState = cliTransferChannel->state();
        QVERIFY((currentState == Tp::FileTransferStateOpen) || (currentState == Tp::FileTransferStateCompleted));
    }

    if (cancelCondition == CancelBeforeData) {
        QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

        QTRY_COMPARE_WITH_TIMEOUT(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
        QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
        QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped), c_defaultTimeout);
        return;
    }

    if (useSequentialDevice) {
        int actualWriteOffset = useAutoSkip ? 0 : initialOffset;
        int writtenBytes = svcOutputDeviceSequential.write(fileContent.mid(actualWriteOffset, (fileSize - actualWriteOffset) / 2));

        QTRY_VERIFY_WITH_TIMEOUT(!spyClientTransferredBytes.isEmpty(), c_defaultTimeout);
        QTRY_COMPARE_WITH_TIMEOUT(spyClientTransferredBytes.last().at(0).toInt(), writtenBytes + actualWriteOffset, c_defaultTimeout);

        if (cancelCondition == CancelBeforeComplete) {
            QCOMPARE(requestCloseCliChannel(cliTransferChannel), 0);

            if (spySvcState.isEmpty()) {
                spySvcState.wait();
            }

            QTRY_COMPARE_WITH_TIMEOUT(uint(svcTransferChannel->state()), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
            QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(0).toUInt(), uint(Tp::FileTransferStateCancelled), c_defaultTimeout);
            QTRY_COMPARE_WITH_TIMEOUT(spySvcState.last().at(1).toUInt(), uint(Tp::FileTransferStateChangeReasonLocalStopped), c_defaultTimeout);
            return;
        }

        writtenBytes += svcOutputDeviceSequential.write(fileContent.mid(actualWriteOffset + writtenBytes));
    }

    QTRY_VERIFY_WITH_TIMEOUT(!spyClientTransferredBytes.isEmpty(), c_defaultTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(spyClientTransferredBytes.last().at(0).toInt(), fileSize, c_defaultTimeout);

    QTRY_COMPARE_WITH_TIMEOUT(spyClientState.count(), 2, c_defaultTimeout);
    QCOMPARE(uint(cliTransferChannel->state()), uint(Tp::FileTransferStateCompleted));

    QVERIFY(cliInputDevice.isOpen());
    QVERIFY(cliInputDevice.isReadable());
    QByteArray cliData = cliInputDevice.readAll();
    QCOMPARE(cliData, fileContent.mid(initialOffset));
}

void TestBaseFileTranfserChannel::testReceiveFile_data()
{
    QTest::addColumn<int>("fileSize");
    QTest::addColumn<int>("initialOffset");
    QTest::addColumn<int>("cancelCondition");
    QTest::addColumn<bool>("useSequentialDevice");
    QTest::addColumn<bool>("useAutoSkip");

    QTest::newRow("Complete (sequential)")                             << 2048 << 0    << int(NoCancel) << true  << false;
    QTest::newRow("Complete (random-access)")                          << 2048 << 0    << int(NoCancel) << false << false;
    QTest::newRow("Complete with an offset (sequential)")              << 2048 << 1000 << int(NoCancel) << true  << false;
    QTest::newRow("Complete with an offset (sequential, autoskip)")    << 2048 << 1000 << int(NoCancel) << true  << true;
    QTest::newRow("Complete with an offset (random-access)")           << 2048 << 1000 << int(NoCancel) << false << false;
    QTest::newRow("Complete with an offset (random-access, autoskip)") << 2048 << 1000 << int(NoCancel) << false << true;

    // It makes no sense to use random-access device in follow tests, because we either don't use the device
    QTest::newRow("Cancel before accept")             << 2048 << 0 << int(CancelBeforeAccept)  << true << false;
    QTest::newRow("Cancel before provide")            << 2048 << 0 << int(CancelBeforeProvide) << true << false;
    // or need sequential device to control data flow
    QTest::newRow("Cancel before the data")           << 2048 << 0 << int(CancelBeforeData)    << true << false;
    QTest::newRow("Cancel in the middle of the data") << 2048 << 0 << int(CancelBeforeComplete)<< true << false;
}

void TestBaseFileTranfserChannel::cleanup()
{
    cleanupImpl();
}

void TestBaseFileTranfserChannel::cleanupTestCase()
{
    cleanupTestCaseImpl();
}

QTEST_MAIN(TestBaseFileTranfserChannel)
#include "_gen/base-filetransfer.cpp.moc.hpp"
