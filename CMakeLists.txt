project(TelepathyQt4)

cmake_minimum_required(VERSION 2.6)

# CMake policies are used for backwards compatibilty. Setting a policy to a behavior lets newer
# CMake versions where some behaviors changed behave in a way or another. In our specific case,
# From CMake's documentation:
#
# In CMake 2.6.2 and below, CMake Policy settings in scripts loaded by
# the include() and find_package() commands would affect the includer.
# Explicit invocations of cmake_policy(PUSH) and cmake_policy(POP) were
# required to isolate policy changes and protect the includer.  While
# some scripts intend to affect the policies of their includer, most do
# not.  In CMake 2.6.3 and above, include() and find_package() by
# default PUSH and POP an entry on the policy stack around an included
# script, but provide a NO_POLICY_SCOPE option to disable it.  This
# policy determines whether or not to imply NO_POLICY_SCOPE for
# compatibility.  The OLD behavior for this policy is to imply
# NO_POLICY_SCOPE for include() and find_package() commands.  The NEW
# behavior for this policy is to allow the commands to do their default
# cmake_policy PUSH and POP.
#
# This policy was introduced in CMake version 2.6.3.  CMake version
# 2.8.2 warns when the policy is not set and uses OLD behavior.  Use the
# cmake_policy command to set it to OLD or NEW explicitly.
#
# Whenever our cmake_minimum_required version bumps up to 2.7 or 2.6.3, this policy setting can
# hence be removed.
if(POLICY CMP0011)
    cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

# Making releases:
#   set the new version number:
#     odd minor -> development series
#     even minor -> stable series
#     increment micro for each release within a series
#   set nano_version to 0
#   make the release, tag it
#   set nano_version to 1
set(TP_QT4_MAJOR_VERSION 0)
set(TP_QT4_MINOR_VERSION 3)
set(TP_QT4_MICRO_VERSION 13)
set(TP_QT4_NANO_VERSION  0)

# This value contains the library's SOVERSION. This value is to be increased everytime an API/ABI break
# occurs, and will be used for the SOVERSION of the generated shared libraries.
set(TP_QT4_ABI_VERSION 0)
# This variable is used for the library's long version. It is generated dynamically, so don't change its
# value! Change TP_QT4_ABI_VERSION and TP_QT4_*_VERSION instead.
# NOTE: The generated version here has an additional .99 to differentiate its age from the old autotools version.
#       Drop this as soon as ABI_VERSION is > 0
if (${TP_QT4_NANO_VERSION} EQUAL 0)
    set(TP_QT4_LIBRARY_VERSION ${TP_QT4_ABI_VERSION}.99.${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION})
else (${TP_QT4_NANO_VERSION} EQUAL 0)
    set(TP_QT4_LIBRARY_VERSION ${TP_QT4_ABI_VERSION}.99.${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION}.${TP_QT4_NANO_VERSION})
endif (${TP_QT4_NANO_VERSION} EQUAL 0)

set(PACKAGE_NAME telepathy-qt4)

if (${TP_QT4_NANO_VERSION} EQUAL 0)
    set(PACKAGE_VERSION ${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION})
else (${TP_QT4_NANO_VERSION} EQUAL 0)
    set(PACKAGE_VERSION ${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION}.${TP_QT4_NANO_VERSION})
