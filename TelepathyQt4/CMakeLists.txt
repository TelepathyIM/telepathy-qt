file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VISIBILITY_HIDDEN_FLAGS}")

add_definitions(-DBUILDING_TELEPATHY_QT4)

set(telepathy_qt4_SRCS
    abstract-client.cpp
    abstract-interface.cpp
    account.cpp
    account-factory.cpp
    account-manager.cpp
    account-set.cpp
    account-set-internal.h
    capabilities-base.cpp
    channel.cpp
    channel-factory.cpp
    channel-factory.h
    channel-request.cpp
    channel-dispatcher.cpp
    channel-dispatch-operation.cpp
    client.cpp
    client-registrar.cpp
    client-registrar-internal.h
    connection.cpp
    connection-capabilities.cpp
    connection-factory.cpp
    connection-manager.cpp
    connection-internal.h
    connection-manager-internal.h
    contact.cpp
    contact-capabilities.cpp
    contact-factory.cpp
    contact-location.cpp
    contact-manager.cpp
    dbus.cpp
    dbus-proxy.cpp
    dbus-proxy-factory.cpp
    dbus-proxy-factory-internal.h
    debug.cpp
    debug-internal.h
    feature.cpp
    file-transfer-channel.cpp
    file-transfer-channel-creation-properties.cpp
    fixed-feature-factory.cpp
    future.cpp
    future-internal.h
    incoming-file-transfer-channel.cpp
    key-file.cpp
    manager-file.cpp
    media-session-handler.cpp
    media-stream-handler.cpp
    message.cpp
    optional-interface-factory.cpp
    outgoing-file-transfer-channel.cpp
    pending-account.cpp
    pending-channel.cpp
    pending-channel-request.cpp
    pending-channel-request-internal.h
    pending-connection.cpp
    pending-contact-attributes.cpp
    pending-contact-info.cpp
    pending-contacts.cpp
    pending-handles.cpp
    pending-operation.cpp
    pending-ready.cpp
    pending-string-list.cpp
    pending-variant.cpp
    properties.cpp
    readiness-helper.cpp
    ready-object.cpp
    referenced-handles.cpp
    room-list-channel.cpp
    streamed-media-channel.cpp
    streamed-media-channel-internal.h
    test-backdoors.cpp
    test-backdoors.h
    text-channel.cpp
    types.cpp
    types-internal.h
    utils.cpp)

set(telepathy_qt4_HEADERS
    AbstractClient
    AbstractClientApprover
    abstract-client.h
    AbstractClientHandler
    AbstractClientObserver
    AbstractInterface
    abstract-interface.h
    Account
    account.h
    AccountFactory
    account-factory.h
    AccountInterface
    AccountManager
    account-manager.h
    AccountManagerInterface
    AccountSet
    account-set.h
    CapabilitiesBase
    capabilities-base.h
    Channel
    ChannelDispatcher
    channel-dispatcher.h
    ChannelDispatcherInterface
    ChannelDispatchOperation
    channel-dispatch-operation.h
    ChannelDispatchOperationInterface
    ChannelFactory
    channel-factory.h
    channel.h
    ChannelInterface
    ChannelInterfaceCallStateInterface
    ChannelInterfaceChatStateInterface
    ChannelInterfaceDTMFInterface
    ChannelInterfaceGroupInterface
    ChannelInterfaceHoldInterface
    ChannelInterfaceMediaSignallingInterface
    ChannelInterfacePasswordInterface
    ChannelRequest
    channel-request.h
    ChannelRequestInterface
    ChannelTypeContactListInterface
    ChannelTypeContactSearchInterface
    ChannelTypeRoomListInterface
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
    ConnectionInterface
    ConnectionInterfaceAliasingInterface
    ConnectionInterfaceAnonymityInterface
    ConnectionInterfaceAvatarsInterface
    ConnectionInterfaceBalanceInterface
    ConnectionInterfaceCapabilitiesInterface
    ConnectionInterfaceCellularInterface
    ConnectionInterfaceContactCapabilitiesInterface
    ConnectionInterfaceContactInfoInterface
    ConnectionInterfaceContactsInterface
    ConnectionInterfaceLocationInterface
    ConnectionInterfacePresenceInterface
    ConnectionInterfaceRequestsInterface
    ConnectionInterfaceServicePointInterface
    ConnectionInterfaceSimplePresenceInterface
    ConnectionManager
    connection-manager.h
    ConnectionManagerInterface
    Constants
    constants.h
    Contact
    contact.h
    ContactCapabilities
    contact-capabilities.h
    ContactFactory
    contact-factory.h
    ContactLocation
    contact-location.h
    ContactManager
    contact-manager.h
    DBus
    DBusDaemonInterface
    dbus.h
    DBusProxy
    dbus-proxy.h
    DBusProxyFactory
    dbus-proxy-factory.h
    Debug
    debug.h
    Feature
    feature.h
    FileTransferChannel
    FileTransferChannelCreationProperties
    file-transfer-channel-creation-properties.h
    file-transfer-channel.h
    FixedFeatureFactory
    fixed-feature-factory.h
    Global
    global.h
    IncomingFileTransferChannel
    incoming-file-transfer-channel.h
    IntrospectableInterface
    KeyFile
    key-file.h
    ManagerFile
    manager-file.h
    MediaSessionHandler
    media-session-handler.h
    MediaSessionHandlerInterface
    MediaStreamHandler
    media-stream-handler.h
    MediaStreamHandlerInterface
    Message
    message.h
    MethodInvocationContext
    method-invocation-context.h
    OptionalInterfaceFactory
    optional-interface-factory.h
    OutgoingFileTransferChannel
    outgoing-file-transfer-channel.h
    PeerInterface
    PendingAccount
    pending-account.h
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
    PendingFailure
    PendingHandles
    pending-handles.h
    PendingOperation
    pending-operation.h
    PendingReady
    pending-ready.h
    PendingStringList
    pending-string-list.h
    PendingSuccess
    PendingVariant
    pending-variant.h
    PendingVoid
    Properties
    properties.h
    PropertiesInterface
    PropertiesInterfaceInterface
    ReadinessHelper
    readiness-helper.h
    ReadyObject
    ready-object.h
    ReceivedMessage
    RefCounted
    ReferencedHandles
    referenced-handles.h
    ReferencedHandlesIterator
    RoomListChannel
    room-list-channel.h
    SharedPtr
    shared-ptr.h
    simple-pending-operations.h
    StatefulDBusProxy
    StatelessDBusProxy
    StreamedMediaChannel
    streamed-media-channel.h
    TextChannel
    text-channel.h
    Types
    types.h
    Utils
    utils.h
    WeakPtr)

