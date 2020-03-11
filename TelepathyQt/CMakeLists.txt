file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

# Set the required flags found in TelepathyDefaults
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VISIBILITY_HIDDEN_FLAGS} ${COMPILER_COVERAGE_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")
set(LD_FLAGS "${LD_FLAGS} ${VISIBILITY_HIDDEN_FLAGS} ${COMPILER_COVERAGE_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")

configure_file(global.h.in ${CMAKE_CURRENT_BINARY_DIR}/global.h)

# Sources for Tp-Qt
set(telepathy_qt_SRCS
    abstract-client.cpp
    abstract-interface.cpp
    account-factory.cpp
    account-manager.cpp
    account-property-filter.cpp
    account-set-internal.h
    account-set.cpp
    account.cpp
    avatar.cpp
    call-channel.cpp
    call-content-media-description.cpp
    call-content.cpp
    call-content.cpp
    call-stream-endpoint.cpp
    call-stream.cpp
    call-stream.cpp
    capabilities-base.cpp
    captcha-authentication.cpp
    captcha.cpp
    channel-class-spec.cpp
    channel-dispatch-operation.cpp
    channel-dispatcher.cpp
    channel-factory.cpp
    channel-internal.h
    channel-request.cpp
    channel-requests-properties-internal.cpp
    channel-requests-properties-internal.h
    channel.cpp
    chat-manager.cpp
    client-registrar-internal.h
    client-registrar.cpp
    client.cpp
    connection-capabilities.cpp
    connection-factory.cpp
    connection-internal.h
    connection-manager-internal.h
    connection-manager.cpp
    connection.cpp
    contact-capabilities.cpp
    contact-factory.cpp
    contact-manager-roster.cpp
    contact-manager.cpp
    contact-messenger.cpp
    contact-search-channel.cpp
    contact.cpp
    dbus-proxy-factory-internal.h
    dbus-proxy-factory.cpp
    dbus-proxy.cpp
    dbus-tube-channel.cpp
    dbus.cpp
    debug-internal.h
    debug-receiver.cpp
    debug.cpp
    fake-handler-manager-internal.cpp
    fake-handler-manager-internal.h
    feature.cpp
    file-transfer-channel-creation-properties.cpp
    file-transfer-channel.cpp
    fixed-feature-factory.cpp
    future-internal.h
    future.cpp
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
    message-content-part.cpp
    message.cpp
    object.cpp
    optional-interface-factory.cpp
    outgoing-dbus-tube-channel.cpp
    outgoing-file-transfer-channel.cpp
    outgoing-stream-tube-channel.cpp
    pending-account.cpp
    pending-captchas.cpp
    pending-channel-request-internal.h
    pending-channel-request.cpp
    pending-channel.cpp
    pending-chat-read.cpp
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
    pending-stream-tube-connection.cpp
    pending-string-list.cpp
    pending-string.cpp
    pending-variant-map.cpp
    pending-variant.cpp
    presence.cpp
    profile-manager.cpp
    profile.cpp
    properties.cpp
    protocol-info.cpp
    protocol-parameter.cpp
    readiness-helper.cpp
    ready-object.cpp
    referenced-handles.cpp
    request-temporary-handler-internal.cpp
    request-temporary-handler-internal.h
    requestable-channel-class-spec.cpp
    room-list-channel.cpp
    server-authentication-channel.cpp
    simple-call-observer.cpp
    simple-observer-internal.h
    simple-observer.cpp
    simple-stream-tube-handler.cpp
    simple-text-observer-internal.h
    simple-text-observer.cpp
    stream-tube-channel.cpp
    stream-tube-client-internal.h
    stream-tube-client.cpp
    stream-tube-server-internal.h
    stream-tube-server.cpp
    streamed-media-channel.cpp
    text-channel.cpp
    tls-certificate.cpp
    tube-channel.cpp
    types-internal.h
    types.cpp
    utils.cpp
)

# Exported headers for Tp-Qt
set(telepathy_qt_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/global.h
    AbstractClient
    AbstractClientApprover
    AbstractClientHandler
    AbstractClientObserver
    AbstractInterface
    Account
    AccountCapabilityFilter
    AccountFactory
    AccountFilter
    AccountInterface
    AccountInterfaceAddressingInterface
    AccountInterfaceAvatarInterface
    AccountInterfaceStorageInterface
    AccountManager
    AccountManagerInterface
    AccountPropertyFilter
    AccountSet
    AndFilter
    AuthenticationTLSCertificateInterface
    AvatarData
    AvatarSpec
    CallChannel
    CallContent
    CallContentInterface
    CallContentInterfaceAudioControlInterface
    CallContentInterfaceDTMFInterface
    CallContentInterfaceMediaInterface
    CallContentInterfaceVideoControlInterface
    CallContentMediaDescription
    CallContentMediaDescriptionInterface
    CallContentMediaDescriptionInterfaceRTCPExtendedReportsInterface
    CallContentMediaDescriptionInterfaceRTCPFeedbackInterface
    CallContentMediaDescriptionInterfaceRTPHeaderExtensionsInterface
    CallStream
    CallStreamEndpoint
    CallStreamEndpoint
    CallStreamEndpointInterface
    CallStreamInterface
    CallStreamInterfaceMediaInterface
    Callbacks
    CapabilitiesBase
    Captcha
    CaptchaAuthentication
    Channel
    ChannelClassFeatures
    ChannelClassSpec
    ChannelClassSpecList
    ChannelDispatchOperation
    ChannelDispatchOperationInterface
    ChannelDispatcher
    ChannelDispatcherInterface
    ChannelFactory
    ChannelInterface
    ChannelInterfaceAnonymityInterface
    ChannelInterfaceCallStateInterface
    ChannelInterfaceCaptchaAuthenticationInterface
    ChannelInterfaceChatStateInterface
    ChannelInterfaceConferenceInterface
    ChannelInterfaceDTMFInterface
    ChannelInterfaceDestroyableInterface
    ChannelInterfaceFileTransferMetadataInterface
    ChannelInterfaceGroupInterface
    ChannelInterfaceHoldInterface
    ChannelInterfaceMediaSignallingInterface
    ChannelInterfaceMessagesInterface
    ChannelInterfacePasswordInterface
    ChannelInterfaceRoomConfigInterface
    ChannelInterfaceRoomInterface
    ChannelInterfaceSASLAuthenticationInterface
    ChannelInterfaceSMSInterface
    ChannelInterfaceSecurableInterface
    ChannelInterfaceServicePointInterface
    ChannelInterfaceSubjectInterface
    ChannelInterfaceTubeInterface
    ChannelRequest
    ChannelRequestHints
    ChannelRequestInterface
    ChannelTypeCallInterface
    ChannelTypeContactListInterface
    ChannelTypeContactSearchInterface
    ChannelTypeDBusTubeInterface
    ChannelTypeFileTransferInterface
    ChannelTypeRoomListInterface
    ChannelTypeServerAuthenticationInterface
    ChannelTypeServerTLSConnectionInterface
    ChannelTypeStreamTubeInterface
    ChannelTypeStreamedMediaInterface
    ChannelTypeTextInterface
    ChannelTypeTubeInterface
    ChannelTypeTubesInterface
    ChatManager
    Client
    ClientApproverInterface
    ClientHandlerInterface
    ClientInterface
    ClientInterfaceRequestsInterface
    ClientObserverInterface
    ClientRegistrar
    Connection
    ConnectionCapabilities
    ConnectionFactory
    ConnectionInterface
    ConnectionInterfaceAddressingInterface
    ConnectionInterfaceAliasingInterface
    ConnectionInterfaceAnonymityInterface
    ConnectionInterfaceAvatarsInterface
    ConnectionInterfaceBalanceInterface
    ConnectionInterfaceCapabilitiesInterface
    ConnectionInterfaceCellularInterface
    ConnectionInterfaceClientTypesInterface
    ConnectionInterfaceContactBlockingInterface
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
    ConnectionManagerInterface
    ConnectionManagerLowlevel
    Constants
    Contact
    ContactCapabilities
    ContactFactory
    ContactManager
    ContactMessenger
    ContactSearchChannel
    DBus
    DBusDaemonInterface
    DBusProxy
    DBusProxyFactory
    DBusTubeChannel
    Debug
    DebugReceiver
    Feature
    Features
    FileTransferChannel
    FileTransferChannelCreationProperties
    Filter
    FixedFeatureFactory
    Functors
    GenericCapabilityFilter
    GenericPropertyFilter
    Global
    HandledChannelNotifier
    IncomingDBusTubeChannel
    IncomingFileTransferChannel
    IncomingStreamTubeChannel
    IntrospectableInterface
    LocationInfo
    MediaSessionHandler
    MediaSessionHandlerInterface
    MediaStreamHandler
    MediaStreamHandlerInterface
    Message
    MessageContentPart
    MessageContentPartList
    MethodInvocationContext
    NotFilter
    Object
    OptionalInterfaceFactory
    OrFilter
    OutgoingDBusTubeChannel
    OutgoingFileTransferChannel
    OutgoingStreamTubeChannel
    PeerInterface
    PendingAccount
    PendingCallContent
    PendingCaptchas
    PendingChannel
    PendingChannelRequest
    PendingComposite
    PendingConnection
    PendingContactAttributes
    PendingContactInfo
    PendingContacts
    PendingDBusTubeConnection
    PendingDebugMessageList
    PendingFailure
    PendingHandles
    PendingOperation
    PendingReady
    PendingSendMessage
    PendingStreamTubeConnection
    PendingStreamedMediaStreams
    PendingString
    PendingStringList
    PendingSuccess
    PendingVariant
    PendingVariantMap
    PendingVoid
    Presence
    PresenceSpec
    PresenceSpecList
    Profile
    ProfileManager
    Properties
    PropertiesInterface
    PropertiesInterfaceInterface
    ProtocolInfo
    ProtocolInfoList
    ProtocolInterface
    ProtocolInterfaceAddressingInterface
    ProtocolInterfaceAvatarsInterface
    ProtocolInterfacePresenceInterface
    ProtocolParameter
    ProtocolParameterList
    ReadinessHelper
    ReadyObject
    ReceivedMessage
    RefCounted
    ReferencedHandles
    ReferencedHandlesIterator
    RequestableChannelClassSpec
    RequestableChannelClassSpecList
    RoomListChannel
    ServerAuthenticationChannel
    SharedPtr
    SharedPtr
    SimpleCallObserver
    SimpleObserver
    SimpleTextObserver
    StatefulDBusProxy
    StatelessDBusProxy
    StreamTubeChannel
    StreamTubeClient
    StreamTubeServer
    StreamedMediaChannel
    StreamedMediaStream
    TextChannel
    TubeChannel
    Types
    Utils
    abstract-client.h
    abstract-interface.h
    account-capability-filter.h
    account-factory.h
    account-filter.h
    account-manager.h
    account-property-filter.h
    account-set.h
    account.h
    and-filter.h
    avatar.h
    call-channel.h
    call-content-media-description.h
    call-content.h
    call-stream-endpoint.h
    call-stream.h
    callbacks.h
    capabilities-base.h
    captcha-authentication.h
    captcha.h
    channel-class-features.h
    channel-class-spec.h
    channel-dispatch-operation.h
    channel-dispatcher.h
    channel-factory.h
    channel-request.h
    channel.h
    chat-manager.h
    client-registrar.h
    client.h
    connection-capabilities.h
    connection-factory.h
    connection-lowlevel.h
    connection-manager-lowlevel.h
    connection-manager.h
    connection.h
    constants.h
    contact-capabilities.h
    contact-factory.h
    contact-manager.h
    contact-messenger.h
    contact-search-channel.h
    contact.h
    dbus-proxy-factory.h
    dbus-proxy.h
    dbus-tube-channel.h
    dbus.h
    debug-receiver.h
    debug.h
    feature.h
    file-transfer-channel-creation-properties.h
    file-transfer-channel.h
    filter.h
    fixed-feature-factory.h
    functors.h
    generic-capability-filter.h
    generic-property-filter.h
    handled-channel-notifier.h
    incoming-dbus-tube-channel.h
    incoming-file-transfer-channel.h
    incoming-stream-tube-channel.h
    location-info.h
    media-session-handler.h
    media-stream-handler.h
    message-content-part.h
    message.h
    method-invocation-context.h
    not-filter.h
    object.h
    optional-interface-factory.h
    or-filter.h
    outgoing-dbus-tube-channel.h
    outgoing-file-transfer-channel.h
    outgoing-stream-tube-channel.h
    pending-account.h
    pending-captchas.h
    pending-channel-request.h
    pending-channel.h
    pending-connection.h
    pending-contact-attributes.h
    pending-contact-info.h
    pending-contacts.h
    pending-dbus-tube-connection.h
    pending-debug-message-list.h
    pending-handles.h
    pending-operation.h
    pending-ready.h
    pending-send-message.h
    pending-stream-tube-connection.h
    pending-string-list.h
    pending-string.h
    pending-variant-map.h
    pending-variant.h
    presence.h
    profile-manager.h
    profile.h
    properties.h
    protocol-info.h
    protocol-parameter.h
    readiness-helper.h
    ready-object.h
    referenced-handles.h
    requestable-channel-class-spec.h
    room-list-channel.h
    server-authentication-channel.h
    shared-ptr.h
    simple-call-observer.h
    simple-observer.h
    simple-pending-operations.h
    simple-text-observer.h
    stream-tube-channel.h
    stream-tube-client.h
    stream-tube-server.h
    streamed-media-channel.h
    text-channel.h
    tls-certificate.h
    tube-channel.h
    types.h
    utils.h
)

# Generated headers which will be installed and exported
set(telepathy_qt_gen_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-content-media-description.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-content.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-stream-endpoint.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-call-stream.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatch-operation.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatcher.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-request.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-client.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-dbus.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-debug-receiver.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-session-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-stream-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-properties.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-tls-certificate.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h
)

# Headers file moc will be run on
set(telepathy_qt_MOC_SRCS
    abstract-interface.h
    account-factory.h
    account-manager.h
    account-set-internal.h
    account-set.h
    account.h
    call-channel.h
    call-content.h
    call-stream.h
    captcha-authentication-internal.h
    captcha-authentication.h
    channel-dispatch-operation-internal.h
    channel-dispatch-operation.h
    channel-factory.h
    channel-internal.h
    channel-request.h
    channel.h
    chat-manager.h
    client-registrar-internal.h
    client-registrar.h
    connection-internal.h
    connection-lowlevel.h
    connection-manager-internal.h
    connection-manager-lowlevel.h
    connection-manager.h
    connection.h
    contact-manager-internal.h
    contact-manager.h
    contact-messenger.h
    contact-search-channel-internal.h
    contact-search-channel.h
    contact.h
    dbus-proxy-factory-internal.h
    dbus-proxy-factory.h
    dbus-proxy.h
    dbus-tube-channel.h
    debug-receiver.h
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
    outgoing-stream-tube-channel-internal.h
    outgoing-stream-tube-channel.h
    pending-account.h
    pending-captchas.h
    pending-channel-request-internal.h
    pending-channel-request.h
    pending-channel.h
    pending-chat-read.h
    pending-connection.h
    pending-contact-attributes.h
    pending-contact-info.h
    pending-contacts-internal.h
    pending-contacts.h
    pending-dbus-tube-connection.h
    pending-debug-message-list.h
    pending-handles.h
    pending-operation.h
    pending-ready.h
    pending-send-message.h
    pending-stream-tube-connection.h
    pending-string-list.h
    pending-string.h
    pending-variant-map.h
    pending-variant.h
    profile-manager.h
    readiness-helper.h
    request-temporary-handler-internal.h
    room-list-channel.h
    server-authentication-channel.h
    simple-call-observer.h
    simple-observer-internal.h
    simple-observer.h
    simple-pending-operations.h
    simple-stream-tube-handler.h
    simple-text-observer-internal.h
    simple-text-observer.h
    stream-tube-channel.h
    stream-tube-client-internal.h
    stream-tube-client.h
    stream-tube-server-internal.h
    stream-tube-server.h
    streamed-media-channel.h
    text-channel.h
    tube-channel.h
)

# Sources for test library, used by tests to test some unexported functionality
set(telepathy_qt_test_backdoors_SRCS
    key-file.cpp
    manager-file.cpp
    test-backdoors.cpp
    utils.cpp
)

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
    channel-dispatch-operation
    channel-dispatcher
    channel-request
    client
    connection
    connection-manager
    dbus
    debug-receiver
    media-session-handler
    media-stream-handler
    properties
    tls-certificate
)

