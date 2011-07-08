# - Try to find the GIO unix libraries
# Once done this will define
#
#  GIOUNIX_FOUND - system has GIO unix
#  GIOUNIX_INCLUDE_DIR - the GIO unix include directory
#
# Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright (C) 2011 Nokia Corporation
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(GIOUNIX_INCLUDE_DIR)
    # Already in cache, be silent
    set(GIOUNIX_FIND_QUIETLY TRUE)
endif(GIOUNIX_INCLUDE_DIR)

include(UsePkgConfig)
pkg_check_modules(PC_LibGIOUnix gio-unix-2.0)

find_path(GIOUNIX_MAIN_INCLUDE_DIR
          NAMES gio/gunixconnection.h
          HINTS ${PC_LibGIOUnix_INCLUDEDIR}
          PATH_SUFFIXES gio-unix-2.0)

set(GIOUNIX_INCLUDE_DIR "${GIOUNIX_MAIN_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GIOUNIX DEFAULT_MSG GIOUNIX_MAIN_INCLUDE_DIR)

mark_as_advanced(GIOUNIX_INCLUDE_DIR)