set(telepathy_qt4_gen_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatcher.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-dispatch-operation.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel-request.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-client.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-dbus.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-session-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-stream-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-properties.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-channel.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-constants.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-misc.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)

set(telepathy_qt4_MOC_SRCS
    abstract-interface.h
    account.h
    account-manager.h
    account-set.h
    account-set-internal.h
    channel.h
    channel-dispatch-operation.h
    channel-request.h
    client-registrar.h
    client-registrar-internal.h
    connection.h
    connection-internal.h
    connection-manager.h
    connection-manager-internal.h
    contact.h
    contact-manager.h
    contact-manager-internal.h
    dbus-proxy.h
    dbus-proxy-factory-internal.h
    file-transfer-channel.h
    incoming-file-transfer-channel.h
    outgoing-file-transfer-channel.h
    pending-account.h
    pending-channel.h
    pending-channel-request.h
    pending-channel-request-internal.h
    pending-connection.h
    pending-contact-attributes.h
    pending-contact-info.h
    pending-contacts.h
    pending-handles.h
    pending-operation.h
    pending-ready.h
    pending-string-list.h
    pending-variant.h
    readiness-helper.h
    room-list-channel.h
    simple-pending-operations.h
    streamed-media-channel.h
    streamed-media-channel-internal.h
    text-channel.h)

set(gen_stable_spec_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml)
set(gen_future_spec_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml)

tpqt4_xincludator(${CMAKE_CURRENT_SOURCE_DIR}/stable-interfaces.xml ${gen_stable_spec_xml})
tpqt4_xincludator(${CMAKE_CURRENT_SOURCE_DIR}/future-interfaces.xml ${gen_future_spec_xml})

tpqt4_constants_gen(${gen_stable_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
                    --namespace=Tp
                    --str-constant-prefix=TELEPATHY_
                    --must-define=IN_TELEPATHY_QT4_HEADER)
tpqt4_constants_gen(${gen_future_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-constants.h
                    --namespace=TpFuture
                    --str-constant-prefix=TP_FUTURE_)

tpqt4_types_gen(${gen_stable_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp
                Tp TelepathyQt4/types.h TelepathyQt4/Types
                --must-define=IN_TELEPATHY_QT4_HEADER
                --visibility=TELEPATHY_QT4_EXPORT)
tpqt4_types_gen(${gen_future_spec_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-constants.h
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types-body.hpp
                TpFuture TelepathyQt4/future-internal.h TelepathyQt4/future-internal.h)

list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)
list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp)
list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h)
list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types-body.hpp)

set(SPECS
    account
    account-manager
    channel
    channel-dispatcher
    channel-dispatch-operation
    channel-request
    client
    connection
    connection-manager
    dbus
    media-session-handler
    media-stream-handler
    properties)
