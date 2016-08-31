file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

# Set the required flags found in TelepathyDefaults
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VISIBILITY_HIDDEN_FLAGS} ${COMPILER_COVERAGE_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")
set(LD_FLAGS "${LD_FLAGS} ${VISIBILITY_HIDDEN_FLAGS} ${COMPILER_COVERAGE_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")

# We are building Telepathy-Qt
add_definitions(-DBUILDING_TP_QT)

configure_file(global.h.in ${CMAKE_CURRENT_BINARY_DIR}/global.h)

# Sources for Tp-Qt
set(telepathy_qt_SRCS
    abstract-client.cpp
    abstract-interface.cpp
    account.cpp
    account-factory.cpp
    account-manager.cpp
    account-property-filter.cpp
    account-set.cpp
    account-set-internal.h
    avatar.cpp
    call-channel.cpp
    call-content.cpp
    call-stream.cpp
    capabilities-base.cpp
    call-content.cpp
    call-content-media-description.cpp
    call-stream.cpp
    call-stream-endpoint.cpp
    captcha.cpp
    captcha-authentication.cpp
    channel.cpp
    channel-class-spec.cpp
    channel-dispatcher.cpp
    channel-dispatch-operation.cpp
    channel-factory.cpp
    channel-internal.h
    channel-request.cpp
    client.cpp
    client-registrar.cpp
    client-registrar-internal.h
    connection.cpp
    connection-capabilities.cpp
    connection-factory.cpp
    connection-internal.h
    connection-manager.cpp
    connection-manager-internal.h
    contact.cpp
    contact-capabilities.cpp
    contact-factory.cpp
    contact-manager.cpp
    contact-manager-roster.cpp
    contact-messenger.cpp
    contact-search-channel.cpp
    dbus.cpp
    dbus-proxy.cpp
    dbus-proxy-factory.cpp
    dbus-proxy-factory-internal.h
    dbus-tube-channel.cpp
    debug.cpp
    debug-receiver.cpp
    debug-internal.h
    fake-handler-manager-internal.cpp
    fake-handler-manager-internal.h
    feature.cpp
    file-transfer-channel.cpp
    file-transfer-channel-creation-properties.cpp
    fixed-feature-factory.cpp
    future.cpp
    future-internal.h
    handled-channel-notifier.cpp
    incoming-dbus-tube-channel.cpp
    incoming-file-transfer-channel.cpp
    incoming-stream-tube-channel.cpp
    key-file.cpp
    key-file.h
    location-info.cpp
    manager-file.cpp
    manager-file.h
    media-session-handler.cpp
    media-stream-handler.cpp
    message.cpp
    message-content-part.cpp
    object.cpp
    optional-interface-factory.cpp
    outgoing-dbus-tube-channel.cpp
    outgoing-file-transfer-channel.cpp
    outgoing-stream-tube-channel.cpp
    pending-account.cpp
    pending-captchas.cpp
    pending-channel.cpp
    pending-channel-request.cpp
    pending-channel-request-internal.h
    pending-connection.cpp
    pending-contact-attributes.cpp
    pending-contact-info.cpp
    pending-contacts.cpp
    pending-dbus-tube-connection.cpp
    pending-debug-message-list.cpp
    pending-handles.cpp
    pending-operation.cpp
    pending-ready.cpp
    pending-send-message.cpp
    pending-string.cpp
    pending-string-list.cpp
    pending-stream-tube-connection.cpp
    pending-variant.cpp
    presence.cpp
    pending-variant-map.cpp
    profile.cpp
    profile-manager.cpp
    properties.cpp
    protocol-info.cpp
    protocol-parameter.cpp
    readiness-helper.cpp
    requestable-channel-class-spec.cpp
    ready-object.cpp
    referenced-handles.cpp
    request-temporary-handler-internal.cpp
    request-temporary-handler-internal.h
    room-list-channel.cpp
    server-authentication-channel.cpp
    simple-call-observer.cpp
    simple-observer.cpp
    simple-observer-internal.h
    simple-stream-tube-handler.cpp
    simple-text-observer.cpp
    simple-text-observer-internal.h
    stream-tube-channel.cpp
    stream-tube-client.cpp
    stream-tube-client-internal.h
    stream-tube-server.cpp
    stream-tube-server-internal.h
    streamed-media-channel.cpp
    text-channel.cpp
    tls-certificate.cpp
    tube-channel.cpp
    types.cpp
    types-internal.h
    utils.cpp)

# Exported headers for Tp-Qt
set(telepathy_qt_HEADERS
    AbstractClient
    AbstractClientApprover
    abstract-client.h
    AbstractClientHandler
    AbstractClientObserver
    AbstractInterface
    abstract-interface.h
    Account
    account.h
    AccountCapabilityFilter
    account-capability-filter.h
    AccountFactory
    account-factory.h
    AccountFilter
    account-filter.h
    AccountInterface
    AccountInterfaceAddressingInterface
    AccountInterfaceAvatarInterface
    AccountInterfaceStorageInterface
    AccountManager
    account-manager.h
    AccountManagerInterface
    account-property-filter.h
    AccountPropertyFilter
    AccountSet
    account-set.h
    AndFilter
    and-filter.h
    AuthenticationTLSCertificateInterface
    AvatarData
    AvatarSpec
    avatar.h
    CallChannel
    call-channel.h
    Callbacks
    callbacks.h
    CallContent
    CallContentInterface
    CallContentInterfaceAudioControlInterface
    CallContentInterfaceDTMFInterface
    CallContentInterfaceMediaInterface
    CallContentInterfaceVideoControlInterface
    call-content.h
    CallContentMediaDescription
    CallContentMediaDescriptionInterface
    CallContentMediaDescriptionInterfaceRTCPExtendedReportsInterface
    CallContentMediaDescriptionInterfaceRTCPFeedbackInterface
    CallContentMediaDescriptionInterfaceRTPHeaderExtensionsInterface
    call-content-media-description.h
    CallStream
    CallStreamInterface
    CallStreamInterfaceMediaInterface
    call-stream.h
    CallStreamEndpoint
    CallStreamEndpoint
    CallStreamEndpointInterface
    call-stream-endpoint.h
    CapabilitiesBase
    capabilities-base.h
    Captcha
    captcha.h
    CaptchaAuthentication
    captcha-authentication.h
    Channel
    channel.h
    ChannelClassFeatures
    channel-class-features.h
    ChannelClassSpec
    ChannelClassSpecList
    channel-class-spec.h
    ChannelDispatcher
    ChannelDispatcherInterface
    channel-dispatcher.h
    ChannelDispatchOperation
    channel-dispatch-operation.h
    ChannelDispatchOperationInterface
    ChannelFactory
    channel-factory.h
    ChannelInterface
    ChannelInterfaceAnonymityInterface
    ChannelInterfaceCaptchaAuthenticationInterface
    ChannelInterfaceCallStateInterface
    ChannelInterfaceChatStateInterface
    ChannelInterfaceConferenceInterface
    ChannelInterfaceDestroyableInterface
    ChannelInterfaceDTMFInterface
    ChannelInterfaceFileTransferMetadataInterface
    ChannelInterfaceGroupInterface
    ChannelInterfaceHoldInterface
    ChannelInterfaceMediaSignallingInterface
    ChannelInterfaceMessagesInterface
    ChannelInterfacePasswordInterface
    ChannelInterfaceRoomInterface
    ChannelInterfaceRoomConfigInterface
    ChannelInterfaceSASLAuthenticationInterface
    ChannelInterfaceSecurableInterface
    ChannelInterfaceServicePointInterface
    ChannelInterfaceSMSInterface
    ChannelInterfaceSubjectInterface
    ChannelInterfaceTubeInterface
    ChannelRequest
    ChannelRequestHints
    channel-request.h
    ChannelRequestInterface
    ChannelTypeCallInterface
    ChannelTypeContactListInterface
    ChannelTypeContactSearchInterface
    ChannelTypeDBusTubeInterface
    ChannelTypeFileTransferInterface
    ChannelTypeRoomListInterface
    ChannelTypeServerAuthenticationInterface
    ChannelTypeServerTLSConnectionInterface
    ChannelTypeStreamedMediaInterface
    ChannelTypeStreamTubeInterface
    ChannelTypeTextInterface
    ChannelTypeTubeInterface
    ChannelTypeTubesInterface
    Client
    ClientApproverInterface
    client.h
    ClientHandlerInterface
    ClientInterface
    ClientInterfaceRequestsInterface
    ClientObserverInterface
    ClientRegistrar
    client-registrar.h
    Connection
    ConnectionCapabilities
    connection-capabilities.h
    connection.h
    ConnectionFactory
    connection-factory.h
    connection-lowlevel.h
    ConnectionInterface
    ConnectionInterfaceAddressingInterface
    ConnectionInterfaceAliasingInterface
    ConnectionInterfaceAnonymityInterface
    ConnectionInterfaceAvatarsInterface
    ConnectionInterfaceBalanceInterface
    ConnectionInterfaceCapabilitiesInterface
    ConnectionInterfaceCellularInterface
    ConnectionInterfaceContactBlockingInterface
    ConnectionInterfaceClientTypesInterface
    ConnectionInterfaceContactCapabilitiesInterface
    ConnectionInterfaceContactGroupsInterface
    ConnectionInterfaceContactInfoInterface
    ConnectionInterfaceContactListInterface
    ConnectionInterfaceContactsInterface
    ConnectionInterfaceLocationInterface
    ConnectionInterfaceMailNotificationInterface
    ConnectionInterfacePowerSavingInterface
    ConnectionInterfacePresenceInterface
    ConnectionInterfaceRequestsInterface
    ConnectionInterfaceServicePointInterface
    ConnectionInterfaceSimplePresenceInterface
    ConnectionLowlevel
    ConnectionManager
    connection-manager.h
    connection-manager-lowlevel.h
    ConnectionManagerInterface
    ConnectionManagerLowlevel
    Constants
    constants.h
    Contact
    contact.h
    ContactCapabilities
    contact-capabilities.h
    ContactFactory
    contact-factory.h
    ContactManager
    contact-manager.h
    ContactMessenger
    contact-messenger.h
    ContactSearchChannel
    contact-search-channel.h
    DBus
    DBusDaemonInterface
    dbus.h
    DBusProxy
    dbus-proxy.h
    DBusProxyFactory
    dbus-proxy-factory.h
    DBusTubeChannel
    dbus-tube-channel.h
    Debug
    debug.h
    DebugReceiver
    debug-receiver.h
    Feature
    Features
    feature.h
    FileTransferChannel
    FileTransferChannelCreationProperties
    file-transfer-channel-creation-properties.h
    file-transfer-channel.h
    Filter
    filter.h
    FixedFeatureFactory
    fixed-feature-factory.h
    Functors
    functors.h
    GenericCapabilityFilter
    generic-capability-filter.h
    GenericPropertyFilter
    generic-property-filter.h
    Global
    ${CMAKE_CURRENT_BINARY_DIR}/global.h
    HandledChannelNotifier
    handled-channel-notifier.h
    IncomingDBusTubeChannel
    incoming-dbus-tube-channel.h
    IncomingFileTransferChannel
    incoming-file-transfer-channel.h
    IncomingStreamTubeChannel
    incoming-stream-tube-channel.h
    IntrospectableInterface
    LocationInfo
    location-info.h
    MediaSessionHandler
    media-session-handler.h
    MediaSessionHandlerInterface
    MediaStreamHandler
    media-stream-handler.h
    MediaStreamHandlerInterface
    Message
    message.h
    MessageContentPart
    MessageContentPartList
    message-content-part.h
    MethodInvocationContext
    method-invocation-context.h
    NotFilter
    not-filter.h
    Object
    object.h
    OptionalInterfaceFactory
    optional-interface-factory.h
    OrFilter
    or-filter.h
    OutgoingDBusTubeChannel
    outgoing-dbus-tube-channel.h
    OutgoingFileTransferChannel
    outgoing-file-transfer-channel.h
    OutgoingStreamTubeChannel
    outgoing-stream-tube-channel.h
    PeerInterface
    PendingAccount
    pending-account.h
    PendingCallContent
    PendingCaptchas
    pending-captchas.h
    PendingChannel
    pending-channel.h
    PendingChannelRequest
    pending-channel-request.h
    PendingComposite
    PendingConnection
    pending-connection.h
    PendingContactAttributes
    pending-contact-attributes.h
    PendingContactInfo
    pending-contact-info.h
    PendingContacts
    pending-contacts.h
    PendingDBusTubeConnection
    pending-dbus-tube-connection.h
    PendingDebugMessageList
    pending-debug-message-list.h
    PendingFailure
    PendingHandles
    pending-handles.h
    PendingOperation
    pending-operation.h
    PendingReady
    pending-ready.h
    PendingSendMessage
    pending-send-message.h
    PendingStreamedMediaStreams
    PendingStreamTubeConnection
    pending-stream-tube-connection.h
    PendingString
    pending-string.h
    PendingStringList
    pending-string-list.h
    PendingSuccess
    PendingVariant
    pending-variant.h
    PendingVariantMap
    pending-variant-map.h
    PendingVoid
    Presence
    presence.h
    PresenceSpec
    PresenceSpecList
    Profile
    profile.h
    ProfileManager
    profile-manager.h
    Properties
    properties.h
    PropertiesInterface
    PropertiesInterfaceInterface
    ProtocolInfo
    ProtocolInfoList
    protocol-info.h
    ProtocolInterface
    ProtocolInterfaceAddressingInterface
    ProtocolInterfaceAvatarsInterface
    ProtocolInterfacePresenceInterface
    ProtocolParameter
    ProtocolParameterList
    protocol-parameter.h
    ReadinessHelper
    readiness-helper.h
    ReadyObject
    ready-object.h
    ReceivedMessage
    RefCounted
    ReferencedHandles
    referenced-handles.h
    ReferencedHandlesIterator
    requestable-channel-class-spec.h
    RequestableChannelClassSpec
    RequestableChannelClassSpecList
    RoomListChannel
    room-list-channel.h
    ServerAuthenticationChannel
    server-authentication-channel.h
    SharedPtr
    shared-ptr.h
    SimpleCallObserver
    simple-call-observer.h
    SimpleObserver
    simple-observer.h
    simple-pending-operations.h
    SimpleTextObserver
    simple-text-observer.h
    StatefulDBusProxy
    StatelessDBusProxy
    StreamTubeChannel
    StreamTubeClient
    StreamTubeServer
    stream-tube-channel.h
    stream-tube-client.h
    stream-tube-server.h
    StreamedMediaChannel
    streamed-media-channel.h
    StreamedMediaStream
    TextChannel
    text-channel.h
    tls-certificate.h
    TubeChannel
    tube-channel.h
    Types
    types.h
    Utils
    utils.h
    SharedPtr)

# Generated headers which will be installed and exported
set(telepathy_qt_gen_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-content.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-content-media-description.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-stream.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-stream-endpoint.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatcher.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatch-operation.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-request.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-client.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-dbus.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-debug-receiver.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-session-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-stream-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-properties.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-tls-certificate.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)

# Headers file moc will be run on
set(telepathy_qt_MOC_SRCS
    abstract-interface.h
    account.h
    account-factory.h
    account-manager.h
    account-set.h
    account-set-internal.h
    call-channel.h
    call-content.h
    call-stream.h
    captcha-authentication.h
    captcha-authentication-internal.h
    channel.h
    channel-dispatch-operation.h
    channel-dispatch-operation-internal.h
    channel-factory.h
    channel-internal.h
    channel-request.h
    client-registrar.h
    client-registrar-internal.h
    connection.h
    connection-internal.h
    connection-lowlevel.h
    connection-manager.h
    connection-manager-internal.h
    connection-manager-lowlevel.h
    contact.h
    contact-manager.h
    contact-manager-internal.h
    contact-messenger.h
    contact-search-channel.h
    contact-search-channel-internal.h
    dbus-proxy.h
    dbus-proxy-factory.h
    dbus-proxy-factory-internal.h
    debug-receiver.h
    dbus-tube-channel.h
    fake-handler-manager-internal.h
    file-transfer-channel.h
    fixed-feature-factory.h
    handled-channel-notifier.h
    incoming-dbus-tube-channel.h
    incoming-file-transfer-channel.h
    incoming-stream-tube-channel.h
    object.h
    outgoing-dbus-tube-channel.h
    outgoing-file-transfer-channel.h
    outgoing-stream-tube-channel.h
    outgoing-stream-tube-channel-internal.h
    pending-account.h
    pending-captchas.h
    pending-channel.h
    pending-channel-request.h
    pending-channel-request-internal.h
    pending-connection.h
    pending-contact-attributes.h
    pending-contact-info.h
    pending-contacts.h
    pending-contacts-internal.h
    pending-dbus-tube-connection.h
    pending-debug-message-list.h
    pending-handles.h
    pending-operation.h
    pending-ready.h
    pending-send-message.h
    pending-stream-tube-connection.h
    pending-string.h
    pending-string-list.h
    pending-variant.h
    pending-variant-map.h
    profile-manager.h
    readiness-helper.h
    request-temporary-handler-internal.h
    room-list-channel.h
    server-authentication-channel.h
    simple-call-observer.h
    simple-pending-operations.h
    simple-observer.h
    simple-observer-internal.h
    simple-stream-tube-handler.h
    simple-text-observer.h
    simple-text-observer-internal.h
    stream-tube-channel.h
    stream-tube-client.h
    stream-tube-client-internal.h
    stream-tube-server.h
    stream-tube-server-internal.h
    streamed-media-channel.h
    text-channel.h
    tube-channel.h)

# Sources for test library, used by tests to test some unexported functionality
set(telepathy_qt_test_backdoors_SRCS
    key-file.cpp
    manager-file.cpp
    test-backdoors.cpp
    utils.cpp)

# Generate the spec files for both stable and future spec
set(gen_stable_spec_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml)
set(gen_future_spec_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml)

tpqt_xincludator(stable-ifaces-includator ${CMAKE_CURRENT_SOURCE_DIR}/stable-interfaces.xml ${gen_stable_spec_xml})
tpqt_xincludator(future-ifaces-includator ${CMAKE_CURRENT_SOURCE_DIR}/future-interfaces.xml ${gen_future_spec_xml})

add_custom_target(all-generated-sources)

tpqt_constants_gen(stable-constants ${gen_stable_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
                    --namespace=Tp
                    --define-prefix=TP_QT_
                    --must-define=IN_TP_QT_HEADER
                    DEPENDS stable-ifaces-includator)
tpqt_constants_gen(future-constants ${gen_future_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-constants.h
                    --namespace=TpFuture
                    --define-prefix=TP_QT_FUTURE_
                    DEPENDS future-ifaces-includator)

tpqt_types_gen(stable-typesgen ${gen_stable_spec_xml}
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp
                Tp TelepathyQt/types.h TelepathyQt/Types
                --must-define=IN_TP_QT_HEADER
                --visibility=TP_QT_EXPORT
                DEPENDS stable-constants)
tpqt_types_gen(future-typesgen ${gen_future_spec_xml}
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types-body.hpp
                TpFuture TelepathyQt/future-internal.h TelepathyQt/future-internal.h
                DEPENDS future-constants)

# Add the generated types to the library's sources
list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)
list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp)
list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-constants.h)
list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h)
list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types-body.hpp)

