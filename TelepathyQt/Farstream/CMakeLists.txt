if(FARSTREAM_COMPONENTS_FOUND)
    include_directories(${TELEPATHY_FARSTREAM_INCLUDE_DIR}
                        ${TELEPATHY_GLIB_INCLUDE_DIR}
                        ${GSTREAMER_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${LIBXML2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TELEPATHY_QT4_YELL)
    # We are building Telepathy-Qt4-Yell-Farstream
    add_definitions(-DBUILDING_TELEPATHY_QT4_YELL_FARSTREAM -DQT_NO_KEYWORDS)

    set(telepathy_qt4_yell_farstream_SRCS
        channel.cpp)

    set(telepathy_qt4_yell_farstream_HEADERS
        Channel
        channel.h
        Global
        global.h
        PendingTfChannel
        Types
        types.h)

    set(telepathy_qt4_yell_farstream_MOC_SRCS
        channel.h)

    # generate client moc files
    tpqt4_generate_mocs(${telepathy_qt4_yell_farstream_MOC_SRCS})

    # Create the library
    add_library(telepathy-qt4-yell-farstream STATIC
                ${telepathy_qt4_yell_farstream_SRCS} ${telepathy_qt4_yell_farstream_MOC_SRCS})

    # Link
    target_link_libraries(telepathy-qt4-yell-farstream
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${TELEPATHY_FARSTREAM_LIBRARIES}
        ${TELEPATHY_GLIB_LIBRARIES}
        ${GSTREAMER_INTERFACE_LIBRARY}
        telepathy-qt4-yell)

    if (ENABLE_COMPILER_COVERAGE)
        target_link_libraries(telepathy-qt4-yell-farstream gcov)
    endif (ENABLE_COMPILER_COVERAGE)

    # Set the correct version number
    set_target_properties(telepathy-qt4-yell-farstream PROPERTIES
                                                       SOVERSION ${TP_QT_YELL_ABI_VERSION}
                                                       VERSION ${TP_QT_YELL_LIBRARY_VERSION})

    # Install the library - watch out for the correct components
    if (WIN32)
        install(TARGETS telepathy-qt4-yell-farstream RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream
                                                     ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream_headers)
    else (WIN32)
        install(TARGETS telepathy-qt4-yell-farstream LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream
                                                     ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream_headers)
    endif (WIN32)

    # Install headers
    install(FILES ${telepathy_qt4_yell_farstream_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4Yell/Farstream
            COMPONENT farstream_headers)
endif(FARSTREAM_COMPONENTS_FOUND)
