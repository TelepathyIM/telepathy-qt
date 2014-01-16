# - Try to find Farstream
# Once done this will define
#
#  FARSTREAM_FOUND - system has Farstream
#  FARSTREAM_INCLUDE_DIR - the Farstream include directory
#  FARSTREAM_LIBRARIES - the libraries needed to use Farstream
#  FARSTREAM_DEFINITIONS - Compiler switches required for using Farstream

# Copyright (c) 2010, Dario Freddi <dario.freddi@collabora.co.uk>
# Copyright (c) 2012, George Kiagiadakis <george.kiagiadakis@collabora.co.uk>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (FARSTREAM_INCLUDE_DIR AND FARSTREAM_LIBRARIES)
   # in cache already
   set(Farstream_FIND_QUIETLY TRUE)
else (FARSTREAM_INCLUDE_DIR AND FARSTREAM_LIBRARIES)
   set(Farstream_FIND_QUIETLY FALSE)
endif (FARSTREAM_INCLUDE_DIR AND FARSTREAM_LIBRARIES)

if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the find_path() and find_library() calls
    find_package(PkgConfig)
    if (FARSTREAM_MIN_VERSION)
        PKG_CHECK_MODULES(PC_FARSTREAM farstream-0.2>=${FARSTREAM_MIN_VERSION})
    else (FARSTREAM_MIN_VERSION)
        PKG_CHECK_MODULES(PC_FARSTREAM farstream-0.2)
    endif (FARSTREAM_MIN_VERSION)
    set(FARSTREAM_DEFINITIONS ${PC_FARSTREAM_CFLAGS_OTHER})
endif (NOT WIN32)

find_path(FARSTREAM_INCLUDE_DIR farstream/fs-conference.h
   PATHS
   ${PC_FARSTREAM_INCLUDEDIR}
   ${PC_FARSTREAM_INCLUDE_DIRS}
   PATH_SUFFIXES farstream-0.2
   )

find_library(FARSTREAM_LIBRARIES NAMES farstream-0.2
   PATHS
   ${PC_FARSTREAM_LIBDIR}
   ${PC_FARSTREAM_LIBRARY_DIRS}
   )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Farstream DEFAULT_MSG FARSTREAM_LIBRARIES
                                                        FARSTREAM_INCLUDE_DIR)

mark_as_advanced(FARSTREAM_INCLUDE_DIR FARSTREAM_LIBRARIES)