# For each spec, both stable and future, generate a cli file and add it to the sources (including mocs).
set(SPECS
    account
    account-manager
    call-content
    call-content-media-description
    call-stream
    call-stream-endpoint
    channel
    channel-dispatcher
    channel-dispatch-operation
    channel-request
    client
    connection
    connection-manager
    dbus
    debug-receiver
    media-session-handler
    media-stream-handler
    properties
    tls-certificate)
foreach(spec ${SPECS})
    tpqt_xincludator(${spec}-spec-xincludator ${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
                      DEPENDS stable-typesgen)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
    list(APPEND telepathy_qt_SRCS ${NEW_FILES})
    list(APPEND telepathy_qt_generated_specs_mocs "moc-cli-${spec}.moc.hpp")
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach(spec ${SPECS})

set(FUTURE_SPECS
    channel
    channel-dispatcher
    misc)
foreach(spec ${FUTURE_SPECS})
    tpqt_xincludator(${spec}-future-xincludator  ${CMAKE_CURRENT_SOURCE_DIR}/future-${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml
                      DEPENDS stable-typesgen future-typesgen)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.moc.hpp)
    list(APPEND telepathy_qt_SRCS ${NEW_FILES})
    list(APPEND telepathy_qt_generated_specs_mocs "moc-future-${spec}.moc.hpp")
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach(spec ${FUTURE_SPECS})

# The escape character in MSVC is ^
if(MSVC)
    set(TYPES_INCLUDE ^<TelepathyQt/Types^> )
else(MSVC)
    set(TYPES_INCLUDE '<TelepathyQt/Types>' )
endif(MSVC)

# Use the client generator for generating headers out of specs
tpqt_client_generator(account clientaccount AccountManager Tp::Client --mainiface=Tp::Client::AccountInterface DEPENDS account-spec-xincludator)
tpqt_client_generator(account-manager clientam AccountManager Tp::Client --mainiface=Tp::Client::AccountManagerInterface DEPENDS account-manager-spec-xincludator)
tpqt_client_generator(call-content clientcall CallContent Tp::Client --mainiface=Tp::Client::CallContentInterface DEPENDS call-content-spec-xincludator)
tpqt_client_generator(call-content-media-description clientcall CallContentMediaDescriptionInterface Tp::Client --mainiface=Tp::Client::CallContentMediaDescriptionInterface DEPENDS call-content-media-description-spec-xincludator)
tpqt_client_generator(call-stream clientcall CallStream Tp::Client --mainiface=Tp::Client::CallStreamInterface DEPENDS call-stream-spec-xincludator)
tpqt_client_generator(call-stream-endpoint clientcall CallStreamEndpoint Tp::Client --mainiface=Tp::Client::CallStreamEndpointInterface DEPENDS call-stream-endpoint-spec-xincludator)
tpqt_client_generator(channel clientchannel Channel Tp::Client --mainiface=Tp::Client::ChannelInterface DEPENDS channel-spec-xincludator)
tpqt_client_generator(channel-dispatcher clientchanneldispatcher ChannelDispatcher Tp::Client --mainiface=Tp::Client::ChannelDispatcherInterface DEPENDS channel-dispatcher-spec-xincludator)
tpqt_client_generator(channel-dispatch-operation clientchanneldispatchoperation ChannelDispatchOperation Tp::Client --mainiface=Tp::Client::ChannelDispatchOperationInterface DEPENDS channel-dispatch-operation-spec-xincludator)
tpqt_client_generator(channel-request clientchannelrequest ChannelRequest Tp::Client --mainiface=Tp::Client::ChannelRequestInterface DEPENDS channel-request-spec-xincludator)
tpqt_client_generator(client clientclient Client Tp::Client --mainiface=Tp::Client::ClientInterface DEPENDS client-spec-xincludator)
tpqt_client_generator(connection clientconn Connection Tp::Client --mainiface=Tp::Client::ConnectionInterface DEPENDS connection-spec-xincludator)
tpqt_client_generator(connection-manager clientcm ConnectionManager Tp::Client --mainiface=Tp::Client::ConnectionManagerInterface DEPENDS connection-manager-spec-xincludator)
tpqt_client_generator(dbus clientdbus DBus Tp::Client::DBus DEPENDS dbus-spec-xincludator)
tpqt_client_generator(debug-receiver clientdebug DebugReceiver Tp::Client --mainiface=Tp::Client::DebugInterface DEPENDS debug-receiver-spec-xincludator)
tpqt_client_generator(media-session-handler clientmsesh MediaSessionHandler Tp::Client --mainiface=Tp::Client::MediaSessionHandlerInterface DEPENDS media-session-handler-spec-xincludator)
tpqt_client_generator(media-stream-handler clientmstrh MediaStreamHandler Tp::Client --mainiface=Tp::Client::MediaStreamHandlerInterface DEPENDS media-stream-handler-spec-xincludator)
tpqt_client_generator(properties clientprops Properties Tp::Client DEPENDS properties-spec-xincludator)
tpqt_client_generator(tls-certificate clienttls TLSCertificate Tp::Client DEPENDS tls-certificate-spec-xincludator)

tpqt_future_client_generator(channel TpFuture::Client --mainiface=Tp::Client::ChannelInterface DEPENDS channel-future-xincludator)
tpqt_future_client_generator(channel-dispatcher TpFuture::Client --mainiface=Tp::Client::ChannelDispatcherInterface DEPENDS channel-dispatcher-future-xincludator)
tpqt_future_client_generator(misc TpFuture::Client DEPENDS misc-future-xincludator)

if (TARGET doxygen-doc)
    add_dependencies(doxygen-doc all-generated-sources)
endif (TARGET doxygen-doc)

# Create the library
if (ENABLE_COMPILER_COVERAGE)
    add_library(telepathy-qt${QT_VERSION_MAJOR} STATIC ${telepathy_qt_SRCS})
else (ENABLE_COMPILER_COVERAGE)
    add_library(telepathy-qt${QT_VERSION_MAJOR} SHARED ${telepathy_qt_SRCS})
endif (ENABLE_COMPILER_COVERAGE)

# Library used by tests to test some unexported functionality
add_library(telepathy-qt-test-backdoors STATIC ${telepathy_qt_test_backdoors_SRCS})
add_dependencies(telepathy-qt-test-backdoors stable-constants)
add_dependencies(telepathy-qt-test-backdoors stable-typesgen)

# generate client moc files
foreach(moc_src ${telepathy_qt_MOC_SRCS})
    set(generated_file _gen/${moc_src})
    string(REPLACE ".h" ".moc.hpp" generated_file ${generated_file})
    tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/${generated_file}
                                     ${telepathy_qt_generated_specs_mocs})
    list(APPEND telepathy_qt_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
    string(REPLACE ".h" ".moc.hpp" moc_src ${moc_src})
    add_dependencies(telepathy-qt${QT_VERSION_MAJOR} "moc-${moc_src}")
endforeach(moc_src ${telepathy_qt_MOC_SRCS})

# Link
target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${TP_QT_LIBRARY_LINKER_FLAGS})
target_include_directories(telepathy-qt${QT_VERSION_MAJOR} PUBLIC "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/>")