endif (${TP_QT4_NANO_VERSION} EQUAL 0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Default build type is RelWithDebInfo for release versions and Debug for developement
# versions
if(NOT CMAKE_BUILD_TYPE)
    if(TP_QT4_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
    else(TP_QT4_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE Debug)
    endif(TP_QT4_NANO_VERSION EQUAL 0)
endif(NOT CMAKE_BUILD_TYPE)

# This file contains all the needed initialization macros
include(TelepathyDefaults)

# This file contains all the tpqt4 macros used in the buildsystem
include(TpQt4Macros)

include(Doxygen)
include(MacroLogFeature)

# external dependencies

# Required dependencies
# Find qt4 version >= 4.5
set (QT_MIN_VERSION "4.5.0")
find_package(Qt4 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${QT_INCLUDES})

add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(ENABLE_DEBUG_OUTPUT ON CACHE BOOL "If activated, compiles support for printing debug output to stderr")
if (ENABLE_DEBUG_OUTPUT)
    add_definitions(-DENABLE_DEBUG)
endif (ENABLE_DEBUG_OUTPUT)

# Check for Qt4 Glib support
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
set(CMAKE_REQUIRED_DEFINITIONS "")
set(CMAKE_REQUIRED_FLAGS "")

CHECK_CXX_SOURCE_COMPILES("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT4_GLIB_SUPPORT)
macro_log_feature(QT4_GLIB_SUPPORT "Qt4 Glib Support"
                  "QtCore library using Glib's main event loop"
                  "http://qt.nokia.com/" FALSE ""
                  "Needed, together with Telepathy-Glib, to build most of the unit tests")

# Check for operator== in QDBusVariant
SET(CMAKE_REQUIRED_LIBRARIES ${QT_QTDBUS_LIBRARY})
CHECK_CXX_SOURCE_COMPILES("
#include <QtDBus>
int main()
{
QDBusVariant a; QDBusVariant b; if (a == b) { ; }
return 0;
}"
HAVE_QDBUSVARIANT_OPERATOR_EQUAL)

# Check for QDBusServiceWatcher being present
# This check can be safely removed if we start depending on Qt 4.6
CHECK_CXX_SOURCE_COMPILES("
#include <QtDBus>
int main()
{
QDBusServiceWatcher watcher;
return 0;
}
"
HAVE_QDBUSSERVICEWATCHER)
set(CMAKE_REQUIRED_LIBRARIES)

# Find python version >= 2.5
find_package(PythonLibrary REQUIRED)
set(REQUIRED_PY 2.5)
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

# Find GLib2, GObject, DBus and LibXml2
# Those are needed for the insane include dir dependency hell
find_package(GLIB2)
find_package(GObject)
find_package(DBus)
find_package(LibXml2)

# Find tp-farsight
set(TELEPATHY_FARSIGHT_MIN_VERSION "0.0.4")
find_package(TelepathyFarsight)
macro_log_feature(TELEPATHYFARSIGHT_FOUND "Telepathy-Farsight"
                  "A Framework for dealing with audio/video conferencing protocols"
                  "http://farsight.freedesktop.org/wiki/" FALSE "0.0.4"
                  "Needed, together with GStreamer, to build telepathy-qt4-farsight and some additional examples")

# Find GStreamer
find_package(GStreamer)
macro_log_feature(GSTREAMER_FOUND "GStreamer"
                  "An open source multimedia framework"
                  "Needed, together with Tp-Farsight, to build telepathy-qt4-farsight and some additional examples"
                  "http://www.gstreamer.net/" FALSE)

# Build TelepathyQt4-Farsight only if GStreamer, TelepathyFarsight and all of their dependencies were found
if (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)
    set (FARSIGHT_COMPONENTS_FOUND 1)
else (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)
    set (FARSIGHT_COMPONENTS_FOUND 0)
endif (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)

# Find telepathy-glib
set(TELEPATHY_GLIB_MIN_VERSION 0.11.16)
find_package(TelepathyGlib)
macro_log_feature(TELEPATHYGLIB_FOUND "Telepathy-glib"
                  "Glib bindings for Telepathy"
                  "http://telepathy.freedesktop.org/" FALSE "0.11.16"
                  "Needed, together with Qt Glib integration, to build most of the unit tests")

find_program(GLIB_GENMARSHAL glib-genmarshal)

# Enable glib-based tests only if Qt4 has GLib support and Telepathy-glib was found
if(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
    set(ENABLE_TP_GLIB_TESTS 1)
else(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
    set(ENABLE_TP_GLIB_TESTS 0)
endif(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)

# Add the source subdirectories
add_subdirectory(TelepathyQt4)
add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(tools)

# Generate config.h
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

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
