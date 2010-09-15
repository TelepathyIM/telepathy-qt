# setup make dist
add_custom_target(dist cd ${CMAKE_SOURCE_DIR} &&
                        git archive --format=tar --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD |
                            gzip > ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz)

# setup make distcheck
add_custom_target(distcheck cd ${CMAKE_BINARY_DIR} &&
                        rm -rf ${PACKAGE_NAME}-${PACKAGE_VERSION} &&
                        gzip -df ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz &&
                        tar -xf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar &&
                        cd ${PACKAGE_NAME}-${PACKAGE_VERSION}/ &&
                        cmake . && make && make test && make doxygen-doc &&
                        cd ${CMAKE_BINARY_DIR} &&
                        tar -rf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar ${PACKAGE_NAME}-${PACKAGE_VERSION}/doc/ &&
                        gzip ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar)
add_dependencies(distcheck dist)

# CPack

include(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A high-level binding for Telepathy in Qt4")
SET(CPACK_PACKAGE_VENDOR "Collabora Ltd.")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR ${TP_QT4_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${TP_QT4_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${TP_QT4_MICRO_VERSION})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "TelepathyQt4")
SET(CPACK_PACKAGE_CONTACT "telepathy@lists.freedesktop.org")
set(CPACK_SOURCE_IGNORE_FILES
   "/build/;/.bzr/;~$;/.git/;/.kdev4/;${CPACK_SOURCE_IGNORE_FILES}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  #SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  #SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} TelepathyQt4")
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
set(CPACK_COMPONENT_MAINLIBRARY_DISPLAY_NAME "TelepathyQt4 main components")
set(CPACK_COMPONENT_FARSIGHT_DISPLAY_NAME "TelepathyQt4 Farsight support")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Development files for TelepathyQt4")
set(CPACK_COMPONENT_FARSIGHT_HEADERS_DISPLAY_NAME "Development files for TelepathyQt4-Farsight")

#components description
set(CPACK_COMPONENT_MAINLIBRARY_DESCRIPTION
   "The main TelepathyQt4 library")
set(CPACK_COMPONENT_FARSIGHT_DESCRIPTION
   "The TelepathyQt4-Farsight library")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
   "Development files for TelepathyQt4")
set(CPACK_COMPONENT_FARSIGHT_HEADERS_DESCRIPTION
   "Development files for TelepathyQt4-Farsight")

set(CPACK_COMPONENT_HEADERS_DEPENDS mainlibrary)
set(CPACK_COMPONENT_FARSIGHT_DEPENDS mainlibrary)
set(CPACK_COMPONENT_FARSIGHT_HEADERS_DEPENDS mainlibrary farsight)

#installation types
set(CPACK_ALL_INSTALL_TYPES User Developer Minimal)

set(CPACK_COMPONENT_MAINLIBRARY_INSTALL_TYPES User Developer Minimal)
set(CPACK_COMPONENT_FARSIGHT_INSTALL_TYPES User Developer)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer)
set(CPACK_COMPONENT_FARSIGHT_HEADERS_INSTALL_TYPES Developer)

# Leave this as the last declaration, always!!!
include(CPack)
