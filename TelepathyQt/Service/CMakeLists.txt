if(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

    # It gets inherited from the previous directory, hence it has to be removed explicitely
    remove_definitions(-DBUILDING_TP_QT)
    # We are building Telepathy-Qt-Service
    add_definitions(-DBUILDING_TP_QT_SERVICE)

    set(telepathy_qt_service_SRCS
        base-connection-manager.cpp
        abstract-adaptor.cpp)

    set(telepathy_qt_service_HEADERS
        BaseConnectionManager
        base-connection-manager.h
        AbstractAdaptor
        abstract-adaptor.h
        Global
        global.h)

    # Generated headers which will be installed and exported
    set(telepathy_qt_service_gen_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection-manager.h)

    # Headers file moc will be run on
    set(telepathy_qt_service_MOC_SRCS
        abstract-adaptor.h
        base-connection-manager-internal.h)

    add_custom_target(all-generated-service-sources)

    set(SPECS
        connection-manager)
    foreach(spec ${SPECS})
        set(NEW_FILES
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.h
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.moc.hpp)
        list(APPEND telepathy_qt_service_SRCS ${NEW_FILES})
        list(APPEND telepathy_qt_service_generated_specs_mocs "moc-svc-${spec}.moc.hpp")
        set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)
    endforeach(spec ${SPECS})

    tpqt_service_generator(connection-manager servicecm ConnectionManager Tp::Service)

    if (TARGET doxygen-doc)
        add_dependencies(doxygen-doc all-generated-service-sources)
    endif (TARGET doxygen-doc)

    # Create the library
    #if (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})
    #else (ENABLE_COMPILER_COVERAGE)
    #    add_library(telepathy-qt${QT_VERSION_MAJOR}-service SHARED ${telepathy_qt_service_SRCS})
    #endif (ENABLE_COMPILER_COVERAGE)
    # lets build a static only library until we have a stable API/ABI
    add_library(telepathy-qt${QT_VERSION_MAJOR}-service STATIC ${telepathy_qt_service_SRCS})

    # generate service moc files
    foreach(moc_src ${telepathy_qt_service_MOC_SRCS})
        get_filename_component(_moc_outfile_name "${moc_src}" NAME)
        set(generated_file _gen/${_moc_outfile_name})
        string(REPLACE ".h" ".moc.hpp" generated_file ${generated_file})

        tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/${generated_file}
                                        ${telepathy_qt_service_generated_specs_mocs})
        list(APPEND telepathy_qt_service_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
        add_custom_target(moc-${_moc_outfile_name} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
        add_dependencies(telepathy-qt${QT_VERSION_MAJOR}-service "moc-${_moc_outfile_name}")
    endforeach(moc_src ${telepathy_qt_service_MOC_SRCS})

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
    install(FILES ${telepathy_qt_service_gen_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-qt${QT_VERSION_MAJOR}/TelepathyQt/Service/_gen
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
