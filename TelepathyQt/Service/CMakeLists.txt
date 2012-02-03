if(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
    
    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TP_QT)
    # We are building Telepathy-Qt-Service
    add_definitions(-DBUILDING_TP_QT_SERVICE -DQT_NO_KEYWORDS)

    set(telepathy_qt_service_SRCS
        stub.cpp)

    set(telepathy_qt_service_HEADERS
        Global
        global.h)

    # Create the library
    #if (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})
    #else (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service SHARED ${telepathy_qt_service_SRCS})
    #endif (ENABLE_COMPILER_COVERAGE)
    # lets build a static only library until we have a stable API/ABI
    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})

    # Link
    target_link_libraries(telepathy-qt${QT_VERSION_MAJOR}-service
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR})

    # Set the correct version number
    set_target_properties(telepathy-qt${QT_VERSION_MAJOR}-service PROPERTIES
                                                 SOVERSION ${TP_QT_ABI_VERSION}
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
    install(FILES ${telepathy_qt_service_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/Service
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

endif(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
