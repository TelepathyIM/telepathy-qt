# - Try to find Telepathy-Glib
# Once done this will define
#
#  TELEPATHY_GLIB_FOUND - system has Telepathy-Glib
#  TELEPATHY_GLIB_INCLUDE_DIR - the Telepathy-Glib include directory
#  TELEPATHY_GLIB_LIBRARIES - the libraries needed to use Telepathy-Glib
#  TELEPATHY_GLIB_DEFINITIONS - Compiler switches required for using Telepathy-Glib

# Copyright (c) 2010, Dario Freddi <dario.freddi@collabora.co.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (TELEPATHY_GLIB_INCLUDE_DIR AND TELEPATHY_GLIB_LIBRARIES)
   # in cache already
   set(TELEPATHYGLIB_FIND_QUIETLY TRUE)
else (TELEPATHY_GLIB_INCLUDE_DIR AND TELEPATHY_GLIB_LIBRARIES)
   set(TELEPATHYGLIB_FIND_QUIETLY FALSE)
endif (TELEPATHY_GLIB_INCLUDE_DIR AND TELEPATHY_GLIB_LIBRARIES)

if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the find_path() and find_library() calls
    find_package(PkgConfig)
    if (TELEPATHY_GLIB_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_GLIB telepathy-glib>=${TELEPATHY_GLIB_MIN_VERSION})
    else (TELEPATHY_GLIB_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_GLIB telepathy-glib)
    endif (TELEPATHY_GLIB_MIN_VERSION)
    set(TELEPATHY_GLIB_DEFINITIONS ${PC_TELEPATHY_GLIB_CFLAGS_OTHER})
endif (NOT WIN32)

if (TELEPATHY_GLIB_MIN_VERSION AND NOT PC_TELEPATHY_GLIB_FOUND)
    message(STATUS "Telepathy-glib not found or its version is < ${TELEPATHY_GLIB_MIN_VERSION}")
else (TELEPATHY_GLIB_MIN_VERSION AND NOT PC_TELEPATHY_GLIB_FOUND)
    find_path(TELEPATHY_GLIB_INCLUDE_DIR telepathy-glib/client.h
       PATHS
       ${PC_TELEPATHY_GLIB_INCLUDEDIR}
       ${PC_TELEPATHY_GLIB_INCLUDE_DIRS}
       PATH_SUFFIXES telepathy-1.0
    )

    find_library(TELEPATHY_GLIB_LIBRARIES NAMES telepathy-glib
       PATHS
       ${PC_TELEPATHY_GLIB_LIBDIR}
       ${PC_TELEPATHY_GLIB_LIBRARY_DIRS}
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(TelepathyGlib DEFAULT_MSG TELEPATHY_GLIB_LIBRARIES
                                                                TELEPATHY_GLIB_INCLUDE_DIR)

    mark_as_advanced(TELEPATHY_GLIB_INCLUDE_DIR TELEPATHY_GLIB_LIBRARIES)

endif (TELEPATHY_GLIB_MIN_VERSION AND NOT PC_TELEPATHY_GLIB_FOUND)
