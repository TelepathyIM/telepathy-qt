project(TelepathyQt)

cmake_minimum_required(VERSION 2.8.12)

# Making releases:
#   set the new version number:
#     odd minor -> development series
#     even minor -> stable series
#     increment micro for each release within a series
#   set nano_version to 0
#   make the release, tag it
#   set nano_version to 1
set(TP_QT_MAJOR_VERSION 0)
set(TP_QT_MINOR_VERSION 9)
set(TP_QT_MICRO_VERSION 7)
set(TP_QT_NANO_VERSION  0)

set(PACKAGE_NAME telepathy-qt)

if (${TP_QT_NANO_VERSION} EQUAL 0)
    set(PACKAGE_VERSION ${TP_QT_MAJOR_VERSION}.${TP_QT_MINOR_VERSION}.${TP_QT_MICRO_VERSION})
else (${TP_QT_NANO_VERSION} EQUAL 0)
    set(PACKAGE_VERSION ${TP_QT_MAJOR_VERSION}.${TP_QT_MINOR_VERSION}.${TP_QT_MICRO_VERSION}.${TP_QT_NANO_VERSION})
endif (${TP_QT_NANO_VERSION} EQUAL 0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Default build type is RelWithDebInfo for release versions and Debug for developement
# versions
if(NOT CMAKE_BUILD_TYPE)
    if(TP_QT_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
    else(TP_QT_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE Debug)
    endif(TP_QT_NANO_VERSION EQUAL 0)
endif(NOT CMAKE_BUILD_TYPE)

# This file contains all the needed initialization macros
include(TelepathyDefaults)

# This file contains all the tpqt macros used in the buildsystem
include(TpQtMacros)

include(MacroLogFeature)

# external dependencies

# Required dependencies
# Find qt4 version >= 4.6 or qt5 >= 5.0.0
set(QT4_MIN_VERSION "4.6.0")
set(QT4_MAX_VERSION "5.0.0")
set(QT5_MIN_VERSION "5.0.0")
set(QT5_MAX_VERSION "6.0.0")
find_package(Qt REQUIRED)

IF(QT_VERSION_MAJOR MATCHES 5)
    # It is QtCore for Qt4 but Qt5Core for Qt5 in pkg-config
    SET(QT_VERSION_PC 5)
ENDIF(QT_VERSION_MAJOR MATCHES 5)

foreach(flag ${QT_CONFIG_FLAGS})
    if (${flag} MATCHES "reduce_relocations")
        set(TP_QT_EXECUTABLE_LINKER_FLAGS "-fPIE")
        set(TP_QT_LIBRARY_LINKER_FLAGS "-fPIC")
    endif (${flag} MATCHES "reduce_relocations")
endforeach(flag ${QT_CONFIG_FLAGS})

# This value contains the library's SOVERSION. This value is to be increased everytime an API/ABI break
# occurs, and will be used for the SOVERSION of the generated shared libraries.
if (${QT_VERSION_MAJOR} EQUAL 4)
    set(TP_QT_ABI_VERSION 2)
else (${QT_VERSION_MAJOR} EQUAL 4)
    set(TP_QT_ABI_VERSION 0)
endif (${QT_VERSION_MAJOR} EQUAL 4)

set(TP_QT_SERVICE_ABI_VERSION 0)

# This variable is used for the library's long version. It is generated dynamically, so don't change its
# value! Change TP_QT_ABI_VERSION and TP_QT_*_VERSION instead.
if (${TP_QT_NANO_VERSION} EQUAL 0)
    set(TP_QT_LIBRARY_VERSION ${TP_QT_ABI_VERSION}.${TP_QT_MAJOR_VERSION}.${TP_QT_MINOR_VERSION}.${TP_QT_MICRO_VERSION})
else (${TP_QT_NANO_VERSION} EQUAL 0)
    set(TP_QT_LIBRARY_VERSION ${TP_QT_ABI_VERSION}.${TP_QT_MAJOR_VERSION}.${TP_QT_MINOR_VERSION}.${TP_QT_MICRO_VERSION}.${TP_QT_NANO_VERSION})
endif (${TP_QT_NANO_VERSION} EQUAL 0)

# Add an option for compiling tp-qt-service
option(ENABLE_SERVICE_SUPPORT "Enable compilation of service side bindings for Telepathy-Qt" TRUE)
# Add an option to select type of service library
option(FORCE_STATIC_SERVICE_LIBRARY "Force Telepathy-Qt service to be a static library." FALSE)
# Add an option for compiling examples
option(ENABLE_EXAMPLES "Enable compilation of examples for Telepathy-Qt" TRUE)
# Add an option for compiling Farstream
option(ENABLE_FARSTREAM "Enable compilation of Farstream bindings" TRUE)
# Add an option for building tests
option(ENABLE_TESTS "Enable compilation of automated tests" TRUE)

# The doxygen macro requires Qt to have been looked up to enable crosslinking
include(Doxygen)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${QT_INCLUDES})

add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(ENABLE_DEBUG_OUTPUT ON CACHE BOOL "If activated, compiles support for printing debug output to stderr")
if (ENABLE_DEBUG_OUTPUT)
    add_definitions(-DENABLE_DEBUG)
endif (ENABLE_DEBUG_OUTPUT)

# Check for Qt Glib support
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
set(CMAKE_REQUIRED_DEFINITIONS "")
# set the flags (-fPIE) here so that CHECK_CXX_SOURCE_COMPILES works properly if qt5 was built with
# -reduce-locations
set(CMAKE_REQUIRED_FLAGS "${TP_QT_EXECUTABLE_LINKER_FLAGS}")

CHECK_CXX_SOURCE_COMPILES("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT_GLIB_SUPPORT)
macro_log_feature(QT_GLIB_SUPPORT "Qt Glib Support"
                  "QtCore library using Glib's main event loop"
                  "http://qt.nokia.com/" FALSE ""
                  "Needed, together with Telepathy-Glib, to build most of the unit tests")

# reset flags
set(CMAKE_REQUIRED_FLAGS "")

# Find python version >= 2.7
set(REQUIRED_PY 2.7)
find_package(PythonLibrary ${REQUIRED_PY} REQUIRED)
if(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})
    message(STATUS "Python ${PYTHON_SHORT_VERSION} found")
else(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})
    message(SEND_ERROR "Python >= ${REQUIRED_PY} is required")
