# Try to find the GLib binding of the DBus library
# DBUS_GLIB_FOUND - system has dbus-glib
# DBUS_GLIB_INCLUDE_DIR - the dbus-glib include directory
# DBUS_GLIB_LIBRARIES - Link these to use dbus-glib

# Copyright (c) 2008, Allen Winter <winter@kde.org>
# Copyright (c) 2009, Andre Moreira Magalhaes <andrunko@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

set(DBUS_GLIB_FIND_REQUIRED ${DBusGLib_FIND_REQUIRED})
if(DBUS_GLIB_INCLUDE_DIR AND DBUS_GLIB_LIBRARIES)
  # Already in cache, be silent
  set(DBUS_GLIB_FIND_QUIETLY TRUE)
endif(DBUS_GLIB_INCLUDE_DIR AND DBUS_GLIB_LIBRARIES)

if(NOT WIN32)
    find_package(PkgConfig)
    if (DBusGLib_FIND_VERSION_EXACT)
        pkg_check_modules(PC_DBUS_GLIB QUIET dbus-glib-1=${DBusGLib_FIND_VERSION})
    else (DBusGLib_FIND_VERSION_EXACT)
        if (DBusGLib_FIND_VERSION)
            pkg_check_modules(PC_DBUS_GLIB REQUIRED dbus-glib-1>=${DBusGLib_FIND_VERSION})
        else (DBusGLib_FIND_VERSION)
            pkg_check_modules(PC_DBUS_GLIB REQUIRED dbus-glib-1)
        endif (DBusGLib_FIND_VERSION)
    endif (DBusGLib_FIND_VERSION_EXACT)
endif(NOT WIN32)

find_path(DBUS_GLIB_INCLUDE_DIR
          NAMES dbus-1.0/dbus/dbus-glib.h
          HINTS
          ${PC_DBUS_GLIB_INCLUDEDIR}
          ${PC_DBUS_GLIB_INCLUDE_DIRS}
)

find_path(DBUS_GLIB_LOWLEVEL_INCLUDE_DIR
          NAMES dbus/dbus-arch-deps.h
          HINTS
          ${PC_DBUS_GLIB_INCLUDEDIR}
          ${PC_DBUS_GLIB_INCLUDE_DIRS}
)

# HACK! Workaround appending "/dbus-1.0" to the HINTS above not working for some reason.
set(DBUS_GLIB_INCLUDE_DIRS
    "${DBUS_GLIB_INCLUDE_DIR}/dbus-1.0" "${DBUS_GLIB_LOWLEVEL_INCLUDE_DIR}"
)

find_library(DBUS_GLIB_LIBRARIES
             NAMES dbus-glib-1
             HINTS
             ${PC_DBUS_GLIB_LIBDIR}
             ${PC_DBUS_GLIB_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBUS_GLIB DEFAULT_MSG
                                  DBUS_GLIB_LIBRARIES DBUS_GLIB_INCLUDE_DIR)
