cmake_minimum_required(VERSION 3.5)

if(${CMAKE_VERSION} VERSION_LESS 3.18)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.18)
endif()

project(TelepathyQt
    VERSION 0.9.8
    LANGUAGES C CXX
)

set(PACKAGE_NAME telepathy-qt)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Default build type is RelWithDebInfo for release versions and Debug for developement
# versions
if(NOT CMAKE_BUILD_TYPE)
    if(NOT TelepathyQt_VERSION_TWEAK)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
    else()
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# This file contains all the needed initialization macros
include(TelepathyDefaults)

# This file contains all the tpqt macros used in the buildsystem
include(QtMacros)
include(TpQtMacros)

include(FeatureSummary)

# external dependencies

# Required dependencies
set(QT_VERSION_MAJOR 5)
SET(QT_VERSION_PC 5)
set(QT_COMPONENTS Core DBus Network Xml)

if(ENABLE_TESTS)
    list(APPEND QT_COMPONENTS Test)
endif()

set(QT_MIN_VERSION "5.6.0")
set(QT_MAX_VERSION "6.0.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS ${QT_COMPONENTS})

set(TP_QT_EXECUTABLE_LINKER_FLAGS "-fPIE")
set(TP_QT_LIBRARY_LINKER_FLAGS "-fPIC")

# This value contains the library's SOVERSION. This value is to be increased everytime an API/ABI break
# occurs, and will be used for the SOVERSION of the generated shared libraries.
if (${QT_VERSION_MAJOR} EQUAL 4)
    set(TP_QT_ABI_VERSION 2)
else ()
    set(TP_QT_ABI_VERSION 0)
endif ()

set(TP_QT_SERVICE_ABI_VERSION 1)

# This variable is used for the library's long version. It is generated dynamically, so don't change its
# value! Change TP_QT_ABI_VERSION and TP_QT_*_VERSION instead.
set(TP_QT_LIBRARY_VERSION ${TP_QT_ABI_VERSION}.${TelepathyQt_VERSION})

# The doxygen macro requires Qt to have been looked up to enable crosslinking
include(Doxygen)

add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(ENABLE_DEBUG_OUTPUT ON CACHE BOOL "If activated, compiles support for printing debug output to stderr")
if (ENABLE_DEBUG_OUTPUT)
    add_definitions(-DENABLE_DEBUG)
endif ()

# Check for Qt Glib support
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES "")
set(CMAKE_REQUIRED_DEFINITIONS "")
# set the flags (-fPIE) here so that CHECK_CXX_SOURCE_COMPILES works properly if qt5 was built with
# -reduce-locations
set(CMAKE_REQUIRED_FLAGS "${TP_QT_EXECUTABLE_LINKER_FLAGS}")
set(CMAKE_REQUIRED_LIBRARIES Qt5::Core)

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
add_feature_info("Qt GLib support" QT_GLIB_SUPPORT
    "QtCore library using Glib's main event loop; needed, together with Telepathy-Glib, to build most of the unit tests")

# reset flags
set(CMAKE_REQUIRED_FLAGS "")

# Find python version >= 3
if(${CMAKE_VERSION} VERSION_LESS 3.15)
    find_package(PythonInterp 3 REQUIRED)
    set_package_properties(PythonInterp PROPERTIES
                           DESCRIPTION "Python Interpreter"
                           URL "http://www.python.org"
                           TYPE REQUIRED)

    # PythonInterp exposes PYTHON_EXECUTABLE on its own
else()
    find_package(Python3 REQUIRED)
    set_package_properties(Python3 PROPERTIES
                           DESCRIPTION "Python Interpreter"
                           URL "http://www.python.org"
                           TYPE REQUIRED)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
endif()

# Check for dbus-python
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import dbus.mainloop.glib"
                RESULT_VARIABLE PYTHON_DBUS_RESULT)
if(PYTHON_DBUS_RESULT EQUAL 0)
    set(HAVE_TEST_PYTHON 1)
else()
    set(HAVE_TEST_PYTHON 0)