endif(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})

# Check for dbus-python
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import dbus.mainloop.glib"
                RESULT_VARIABLE PYTHON_DBUS_RESULT)
if(PYTHON_DBUS_RESULT EQUAL 0)
    set(HAVE_TEST_PYTHON 1)
else(PYTHON_DBUS_RESULT EQUAL 0)
    set(HAVE_TEST_PYTHON 0)
endif(PYTHON_DBUS_RESULT EQUAL 0)
macro_log_feature(HAVE_TEST_PYTHON "dbus-python"
                  "GLib-based Python DBus support"
                  "http://www.python.org/" FALSE ""
                  "Needed to build some additional unit tests")

if(ENABLE_TESTS OR ENABLE_FARSTREAM)
    # Find telepathy-glib
    set(TELEPATHY_GLIB_MIN_VERSION 0.18.0)
    find_package(TelepathyGlib)
    macro_log_feature(TELEPATHYGLIB_FOUND "Telepathy-glib"
                      "Glib bindings for Telepathy"
                      "http://telepathy.freedesktop.org/" FALSE ${TELEPATHY_GLIB_MIN_VERSION}
                      "Needed, together with Qt Glib integration, to build most of the unit tests")

    find_program(GLIB_GENMARSHAL glib-genmarshal)

    # Find GLib2, GObject, DBus and LibXml2
    # Those are needed for the insane include dir dependency hell
    find_package(GLIB2)
    find_package(GObject)
    find_package(GIO)
    find_package(GIOUnix)
    find_package(DBus)
    find_package(DBusGLib)
    find_package(LibXml2)