foreach(spec ${SPECS})
    tpqt_xincludator(${spec}-spec-xincludator ${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
                      DEPENDS stable-typesgen)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
    list(APPEND telepathy_qt_SRCS ${NEW_FILES})
    list(APPEND telepathy_qt_generated_specs_mocs "moc-cli-${spec}.moc.hpp")
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach()

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
endforeach()

# The escape character in MSVC is ^
if(MSVC)
    set(TYPES_INCLUDE ^<TelepathyQt/Types^> )
else()
    set(TYPES_INCLUDE '<TelepathyQt/Types>' )
endif()

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
endif ()

# Create the library
if (ENABLE_COMPILER_COVERAGE)
    add_library(telepathy-qt${QT_VERSION_MAJOR} STATIC ${telepathy_qt_SRCS})
else ()
    add_library(telepathy-qt${QT_VERSION_MAJOR} SHARED ${telepathy_qt_SRCS})
endif ()
add_library(TelepathyQt${QT_VERSION_MAJOR}::Core ALIAS telepathy-qt${QT_VERSION_MAJOR})

# We are building Telepathy-Qt
target_compile_definitions(telepathy-qt${QT_VERSION_MAJOR} PRIVATE BUILDING_TP_QT)

# Library used by tests to test some unexported functionality
add_library(telepathy-qt-test-backdoors STATIC ${telepathy_qt_test_backdoors_SRCS})
target_include_directories(telepathy-qt-test-backdoors PUBLIC
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)

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
endforeach()

# Link
target_link_libraries(telepathy-qt${QT_VERSION_MAJOR} PUBLIC
    Qt5::Core
    Qt5::DBus
    Qt5::Network
)

target_link_libraries(telepathy-qt${QT_VERSION_MAJOR} PRIVATE
    Qt5::Xml
)

target_link_libraries(telepathy-qt${QT_VERSION_MAJOR} PUBLIC
    ${TP_QT_LIBRARY_LINKER_FLAGS})

target_include_directories(telepathy-qt${QT_VERSION_MAJOR} PUBLIC
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)

# Link - Library used by tests to test some unexported functionality
target_link_libraries(telepathy-qt-test-backdoors PUBLIC
    Qt5::Core
    Qt5::DBus
    Qt5::Network
    Qt5::Xml
)

target_link_libraries(telepathy-qt-test-backdoors PUBLIC
    ${TP_QT_LIBRARY_LINKER_FLAGS})

if (ENABLE_COMPILER_COVERAGE)
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR} PRIVATE gcov)
    target_link_libraries(telepathy-qt-test-backdoors PRIVATE gcov)