endif()
add_feature_info(dbus-python HAVE_TEST_PYTHON
    "GLib-based Python DBus support; needed to build some additional unit tests")

if(ENABLE_TESTS OR ENABLE_FARSTREAM)
    # Find telepathy-glib
    set(TELEPATHY_GLIB_MIN_VERSION 0.18.0)
    find_package(TelepathyGlib ${TELEPATHY_GLIB_MIN_VERSION})
    set_package_properties(TelepathyGlib PROPERTIES
          DESCRIPTION "Glib bindings for Telepathy"
          URL "http://telepathy.freedesktop.org/"
          TYPE RECOMMENDED
          PURPOSE "Needed, together with Qt Glib integration, to build most of the unit tests")

    find_program(GLIB_GENMARSHAL glib-genmarshal)

    # Find GLib2, GObject, DBus and LibXml2
    # Those are needed for the insane include dir dependency hell
    find_package(GLIB2 QUIET)
    find_package(GObject QUIET)
    find_package(GIO QUIET)
    find_package(GIOUnix QUIET)
    find_package(DBus QUIET)
    find_package(DBusGLib QUIET)
    find_package(LibXml2 QUIET)
endif()

if(ENABLE_FARSTREAM)
    # Find tp-farstream
    set(FARSTREAM_MIN_VERSION "0.2.0")
    find_package(Farstream)
    set_package_properties(Farstream PROPERTIES
        DESCRIPTION "A Framework for dealing with audio/video conferencing protocols"
        URL "http://www.freedesktop.org/wiki/Software/Farstream"
        TYPE RECOMMENDED
        PURPOSE "Needed, together with GStreamer and Telepathy-Farstream, to build telepathy-qt-farstream")

    # Find tp-farstream
    set(TELEPATHY_FARSTREAM_MIN_VERSION "0.6.0")
    find_package(TelepathyFarstream)
    set_package_properties(TelepathyFarstream PROPERTIES
        DESCRIPTION "A Framework for dealing with audio/video conferencing protocols"
        URL "http://telepathy.freedesktop.org/wiki/"
        TYPE RECOMMENDED
        PURPOSE "Needed, together with GStreamer and Farstream, to build telepathy-qt-farstream")
endif()

if(ENABLE_FARSTREAM)
    # Find GStreamer
    find_package(GStreamer)
    set_package_properties(GStreamer PROPERTIES
        DESCRIPTION "An open source multimedia framework"
        URL "http://www.gstreamer.net/"
        TYPE RECOMMENDED
        PURPOSE "Needed, together with Tp-Farstream, to build telepathy-qt-farstream and some additional examples")
endif()

# Build TelepathyQt-Farstream only if GStreamer, TelepathyFarstream and all of their dependencies were found
if (TELEPATHYFARSTREAM_FOUND AND FARSTREAM_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND AND TELEPATHYGLIB_FOUND AND ENABLE_FARSTREAM)
    set (FARSTREAM_COMPONENTS_FOUND 1)
else ()
    set (FARSTREAM_COMPONENTS_FOUND 0)
endif ()

if (ENABLE_TESTS)
    # Enable glib-based tests only if Qt has GLib support and Telepathy-glib was found
    if(QT_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
        # Disable GLib deprecation warnings for now; GValueArray is deprecated but we
        # need it for telepathy-glib.
        add_definitions(-DGLIB_DISABLE_DEPRECATION_WARNINGS)
        set(ENABLE_TP_GLIB_TESTS 1)
        if(GIO_FOUND AND GIOUNIX_FOUND)
            set(ENABLE_TP_GLIB_GIO_TESTS 1)
        else()
            set(ENABLE_TP_GLIB_GIO_TESTS 0)
        endif()
    else()
        set(ENABLE_TP_GLIB_TESTS 0)
        set(ENABLE_TP_GLIB_GIO_TESTS 0)
    endif()
endif()

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

#add_custom_target(uninstall
#    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Display the feature log
feature_summary(
    FATAL_ON_MISSING_REQUIRED_PACKAGES
    WHAT ALL)

# Always keep it last: CPack definitions file
include(TelepathyDist)
