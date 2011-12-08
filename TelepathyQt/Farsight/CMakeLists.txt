if(FARSIGHT_COMPONENTS_FOUND)
    include_directories(${TELEPATHY_FARSIGHT_INCLUDE_DIR}
                        ${GSTREAMER_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${LIBXML2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TP_QT)
    # We are building Telepathy-Qt-Farsight
    add_definitions(-DBUILDING_TP_QT_FARSIGHT -DQT_NO_KEYWORDS)

    set(telepathy_qt_farsight_SRCS
        channel.cpp)

    set(telepathy_qt_farsight_HEADERS
        Channel
        channel.h
        global.h)

    # Create the library
    if (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt${QT_VERSION_MAJOR}-farsight STATIC ${telepathy_qt_farsight_SRCS})
    else (ENABLE_COMPILER_COVERAGE)
        add_library(telepathy-qt${QT_VERSION_MAJOR}-farsight SHARED ${telepathy_qt_farsight_SRCS})
    endif (ENABLE_COMPILER_COVERAGE)
    # Link
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-farsight
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${TELEPATHY_FARSIGHT_LIBRARIES}
        ${GSTREAMER_INTERFACE_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR})

    # Set the correct version number
    set_target_properties(telepathy-qt${QT_VERSION_MAJOR}-farsight PROPERTIES
                                                 SOVERSION ${TP_QT_ABI_VERSION}
                                                 VERSION ${TP_QT_LIBRARY_VERSION})

    # Install the library - watch out for the correct components
    if (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-farsight
                EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
                RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight_libs)
    else (WIN32)
        install(TARGETS telepathy-qt${QT_VERSION_MAJOR}-farsight
                EXPORT  TelepathyQt${QT_VERSION_MAJOR}Targets
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT farsight_libs)
    endif (WIN32)

    # Install headers
    install(FILES ${telepathy_qt_farsight_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/Farsight
            COMPONENT farsight_headers)

    # pkg-config files, only if not on windows
    if (NOT WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtFarsight.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farsight.pc)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQtFarsight-uninstalled.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farsight-uninstalled.pc)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farsight.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig COMPONENT pkgconfig)
    endif (NOT WIN32)

    # Configure the actual Config file
    configure_file(TelepathyQtFarsightConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarsightConfig.cmake" @ONLY)

    # this file is used by to check if the installed version can be used.
    macro_write_basic_cmake_version_file(${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarsightConfigVersion.cmake
                                         ${PACKAGE_VERSION})

    if(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtFarsightConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/cmake/TelepathyQt${QT_VERSION_MAJOR}Farsight)
    else(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)
        set(_TelepathyQtFarsightConfig_INSTALL_DIR ${LIB_INSTALL_DIR}/TelepathyQt${QT_VERSION_MAJOR}Farsight/cmake)
    endif(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarsightConfigVersion.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt${QT_VERSION_MAJOR}FarsightConfig.cmake
            DESTINATION ${_TelepathyQtFarsightConfig_INSTALL_DIR}
            COMPONENT headers)

endif(FARSIGHT_COMPONENTS_FOUND)
