if(FARSTREAM_COMPONENTS_FOUND)
    include_directories(${TELEPATHY_FARSTREAM_INCLUDE_DIR}
                        ${TELEPATHY_GLIB_INCLUDE_DIR}
                        ${FARSTREAM_INCLUDE_DIR}
                        ${GSTREAMER_INCLUDE_DIRS}
                        ${GLIB2_INCLUDE_DIR}
                        ${LIBXML2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TP_QT)
    # We are building Telepathy-Qt-Farstream
    add_definitions(-DBUILDING_TP_QT_FARSTREAM -DQT_NO_KEYWORDS)

    set(telepathy_qt_farstream_SRCS
        channel.cpp)

    set(telepathy_qt_farstream_HEADERS
        Channel
        channel.h
        Global
        global.h
        PendingChannel)

    set(telepathy_qt_farstream_MOC_SRCS
        channel.h)

    # generate client moc files
    tpqt_generate_mocs(${telepathy_qt_farstream_MOC_SRCS})

    # Create the library
    if (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt${QT_VERSION_MAJOR}-farstream STATIC
                    ${telepathy_qt_farstream_SRCS} ${telepathy_qt_farstream_MOC_SRCS})
    else (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt${QT_VERSION_MAJOR}-farstream SHARED
                    ${telepathy_qt_farstream_SRCS} ${telepathy_qt_farstream_MOC_SRCS})
    endif (ENABLE_COMPILER_COVERAGE)

    # Link
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-farstream
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${GOBJECT_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${TELEPATHY_FARSTREAM_LIBRARIES}
        ${TELEPATHY_GLIB_LIBRARIES}
        telepathy-qt${QT_VERSION_MAJOR}
        ${TP_QT_LIBRARY_LINKER_FLAGS})

    if (ENABLE_COMPILER_COVERAGE)
        target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-farstream gcov)
    endif (ENABLE_COMPILER_COVERAGE)

    # Set the correct version number
    set_target_properties(telepathy-qt${QT_VERSION_MAJOR}-farstream PROPERTIES
                                                       SOVERSION ${TP_QT_ABI_VERSION}
                                                       VERSION ${TP_QT_LIBRARY_VERSION})

    # Install the library - watch out for the correct components
    if (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-farstream
                EXPORT TelepathyQt${QT_VERSION_MAJOR}Targets
                RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream_libs)
    else (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-farstream
                EXPORT TelepathyQt${QT_VERSION_MAJOR}Targets
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farstream_libs)
    endif (WIN32)

    # Install headers
    install(FILES ${telepathy_qt_farstream_HEADERS}
            DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/Farstream
            COMPONENT farstream_headers)

    # pkg-config files, only if not on windows
    if (NOT WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtFarstream.pc.in
                       ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farstream.pc)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtFarstream-uninstalled.pc.in
                       ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farstream-uninstalled.pc)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farstream.pc
                DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT pkgconfig)
    endif (NOT WIN32)

    # Configure the actual Config file
    configure_file(TelepathyQtFarstreamConfig.cmake.in
                   "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarstreamConfig.cmake" @ONLY)

    # this file is used by to check if the installed version can be used.
    macro_write_basic_cmake_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarstreamConfigVersion.cmake
                                         ${PACKAGE_VERSION})

    if(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtFarstreamConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR}Farstream)
    else(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtFarstreamConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farstream/cmake)
    endif(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarstreamConfigVersion.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarstreamConfig.cmake
            DESTINATION ${_TelepathyQtFarstreamConfig_INSTALL_DIR}
            COMPONENT headers)

endif(FARSTREAM_COMPONENTS_FOUND)
