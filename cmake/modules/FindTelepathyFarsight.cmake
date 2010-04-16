# - Try to find Telepathy-Farsight
# Once done this will define
#
#  TELEPATHY_FARSIGHT_FOUND - system has TelepathyFarsight
#  TELEPATHY_FARSIGHT_INCLUDE_DIR - the TelepathyFarsight include directory
#  TELEPATHY_FARSIGHT_LIBRARIES - the libraries needed to use TelepathyFarsight
#  TELEPATHY_FARSIGHT_DEFINITIONS - Compiler switches required for using TelepathyFarsight

# Copyright (c) 2010, Dario Freddi <dario.freddi@collabora.co.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (TELEPATHY_FARSIGHT_INCLUDE_DIR AND TELEPATHY_FARSIGHT_LIBRARIES)
   # in cache already
   set(TelepathyFarsight_FIND_QUIETLY TRUE)
else (TELEPATHY_FARSIGHT_INCLUDE_DIR AND TELEPATHY_FARSIGHT_LIBRARIES)
   set(TelepathyFarsight_FIND_QUIETLY FALSE)
endif (TELEPATHY_FARSIGHT_INCLUDE_DIR AND TELEPATHY_FARSIGHT_LIBRARIES)

if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the find_path() and find_library() calls
    find_package(PkgConfig)
    if (TELEPATHY_FARSIGHT_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_FARSIGHT telepathy-farsight>=${TELEPATHY_FARSIGHT_MIN_VERSION})
    else (TELEPATHY_FARSIGHT_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_FARSIGHT telepathy-farsight)
    endif (TELEPATHY_FARSIGHT_MIN_VERSION)
    set(TELEPATHY_FARSIGHT_DEFINITIONS ${PC_TELEPATHY_FARSIGHT_CFLAGS_OTHER})
endif (NOT WIN32)

find_path(TELEPATHY_FARSIGHT_INCLUDE_DIR telepathy-farsight/channel.h
   PATHS
   ${PC_TELEPATHY_FARSIGHT_INCLUDEDIR}
   ${PC_TELEPATHY_FARSIGHT_INCLUDE_DIRS}
   PATH_SUFFIXES telepathy-1.0
   )

find_library(TELEPATHY_FARSIGHT_LIBRARIES NAMES telepathy-farsight
   PATHS
   ${PC_TELEPATHY_FARSIGHT_LIBDIR}
   ${PC_TELEPATHY_FARSIGHT_LIBRARY_DIRS}
   )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TelepathyFarsight DEFAULT_MSG TELEPATHY_FARSIGHT_LIBRARIES
                                                                TELEPATHY_FARSIGHT_INCLUDE_DIR)

mark_as_advanced(TELEPATHY_FARSIGHT_INCLUDE_DIR TELEPATHY_FARSIGHT_LIBRARIES)