endif()

if(ENABLE_FARSTREAM)
    # Find tp-farstream
    set(FARSTREAM_MIN_VERSION "0.2.0")
    find_package(Farstream)
    macro_log_feature(FARSTREAM_FOUND "Farstream"
                      "A Framework for dealing with audio/video conferencing protocols"
                      "http://www.freedesktop.org/wiki/Software/Farstream" FALSE "${FARSTREAM_MIN_VERSION}"
                      "Needed, together with GStreamer and Telepathy-Farstream, to build telepathy-qt-farstream")

    # Find tp-farstream
    set(TELEPATHY_FARSTREAM_MIN_VERSION "0.6.0")
    find_package(TelepathyFarstream)
    macro_log_feature(TELEPATHYFARSTREAM_FOUND "Telepathy-Farstream"
                      "A Framework for dealing with audio/video conferencing protocols"
                      "http://telepathy.freedesktop.org/wiki/" FALSE "${TELEPATHY_FARSTREAM_MIN_VERSION}"
                      "Needed, together with GStreamer and Farstream, to build telepathy-qt-farstream")
endif()

if(ENABLE_FARSTREAM)
    # Find GStreamer
    find_package(GStreamer)
    macro_log_feature(GSTREAMER_FOUND "GStreamer"
                      "An open source multimedia framework"
                      "Needed, together with Tp-Farstream, to build telepathy-qt-farstream and some additional examples"
                      "http://www.gstreamer.net/" FALSE)
endif()

# Build TelepathyQt-Farstream only if GStreamer, TelepathyFarstream and all of their dependencies were found
if (TELEPATHYFARSTREAM_FOUND AND FARSTREAM_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND AND TELEPATHYGLIB_FOUND AND ENABLE_FARSTREAM)
    set (FARSTREAM_COMPONENTS_FOUND 1)
else (TELEPATHYFARSTREAM_FOUND AND FARSTREAM_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND AND TELEPATHYGLIB_FOUND AND ENABLE_FARSTREAM)
    set (FARSTREAM_COMPONENTS_FOUND 0)
endif (TELEPATHYFARSTREAM_FOUND AND FARSTREAM_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND AND TELEPATHYGLIB_FOUND AND ENABLE_FARSTREAM)

if (ENABLE_TESTS)
    # Enable glib-based tests only if Qt has GLib support and Telepathy-glib was found
    if(QT_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
        # Disable GLib deprecation warnings for now; GValueArray is deprecated but we
        # need it for telepathy-glib.
        add_definitions(-DGLIB_DISABLE_DEPRECATION_WARNINGS)
        set(ENABLE_TP_GLIB_TESTS 1)
        if(GIO_FOUND AND GIOUNIX_FOUND)
            set(ENABLE_TP_GLIB_GIO_TESTS 1)
        else(GIO_FOUND AND GIOUNIX_FOUND)
            set(ENABLE_TP_GLIB_GIO_TESTS 0)
        endif(GIO_FOUND AND GIOUNIX_FOUND)
    else(QT_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
        set(ENABLE_TP_GLIB_TESTS 0)
        set(ENABLE_TP_GLIB_GIO_TESTS 0)
    endif(QT_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
endif(ENABLE_TESTS)

# Add the source subdirectories
add_subdirectory(TelepathyQt)
if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
add_subdirectory(tools)

# Generate config.h and config-version.h
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
configure_file(${CMAKE_SOURCE_DIR}/config-version.h.in ${CMAKE_BINARY_DIR}/config-version.h)

# Create the uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Display the feature log
macro_display_feature_log()

# Always keep it last: CPack definitions file
include(TelepathyDist)
