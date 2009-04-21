include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${QT_INCLUDES})

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

set(telepathy_qt4_SRCS
    abstract-interface.cpp
    account.cpp
    account-manager.cpp
    channel.cpp
    connection.cpp
    connection-internal.h
    connection-manager.cpp
    connection-manager-internal.h
    contact.cpp
    contact-manager.cpp
    dbus.cpp
    dbus-proxy.cpp
    debug.cpp
    debug-internal.h
    feature.cpp
    file-transfer.cpp
    key-file.cpp
    manager-file.cpp
    media-session-handler.cpp
    media-stream-handler.cpp
    message.cpp
    optional-interface-factory.cpp
    pending-account.cpp
    pending-channel.cpp
    pending-connection.cpp
    pending-contact-attributes.cpp
    pending-contacts.cpp
    pending-handles.cpp
    pending-operation.cpp
    pending-ready.cpp
    pending-string-list.cpp
    properties.cpp
    readiness-helper.cpp
    ready-object.cpp
    referenced-handles.cpp
    room-list.cpp
    streamed-media-channel.cpp
    text-channel.cpp
    types.cpp)

set(telepathy_qt4_HEADERS
    AbstractInterface
    abstract-interface.h
    Account
    account.h
    AccountInterface
    AccountManager
    account-manager.h
    AccountManagerInterface
    Channel
    channel.h
    ChannelInterface
    ChannelInterfaceCallStateInterface
    ChannelInterfaceChatStateInterface
    ChannelInterfaceDTMFInterface
    ChannelInterfaceGroupInterface
    ChannelInterfaceHoldInterface
    ChannelInterfaceMediaSignallingInterface
    ChannelInterfacePasswordInterface
    ChannelTypeContactListInterface
    ChannelTypeRoomListInterface
    ChannelTypeStreamedMediaInterface
    ChannelTypeTextInterface
    ChannelTypeTubesInterface
    Connection
    connection.h
    ConnectionInterface
    ConnectionInterfaceAliasingInterface
    ConnectionInterfaceAvatarsInterface
    ConnectionInterfaceCapabilitiesInterface
    ConnectionInterfaceContactsInterface
    ConnectionInterfacePresenceInterface
    ConnectionInterfaceRequestsInterface
    ConnectionInterfaceSimplePresenceInterface
    ConnectionManager
    connection-manager.h
    ConnectionManagerInterface
    Constants
    constants.h
    Contact
    contact.h
    ContactManager
    contact-manager.h
    DBus
    DBusDaemonInterface
    dbus.h
    DBusProxy
    dbus-proxy.h
    Debug
    debug.h
    Feature
    feature.h
    FileTransfer
    file-transfer.h
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
    OptionalInterfaceFactory
    optional-interface-factory.h
    PeerInterface
    PendingAccount
    pending-account.h
    PendingChannel
    pending-channel.h
    PendingConnection
    pending-connection.h
    PendingContactAttributes
    pending-contact-attributes.h
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
    PendingVoidMethodCall
    Properties
    properties.h
    PropertiesInterface
    PropertiesInterfaceInterface
    ReadinessHelper
    readiness-helper.h
    ReadyObject
    ready-object.h
    ReceivedMessage
    ReferencedHandles
    referenced-handles.h
    ReferencedHandlesIterator
    RoomList
    room-list.h
    SharedData
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
    WeakPtr)

set(telepathy_qt4_gen_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-account-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-channel.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-manager.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-dbus.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-session-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-media-stream-handler.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-properties.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h)

set(telepathy_qt4_MOC_SRCS
    abstract-interface.h
    account.h
    account-manager.h
    channel.h
    connection.h
    connection-internal.h
    connection-manager.h
    connection-manager-internal.h
    contact.h
    contact-manager.h
    dbus-proxy.h
    file-transfer.h
    pending-account.h
    pending-channel.h
    pending-connection.h
    pending-contact-attributes.h
    pending-contacts.h
    pending-handles.h
    pending-operation.h
    pending-ready.h
    pending-string-list.h
    readiness-helper.h
    room-list.h
    simple-pending-operations.h
    streamed-media-channel.h
    text-channel.h)

