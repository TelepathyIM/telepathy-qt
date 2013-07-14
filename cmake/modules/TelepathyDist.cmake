# setup make dist
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
                   COMMAND git archive --format=tar --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD |
                           gzip > ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(create-source-working-dir
                  rm -rf ${PACKAGE_NAME}-${PACKAGE_VERSION} &&
                  gzip -df ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz &&
                  tar -xf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar &&
                  rm ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar* &&
                  cd ${PACKAGE_NAME}-${PACKAGE_VERSION}/ &&
                  rm -rf doc && mkdir doc && cp -R ${CMAKE_BINARY_DIR}/doc/html doc/

                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  DEPENDS ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
                  COMMENT "Generating working source dir for the dist tarball")
add_dependencies(create-source-working-dir doxygen-doc)

add_custom_target(dist-hook
                  chmod u+w ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}/ChangeLog &&
                  git log --stat > ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}/ChangeLog ||
                  git log > ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}/ChangeLog

                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  COMMENT "Updating Changelog")
add_dependencies(dist-hook create-source-working-dir)

add_custom_target(dist tar --format=ustar -chf - ${PACKAGE_NAME}-${PACKAGE_VERSION} |
                       GZIP=--best gzip -c > ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  COMMENT "Generating dist tarball")
add_dependencies(dist dist-hook)

# setup make distcheck
add_custom_target(distcheck rm -rf build && mkdir build && cd build && cmake .. && make && make check
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}/
                  COMMENT "Testing successful tarball build")
add_dependencies(distcheck dist)

# CPack
set(ENABLE_CPACK OFF CACHE BOOL "Enables CPack targets generation")
if (ENABLE_CPACK)

    include(InstallRequiredSystemLibraries)

    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A high-level binding for Telepathy in Qt")
    SET(CPACK_PACKAGE_VENDOR "Collabora Ltd.")
    SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
    SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
    SET(CPACK_PACKAGE_VERSION_MAJOR ${TP_QT_MAJOR_VERSION})
    SET(CPACK_PACKAGE_VERSION_MINOR ${TP_QT_MINOR_VERSION})
    SET(CPACK_PACKAGE_VERSION_PATCH ${TP_QT_MICRO_VERSION})
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "TelepathyQt")
    SET(CPACK_PACKAGE_CONTACT "telepathy@lists.freedesktop.org")
    set(CPACK_SOURCE_IGNORE_FILES
    "/build/;/.bzr/;~$;/.git/;/.kdev4/;${CPACK_SOURCE_IGNORE_FILES}")
    IF(WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    #SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
    #SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} TelepathyQt")
    #SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.github.com")
    #SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
    #SET(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
    SET(CPACK_NSIS_MODIFY_PATH ON)
    ELSE(WIN32 AND NOT UNIX)
    #SET(CPACK_STRIP_FILES "bin/MyExecutable")
    SET(CPACK_SOURCE_STRIP_FILES "")
    ENDIF(WIN32 AND NOT UNIX)
    #SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

    if (APPLE)
        set(CPACK_SET_DESTDIR ON)
        set(CPACK_PACKAGE_RELOCATABLE OFF)
    endif (APPLE)

    #name components
    set(CPACK_COMPONENT_MAINLIBRARY_DISPLAY_NAME "TelepathyQt main components")
    set(CPACK_COMPONENT_FARSTREAM_DISPLAY_NAME "TelepathyQt Farstream support")
    set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development files for TelepathyQt")
    set(CPACK_COMPONENT_FARSTREAM_HEADERS_DISPLAY_NAME "Development files for TelepathyQt-Farstream")

    #components description
    set(CPACK_COMPONENT_MAINLIBRARY_DESCRIPTION
    "The main TelepathyQt library")
    set(CPACK_COMPONENT_FARSTREAM_DESCRIPTION
    "The TelepathyQt-Farstream library")
    set(CPACK_COMPONENT_HEADERS_DESCRIPTION
    "Development files for TelepathyQt")
    set(CPACK_COMPONENT_FARSTREAM_HEADERS_DESCRIPTION
    "Development files for TelepathyQt-Farstream")

    set(CPACK_COMPONENT_HEADERS_DEPENDS mainlibrary)
    set(CPACK_COMPONENT_FARSTREAM_DEPENDS mainlibrary)
    set(CPACK_COMPONENT_FARSTREAM_HEADERS_DEPENDS mainlibrary farstream)

    #installation types
    set(CPACK_ALL_INSTALL_TYPES User Developer Minimal)

    set(CPACK_COMPONENT_MAINLIBRARY_INSTALL_TYPES User Developer Minimal)
    set(CPACK_COMPONENT_FARSTREAM_INSTALL_TYPES User Developer)
    set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer)
    set(CPACK_COMPONENT_FARSTREAM_HEADERS_INSTALL_TYPES Developer)

    # Leave this as the last declaration, always!!!
    include(CPack)

endif (ENABLE_CPACK)