# Link - Library used by tests to test some unexported functionality
target_link_libraries(telepathy-qt-test-backdoors
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${TP_QT_LIBRARY_LINKER_FLAGS})

if (ENABLE_COMPILER_COVERAGE)
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR} gcov)
    target_link_libraries(telepathy-qt-test-backdoors gcov)
endif (ENABLE_COMPILER_COVERAGE)

# Set the correct version number
set_target_properties(telepathy-qt${QT_VERSION_MAJOR} PROPERTIES
                                    SOVERSION ${TP_QT_ABI_VERSION}
                                    VERSION ${TP_QT_LIBRARY_VERSION})


# Install header files
install(FILES ${telepathy_qt_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt COMPONENT headers)
install(FILES ${telepathy_qt_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/_gen COMPONENT headers)

# Install the library - watch out for the correct components
if (WIN32)
    install(TARGETS telepathy-qt${QT_VERSION_MAJOR}
            EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
            ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libs)
else (WIN32)
    install(TARGETS telepathy-qt${QT_VERSION_MAJOR}
            EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
            ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libs)
endif (WIN32)

# pkg-config files, only if we are not using windows
if (NOT WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}.pc)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}-uninstalled.pc)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT pkgconfig)
endif (NOT WIN32)

# CMake Version and config files
include(MacroWriteBasicCMakeVersionFile)