endif ()

# Set the correct version number
set_target_properties(telepathy-qt${QT_VERSION_MAJOR} PROPERTIES
                                    EXPORT_NAME Core
                                    SOVERSION ${TP_QT_ABI_VERSION}
                                    VERSION ${TP_QT_LIBRARY_VERSION})


# Install header files
install(FILES ${telepathy_qt_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt COMPONENT headers)
install(FILES ${telepathy_qt_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/_gen COMPONENT headers)

# Install the library
install(TARGETS telepathy-qt${QT_VERSION_MAJOR}
        EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT libs)

# pkg-config files, only if we are not using windows
if (NOT WIN32)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}.pc)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}-uninstalled.pc)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT pkgconfig)
endif ()

# Find out the correct installation directory
set(_TelepathyQtConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR})

# CMake Version and config files
include(CMakePackageConfigHelpers)

# Configure the actual Config file
configure_package_config_file(TelepathyQtConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Config.cmake"
    INSTALL_DESTINATION ${_TelepathyQtConfig_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_PREFIX INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# this file is used by to check if the installed version can be used.
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ConfigVersion.cmake
                                 VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)

install(EXPORT TelepathyQt${QT_VERSION_MAJOR}Targets
        DESTINATION ${_TelepathyQtConfig_INSTALL_DIR}
        NAMESPACE TelepathyQt${QT_VERSION_MAJOR}::
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
        abstract-adaptor.cpp
        base-call.cpp
        base-channel.cpp
        base-connection-manager.cpp
        base-connection.cpp
        base-debug.cpp
        base-protocol.cpp
        dbus-error.cpp
        dbus-object.cpp
        dbus-service.cpp
        io-device.cpp
    )

    set(telepathy_qt_service_HEADERS
        AbstractAdaptor
        AbstractDBusServiceInterface
        AbstractProtocolInterface
        BaseCall
        BaseChannel
        BaseConnection
        BaseConnectionManager
        BaseDebug
        BaseProtocol
        BaseProtocolAddressingInterface
        BaseProtocolAvatarsInterface
        BaseProtocolPresenceInterface
        DBusError
        DBusObject
        DBusService
        IODevice
        ServiceTypes
        abstract-adaptor.h
        base-call.h
        base-channel.h
        base-connection-manager.h
        base-connection.h
        base-debug.h
        base-protocol.h
        dbus-error.h
        dbus-object.h
        dbus-service.h
        io-device.h
        service-types.h
    )

    # Generated headers which will be installed and exported
    set(telepathy_qt_service_gen_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-call.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-channel.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection-manager.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection.h
    )

    # Headers file moc will be run on
    set(telepathy_qt_service_MOC_SRCS
        abstract-adaptor.h
        base-call-internal.h
        base-call.h
        base-channel-internal.h
        base-channel.h
        base-connection-internal.h
        base-connection-manager-internal.h
        base-connection-manager.h
        base-connection.h
        base-debug-internal.h
        base-debug.h
        base-protocol-internal.h
        base-protocol.h
        dbus-object.h
        dbus-service.h
        io-device.h
    )

    add_custom_target(all-generated-service-sources)

    set(SPECS
        svc-call
        svc-channel
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
    endforeach()

    tpqt_service_generator(svc-channel servicechannel Channel Tp::Service DEPENDS svc-channel-spec-xincludator)
    tpqt_service_generator(svc-call servicecall Channel Tp::Service DEPENDS svc-call-spec-xincludator)
    tpqt_service_generator(svc-connection serviceconn Connection Tp::Service DEPENDS svc-connection-spec-xincludator)
    tpqt_service_generator(svc-connection-manager servicecm ConnectionManager Tp::Service DEPENDS svc-connection-manager-spec-xincludator)
    tpqt_service_generator(svc-debug servicecm Debug Tp::Service DEPENDS svc-debug-spec-xincludator)

    if (TARGET doxygen-doc)
        add_dependencies(doxygen-doc all-generated-service-sources)
    endif ()

    # Create the library
    #if (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})
    #else (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service SHARED ${telepathy_qt_service_SRCS})
    #endif (ENABLE_COMPILER_COVERAGE)

    if(FORCE_STATIC_SERVICE_LIBRARY)
        set(SERVICE_LIBRARY_TYPE STATIC)
    else()
        set(SERVICE_LIBRARY_TYPE SHARED)
    endif()

    add_library(telepathy-qt${QT_VERSION_MAJOR}-service ${SERVICE_LIBRARY_TYPE} ${telepathy_qt_service_SRCS})
    add_library(TelepathyQt${QT_VERSION_MAJOR}::Service ALIAS telepathy-qt${QT_VERSION_MAJOR}-service)
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
    endforeach()

    # Link
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-service PUBLIC
        Qt5::Core
        Qt5::DBus
        telepathy-qt${QT_VERSION_MAJOR})

    # Set the correct version number
    set_target_properties(telepathy-qt${QT_VERSION_MAJOR}-service PROPERTIES
                                                 EXPORT_NAME Service
                                                 SOVERSION ${TP_QT_SERVICE_ABI_VERSION}
                                                 VERSION ${TP_QT_LIBRARY_VERSION})

    # Install the library
    install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-service
            EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT service
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT service
            ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT service_libs)

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
    endif ()

    set(_TelepathyQtServiceConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR}Service)

    # Configure the actual Config file
    configure_package_config_file(TelepathyQtServiceConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfig.cmake"
        INSTALL_DESTINATION ${_TelepathyQtServiceConfig_INSTALL_DIR}
        PATH_VARS CMAKE_INSTALL_PREFIX INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
        NO_CHECK_REQUIRED_COMPONENTS_MACRO)

    # this file is used by to check if the installed version can be used.
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfigVersion.cmake
                                     VERSION ${PACKAGE_VERSION} COMPATIBILITY SameMajorVersion)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfigVersion.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}ServiceConfig.cmake
            DESTINATION ${_TelepathyQtServiceConfig_INSTALL_DIR}
            COMPONENT headers)

endif()