foreach(spec ${SPECS})
    tpqt4_xincludator(${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
                      ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
    list(APPEND telepathy_qt4_SRCS ${NEW_FILES})
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach(spec ${SPECS})

set(FUTURE_SPECS
    channel
    misc)
foreach(spec ${FUTURE_SPECS})
    tpqt4_xincludator(${CMAKE_CURRENT_SOURCE_DIR}/future-${spec}.xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml
                      ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h
                      ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-types.h)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.moc.hpp)
    list(APPEND telepathy_qt4_SRCS ${NEW_FILES})
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach(spec ${FUTURE_SPECS})

if(MSVC)
    set(TYPES_INCLUDE ^<TelepathyQt4/Types^> )
else(MSVC)
    set(TYPES_INCLUDE '<TelepathyQt4/Types>' )
endif(MSVC)

tpqt4_client_generator(account clientaccount AccountManager Tp::Client --mainiface=Tp::Client::AccountInterface)
tpqt4_client_generator(account-manager clientam AccountManager Tp::Client --mainiface=Tp::Client::AccountManagerInterface)
tpqt4_client_generator(channel clientchannel Channel Tp::Client --mainiface=Tp::Client::ChannelInterface)
tpqt4_client_generator(channel-dispatcher clientchanneldispatcher ChannelDispatcher Tp::Client --mainiface=Tp::Client::ChannelDispatcherInterface)
tpqt4_client_generator(channel-dispatch-operation clientchanneldispatchoperation ChannelDispatchOperation Tp::Client --mainiface=Tp::Client::ChannelDispatchOperationInterface)
tpqt4_client_generator(channel-request clientchannelrequest ChannelRequest Tp::Client --mainiface=Tp::Client::ChannelRequestInterface)
tpqt4_client_generator(client clientclient Client Tp::Client --mainiface=Tp::Client::ClientInterface)
tpqt4_client_generator(connection clientconn Connection Tp::Client --mainiface=Tp::Client::ConnectionInterface)
tpqt4_client_generator(connection-manager clientcm ConnectionManager Tp::Client --mainiface=Tp::Client::ConnectionManagerInterface)
tpqt4_client_generator(dbus clientdbus DBus Tp::Client::DBus "")
tpqt4_client_generator(media-session-handler clientmsesh MediaSessionHandler Tp::Client --mainiface=Tp::Client::MediaSessionHandlerInterface)
tpqt4_client_generator(media-stream-handler clientmstrh MediaStreamHandler Tp::Client --mainiface=Tp::Client::MediaStreamHandlerInterface)
tpqt4_client_generator(properties clientprops Properties Tp::Client "")

tpqt4_future_client_generator(channel TpFuture::Client --mainiface=Tp::Client::ChannelInterface)
tpqt4_future_client_generator(misc TpFuture::Client '')

foreach(spec ${SPECS})
    list(APPEND telepathy_qt4_generated_specs_mocs "moc-cli-${spec}.moc.hpp")
endforeach(spec ${SPECS})
foreach(spec ${FUTURE_SPECS})
    list(APPEND telepathy_qt4_generated_specs_mocs "moc-future-${spec}.moc.hpp")
endforeach(spec ${FUTURE_SPECS})

add_library(telepathy-qt4 SHARED ${telepathy_qt4_SRCS})

# generate client moc files
foreach(moc_src ${telepathy_qt4_MOC_SRCS})
    set(generated_file _gen/${moc_src})
    string(REPLACE ".h" ".moc.hpp" generated_file ${generated_file})
    qt4_generate_moc_i_target_deps(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/${generated_file}
                                   ${telepathy_qt4_generated_specs_mocs})
    list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
    string(REPLACE ".h" ".moc.hpp" moc_src ${moc_src})
    add_dependencies(telepathy-qt4 "moc-${moc_src}")
endforeach(moc_src ${telepathy_qt4_MOC_SRCS})

target_link_libraries(telepathy-qt4
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTNETWORK_LIBRARIES})

install(FILES ${telepathy_qt4_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4 COMPONENT headers)
install(FILES ${telepathy_qt4_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4/_gen COMPONENT headers)

if (WIN32)
    install(TARGETS telepathy-qt4 RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
                               ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT headers)
else (WIN32)
    install(TARGETS telepathy-qt4 LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT mainlibrary
                               ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT headers)
endif (WIN32)

# pkg-config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4-uninstalled.pc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT headers)

if(FARSIGHT_COMPONENTS_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4Farsight.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4Farsight.pc)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4Farsight-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4Farsight-uninstalled.pc)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4Farsight.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT farsight_headers)
endif(FARSIGHT_COMPONENTS_FOUND)

add_subdirectory(Farsight)
