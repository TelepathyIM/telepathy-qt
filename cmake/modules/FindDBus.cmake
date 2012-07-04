# - Try to find the low-level D-Bus library
# Once done this will define
#
#  DBUS_FOUND - system has D-Bus
#  DBUS_INCLUDE_DIRS - the D-Bus include directories
#  DBUS_INCLUDE_DIR - the D-Bus include directory
#  DBUS_ARCH_INCLUDE_DIR - the D-Bus architecture-specific include directory
#  DBUS_LIBRARIES - the libraries needed to use D-Bus

# Copyright (c) 2012, George Kiagiadakis <kiagiadakis.george@gmail.com>
# Copyright (c) 2008, Kevin Kofler, <kevin.kofler@chello.at>
# modeled after FindLibArt.cmake:
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_DBUS dbus-1)
endif (NOT WIN32)

find_path(DBUS_INCLUDE_DIR dbus/dbus.h
    PATHS ${PC_DBUS_INCLUDE_DIRS}
    PATH_SUFFIXES dbus-1.0
)

find_path(DBUS_ARCH_INCLUDE_DIR dbus/dbus-arch-deps.h
    PATHS ${PC_DBUS_INCLUDE_DIRS}
    HINTS ${CMAKE_LIBRARY_PATH}/dbus-1.0/include
          ${CMAKE_SYSTEM_LIBRARY_PATH}/dbus-1.0/include
)

find_library(DBUS_LIBRARIES NAMES dbus-1
    PATHS ${PC_DBUS_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBus DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR DBUS_LIBRARIES)

set(DBUS_INCLUDE_DIRS ${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})

mark_as_advanced(DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR DBUS_LIBRARIES)
