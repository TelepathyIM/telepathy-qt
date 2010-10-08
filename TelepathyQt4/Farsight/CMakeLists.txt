if(FARSIGHT_COMPONENTS_FOUND)
    include_directories(${TELEPATHY_FARSIGHT_INCLUDE_DIR}
                        ${GSTREAMER_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${LIBXML2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TELEPATHY_QT4)
    # We are building Telepathy-Qt4-Farsight
    add_definitions(-DBUILDING_TELEPATHY_QT4_FARSIGHT -DQT_NO_KEYWORDS)

    set(telepathy_qt4_farsight_SRCS
        channel.cpp)

    set(telepathy_qt4_farsight_HEADERS
        Channel
        channel.h
        global.h)

    # Create the library
    if (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt4-farsight STATIC ${telepathy_qt4_farsight_SRCS})
    else (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt4-farsight SHARED ${telepathy_qt4_farsight_SRCS})
    endif (ENABLE_COMPILER_COVERAGE)
    # Link
    target_link_libraries(telepathy-qt4-farsight
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${TELEPATHY_FARSIGHT_LIBRARIES}
        ${GSTREAMER_INTERFACE_LIBRARY}
        telepathy-qt4)

    # Set the correct version number
    set_target_properties(telepathy-qt4-farsight PROPERTIES
                                                 SOVERSION ${TP_QT4_ABI_VERSION}
                                                 VERSION ${TP_QT4_LIBRARY_VERSION})

    # Install the library - watch out for the correct components
    if (WIN32)
        install(TARGETS telepathy-qt4-farsight RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight
                                               ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight_headers)
    else (WIN32)
        install(TARGETS telepathy-qt4-farsight LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight
                                               ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight_headers)
    endif (WIN32)

    # Install headers
    install(FILES ${telepathy_qt4_farsight_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4/Farsight
            COMPONENT farsight_headers)
endif(FARSIGHT_COMPONENTS_FOUND)