# all the following variables are put into TelepathyQt*Config.cmake, so
# they are usable by projects using TelepathyQt.
make_install_path_absolute(TELEPATHY_QT_INCLUDE_DIR         ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR})
make_install_path_absolute(TELEPATHY_QT_LIB_DIR             ${LIB_INSTALL_DIR})
make_install_path_absolute(TELEPATHY_QT_DATA_DIR            ${DATA_INSTALL_DIR})

# Configure the actual Config file
configure_file(TelepathyQtConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Config.cmake" @ONLY)

# this file is used by to check if the installed version can be used.
macro_write_basic_cmake_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ConfigVersion.cmake
                                     ${PACKAGE_VERSION})

# Find out the correct installation directory
if (USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
    set(_TelepathyQtConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR})
else (USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
    set(_TelepathyQtConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/TelepathyQt${QT_VERSION_MAJOR}/cmake)
endif (USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

install(EXPORT TelepathyQt${QT_VERSION_MAJOR}Targets
        DESTINATION ${_TelepathyQtConfig_INSTALL_DIR}
        COMPONENT headers)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ConfigVersion.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Config.cmake
        DESTINATION ${_TelepathyQtConfig_INSTALL_DIR}
        COMPONENT headers)

add_subdirectory(Farstream)

if(ENABLE_SERVICE_SUPPORT)
    # lets build tp-qt service side support as a separate library until we can guarantee API/ABI
    # stability
    set(telepathy_qt_service_SRCS
        base-call.cpp
        base-connection-manager.cpp
        base-connection.cpp
        base-channel.cpp
        base-debug.cpp
        base-protocol.cpp
        dbus-error.cpp
        dbus-object.cpp
        dbus-service.cpp
        io-device.cpp
        abstract-adaptor.cpp)

    set(telepathy_qt_service_HEADERS
        AbstractAdaptor
        abstract-adaptor.h
        AbstractDBusServiceInterface
        AbstractProtocolInterface
        BaseCall
        base-call.h
        BaseConnectionManager
        base-connection-manager.h
        BaseConnection
        base-connection.h
        BaseChannel
        base-channel.h
        BaseDebug
        base-debug.h
        BaseProtocol
        BaseProtocolAddressingInterface
        BaseProtocolAvatarsInterface
        BaseProtocolPresenceInterface
        base-protocol.h
        DBusError
        dbus-error.h
        DBusObject
        dbus-object.h
        DBusService
        dbus-service.h
        IODevice
        io-device.h
        ServiceTypes
        service-types.h)

    # Generated headers which will be installed and exported
    set(telepathy_qt_service_gen_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-channel.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-call.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection-manager.h)

    # Headers file moc will be run on
    set(telepathy_qt_service_MOC_SRCS
        abstract-adaptor.h
        base-call.h
        base-call-internal.h
        base-connection-manager.h
        base-connection-manager-internal.h
        base-channel.h
        base-channel-internal.h
        base-connection.h
        base-connection-internal.h
        base-debug.h
        base-debug-internal.h
        base-protocol.h
        base-protocol-internal.h
        dbus-object.h
        io-device.h
        dbus-service.h)

    add_custom_target(all-generated-service-sources)

    set(SPECS
        svc-channel
        svc-call
        svc-connection
        svc-connection-manager
        svc-debug
        )
    foreach(spec ${SPECS})
        tpqt_xincludator(${spec}-spec-xincludator ${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
                         DEPENDS stable-typesgen)
        set(NEW_FILES
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.h
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.moc.hpp)
        list(APPEND telepathy_qt_service_SRCS ${NEW_FILES})
        list(APPEND telepathy_qt_service_generated_specs_mocs "moc-${spec}.moc.hpp")
        set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
    endforeach(spec ${SPECS})

    tpqt_service_generator(svc-channel servicechannel Channel Tp::Service DEPENDS svc-channel-spec-xincludator)
    tpqt_service_generator(svc-call servicecall Channel Tp::Service DEPENDS svc-call-spec-xincludator)
    tpqt_service_generator(svc-connection serviceconn Connection Tp::Service DEPENDS svc-connection-spec-xincludator)
    tpqt_service_generator(svc-connection-manager servicecm ConnectionManager Tp::Service DEPENDS svc-connection-manager-spec-xincludator)
    tpqt_service_generator(svc-debug servicecm Debug Tp::Service DEPENDS svc-debug-spec-xincludator)

    if (TARGET doxygen-doc)
        add_dependencies(doxygen-doc all-generated-service-sources)
    endif (TARGET doxygen-doc)

    # Create the library
    #if (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})
    #else (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service SHARED ${telepathy_qt_service_SRCS})
    #endif (ENABLE_COMPILER_COVERAGE)

    if(FORCE_STATIC_SERVICE_LIBRARY)
        set(SERVICE_LIBRARY_TYPE STATIC)
    else(FORCE_STATIC_SERVICE_LIBRARY)
        set(SERVICE_LIBRARY_TYPE SHARED)
    endif(FORCE_STATIC_SERVICE_LIBRARY)

    add_library(telepathy-qt${QT_VERSION_MAJOR}-service ${SERVICE_LIBRARY_TYPE} ${telepathy_qt_service_SRCS})
    add_dependencies(telepathy-qt${QT_VERSION_MAJOR}-service stable-typesgen future-typesgen)

    # generate service moc files
    foreach(moc_src ${telepathy_qt_service_MOC_SRCS})
        set(generated_file _gen/${moc_src})
        string(REPLACE ".h" ".moc.hpp" generated_file ${generated_file})
        tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src}
                                        ${CMAKE_CURRENT_BINARY_DIR}/${generated_file}
                                        ${telepathy_qt_service_generated_specs_mocs})
        list(APPEND telepathy_qt_service_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
        string(REPLACE ".h" ".moc.hpp" moc_src ${moc_src})
        add_dependencies(telepathy-qt${QT_VERSION_MAJOR}-service "moc-${moc_src}")
    endforeach(moc_src ${telepathy_qt_service_MOC_SRCS})

    # Link
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-service
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR}
        ${TP_QT_LIBRARY_LINKER_FLAGS})

    # Set the correct version number
    set_target_properties(telepathy-qt${QT_VERSION_MAJOR}-service PROPERTIES
                                                 SOVERSION ${TP_QT_SERVICE_ABI_VERSION}
                                                 VERSION ${TP_QT_LIBRARY_VERSION})

    # Install the library - watch out for the correct components
    if (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-service
                EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
                RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT service
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT service_libs)
    else (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-service
                EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT service
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT service_libs)
    endif (WIN32)

    # Install headers
    install(FILES ${telepathy_qt_service_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt
            COMPONENT service_headers)
    install(FILES ${telepathy_qt_service_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/_gen
            COMPONENT service_headers)

    # pkg-config files, only if not on windows
    if (NOT WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtService.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Service.pc)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtService-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Service-uninstalled.pc)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Service.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT pkgconfig)
    endif (NOT WIN32)

    # Configure the actual Config file
    configure_file(TelepathyQtServiceConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfig.cmake" @ONLY)

    # this file is used by to check if the installed version can be used.
    macro_write_basic_cmake_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfigVersion.cmake
                                         ${PACKAGE_VERSION})

    if(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtServiceConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR}Service)
    else(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtServiceConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/TelepathyQt${QT_VERSION_MAJOR}Service/cmake)
    endif(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfigVersion.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfig.cmake
            DESTINATION ${_TelepathyQtServiceConfig_INSTALL_DIR}
            COMPONENT headers)

endif(ENABLE_SERVICE_SUPPORT)
