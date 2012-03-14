# - Try to find Telepathy-Farstream
# Once done this will define
#
#  TELEPATHY_FARSTREAM_FOUND - system has TelepathyFarstream
#  TELEPATHY_FARSTREAM_INCLUDE_DIR - the TelepathyFarstream include directory
#  TELEPATHY_FARSTREAM_LIBRARIES - the libraries needed to use TelepathyFarstream
#  TELEPATHY_FARSTREAM_DEFINITIONS - Compiler switches required for using TelepathyFarstream

# Copyright (c) 2010, Dario Freddi <dario.freddi@collabora.co.uk>
# Copyright (c) 2011, Mateu Batle <mateu.batle@collabora.co.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (TELEPATHY_FARSTREAM_INCLUDE_DIR AND TELEPATHY_FARSTREAM_LIBRARIES)
   # in cache already
   set(TelepathyFarstream_FIND_QUIETLY TRUE)
else (TELEPATHY_FARSTREAM_INCLUDE_DIR AND TELEPATHY_FARSTREAM_LIBRARIES)
   set(TelepathyFarstream_FIND_QUIETLY FALSE)
endif (TELEPATHY_FARSTREAM_INCLUDE_DIR AND TELEPATHY_FARSTREAM_LIBRARIES)

if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the find_path() and find_library() calls
    find_package(PkgConfig)
    if (TELEPATHY_FARSTREAM_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_FARSTREAM telepathy-farstream>=${TELEPATHY_FARSTREAM_MIN_VERSION})
    else (TELEPATHY_FARSTREAM_MIN_VERSION)
        PKG_CHECK_MODULES(PC_TELEPATHY_FARSTREAM telepathy-farstream)
    endif (TELEPATHY_FARSTREAM_MIN_VERSION)
    set(TELEPATHY_FARSTREAM_DEFINITIONS ${PC_TELEPATHY_FARSTREAM_CFLAGS_OTHER})
endif (NOT WIN32)

find_path(TELEPATHY_FARSTREAM_INCLUDE_DIR telepathy-farstream/telepathy-farstream.h
   PATHS
   ${PC_TELEPATHY_FARSTREAM_INCLUDEDIR}
   ${PC_TELEPATHY_FARSTREAM_INCLUDE_DIRS}
   PATH_SUFFIXES telepathy-1.0
   )

find_library(TELEPATHY_FARSTREAM_LIBRARIES NAMES telepathy-farstream
   PATHS
   ${PC_TELEPATHY_FARSTREAM_LIBDIR}
   ${PC_TELEPATHY_FARSTREAM_LIBRARY_DIRS}
   )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TelepathyFarstream DEFAULT_MSG TELEPATHY_FARSTREAM_LIBRARIES
                                                                 TELEPATHY_FARSTREAM_INCLUDE_DIR)

mark_as_advanced(TELEPATHY_FARSTREAM_INCLUDE_DIR TELEPATHY_FARSTREAM_LIBRARIES)