set(gen_stable_spec_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml)
add_custom_command(OUTPUT ${gen_stable_spec_xml}
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
         ${CMAKE_CURRENT_SOURCE_DIR}/stable-interfaces.xml
         > ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/stable-interfaces.xml
            ${CMAKE_SOURCE_DIR}/tools/xincludator.py)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CMAKE_SOURCE_DIR}/tools/qt4-constants-gen.py
         --namespace=Tp
         --str-constant-prefix=TELEPATHY_
         --must-define=IN_TELEPATHY_QT4_HEADER
         --specxml=${gen_stable_spec_xml}
         > ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
    DEPENDS ${gen_stable_spec_xml}
            ${CMAKE_SOURCE_DIR}/tools/qt4-constants-gen.py)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CMAKE_SOURCE_DIR}/tools/qt4-types-gen.py
         --namespace=Tp
         --declfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h
         --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp
         --realinclude=TelepathyQt4/types.h
         --prettyinclude=TelepathyQt4/Types
         --must-define=IN_TELEPATHY_QT4_HEADER
         --specxml=${gen_stable_spec_xml}
    DEPENDS ${gen_stable_spec_xml}
            ${CMAKE_SOURCE_DIR}/tools/qt4-types-gen.py
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h)
list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp PROPERTIES GENERATED true)

set(SPECS
    account
    account-manager
    channel
    connection
    connection-manager
    dbus
    media-session-handler
    media-stream-handler
    properties)
foreach(spec ${SPECS})
    add_custom_command(OUTPUT "_gen/spec-${spec}.xml"
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
             ${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml
             > ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
        DEPENDS ${spec}.xml
                ${CMAKE_SOURCE_DIR}/tools/xincludator.py)
    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
    list(APPEND telepathy_qt4_SRCS ${NEW_FILES})
    set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
endforeach(spec ${SPECS})

if(MSVC)
    set(TYPES_INCLUDE ^<TelepathyQt4/Types^> )
else(MSVC)
    set(TYPES_INCLUDE '<TelepathyQt4/Types>' )
endif(MSVC)

function(CLIENT_GENERATOR spec group pretty_include namespace main_iface)
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
            --group=${group}
            --namespace=${namespace}
            --typesnamespace=Tp
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
            --realinclude=TelepathyQt4/${spec}.h
            --prettyinclude=TelepathyQt4/${pretty_include}
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
            --extraincludes=${TYPES_INCLUDE}
            --must-define=IN_TELEPATHY_QT4_HEADER
            ${main_iface})
    add_custom_command(OUTPUT _gen/cli-${spec}.h _gen/cli-${spec}-body.hpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        DEPENDS ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    qt4_generate_moc_i(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
    list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
endfunction(CLIENT_GENERATOR spec group pretty_include main_iface)

client_generator(channel clientchannel Channel Tp::Client --mainiface=Tp::Client::ChannelInterface)
client_generator(account clientaccount AccountManager Tp::Client --mainiface=Tp::Client::AccountInterface)
client_generator(account-manager clientam AccountManager Tp::Client --mainiface=Tp::Client::AccountManagerInterface)
client_generator(connection clientconn Connection Tp::Client --mainiface=Tp::Client::ConnectionInterface)
client_generator(connection-manager clientcm ConnectionManager Tp::Client --mainiface=Tp::Client::ConnectionManagerInterface)
client_generator(dbus clientdbus DBus Tp::Client::DBus "")
client_generator(media-session-handler clientmsesh MediaSessionHandler Tp::Client --mainiface=Tp::Client::MediaSessionHandlerInterface)
client_generator(media-stream-handler clientmstrh MediaStreamHandler Tp::Client --mainiface=Tp::Client::MediaStreamHandlerInterface)
client_generator(properties clientprops Properties Tp::Client "")

# generate client moc files
foreach(moc_src ${telepathy_qt4_MOC_SRCS})
    set(generated_file _gen/${moc_src})
    string(REPLACE ".h" ".moc.hpp" generated_file ${generated_file})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
    list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
endforeach(moc_src ${telepathy_qt4_MOC_SRCS})

add_library(telepathy-qt4 STATIC ${telepathy_qt4_SRCS})
target_link_libraries(telepathy-qt4
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTCORE_LIBRARY})

install(FILES ${telepathy_qt4_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4)
install(FILES ${telepathy_qt4_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4/_gen)
install(TARGETS telepathy-qt4 DESTINATION ${LIB_INSTALL_DIR})

# pkg-config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4.pc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

add_subdirectory(Farsight)
