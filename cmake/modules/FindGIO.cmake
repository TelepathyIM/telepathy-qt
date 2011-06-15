# - Try to find the GIO libraries
# Once done this will define
#
#  GIO_FOUND - system has GIO
#  GIO_INCLUDE_DIR - the GIO include directory
#  GIO_LIBRARIES - GIO library
#
# Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright (C) 2011 Nokia Corporation
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(GIO_INCLUDE_DIR AND GIO_LIBRARIES)
    # Already in cache, be silent
    set(GIO_FIND_QUIETLY TRUE)
endif(GIO_INCLUDE_DIR AND GIO_LIBRARIES)

include(UsePkgConfig)
pkg_check_modules(PC_LibGIO gio-2.0)

find_path(GIO_MAIN_INCLUDE_DIR
          NAMES gio/gio.h
          HINTS ${PC_LibGIO_INCLUDEDIR}
          PATH_SUFFIXES glib-2.0)

find_library(GIO_LIBRARY
             NAMES gio-2.0
             HINTS ${PC_LibGIO_LIBDIR})

set(GIO_INCLUDE_DIR "${GIO_MAIN_INCLUDE_DIR}")
set(GIO_LIBRARIES "${GIO_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GIO DEFAULT_MSG GIO_LIBRARIES GIO_MAIN_INCLUDE_DIR)

mark_as_advanced(GIO_INCLUDE_DIR GIO_LIBRARIES)
