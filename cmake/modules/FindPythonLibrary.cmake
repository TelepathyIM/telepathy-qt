# FindPythonLibrary.cmake
# ~~~~~~~~~~~~~~~~~~~~~~~
# Find the Python interpreter and related Python directories.
#
# This file defines the following variables:
# 
# PYTHON_EXECUTABLE - The path and filename of the Python interpreter.
#
# PYTHON_SHORT_VERSION - The version of the Python interpreter found,
#     excluding the patch version number. (e.g. 2.5 and not 2.5.1))
# 
# PYTHON_LONG_VERSION - The version of the Python interpreter found as a human
#     readable string.
#
# PYTHON_SITE_PACKAGES_DIR - Location of the Python site-packages directory.
#
# PYTHON_INCLUDE_PATH - Directory holding the python.h include file.
#
# PYTHON_LIBRARY, PYTHON_LIBRARIES- Location of the Python library.

# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

INCLUDE(CMakeFindFrameworks)

if(EXISTS PYTHON_LIBRARY)
  # Already in cache, be silent
  set(PYTHONLIBRARY_FOUND TRUE)
else()
  message(STATUS "PythonLibrary find version: ${PythonLibrary_FIND_VERSION}")
  FIND_PACKAGE(PythonInterp ${PythonLibrary_FIND_VERSION} QUIET)

  if(PYTHONINTERP_FOUND)
    FIND_FILE(_find_lib_python_py FindLibPython.py PATHS ${CMAKE_MODULE_PATH})

    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${_find_lib_python_py} OUTPUT_VARIABLE python_config)
    if(python_config)
      STRING(REGEX REPLACE ".*exec_prefix:([^\n]+).*$" "\\1" PYTHON_PREFIX ${python_config})
      STRING(REGEX REPLACE ".*\nshort_version:([^\n]+).*$" "\\1" PYTHON_SHORT_VERSION ${python_config})
      STRING(REGEX REPLACE ".*\nlong_version:([^\n]+).*$" "\\1" PYTHON_LONG_VERSION ${python_config})
      STRING(REGEX REPLACE ".*\npy_inc_dir:([^\n]+).*$" "\\1" PYTHON_INCLUDE_PATH ${python_config})
      STRING(REGEX REPLACE ".*\nsite_packages_dir:([^\n]+).*$" "\\1" PYTHON_SITE_PACKAGES_DIR ${python_config})
      STRING(REGEX REPLACE "([0-9]+).([0-9]+)" "\\1\\2" PYTHON_SHORT_VERSION_NO_DOT ${PYTHON_SHORT_VERSION})
      set(PYTHON_LIBRARY_NAMES python${PYTHON_SHORT_VERSION} python${PYTHON_SHORT_VERSION_NO_DOT})
      if(WIN32)
          STRING(REPLACE "\\" "/" PYTHON_SITE_PACKAGES_DIR ${PYTHON_SITE_PACKAGES_DIR})
      endif()
      FIND_LIBRARY(PYTHON_LIBRARY NAMES ${PYTHON_LIBRARY_NAMES} PATHS ${PYTHON_PREFIX}/lib ${PYTHON_PREFIX}/libs NO_DEFAULT_PATH)
      set(PYTHONLIBRARY_FOUND TRUE)
    endif()

    # adapted from cmake's builtin FindPythonLibs
    if(APPLE)
      CMAKE_FIND_FRAMEWORKS(Python)
      set(PYTHON_FRAMEWORK_INCLUDES)
      if(Python_FRAMEWORKS)
        # If a framework has been selected for the include path,
        # make sure "-framework" is used to link it.
        if("${PYTHON_INCLUDE_PATH}" MATCHES "Python\\.framework")
          set(PYTHON_LIBRARY "")
          set(PYTHON_DEBUG_LIBRARY "")
        endif()
        if(NOT PYTHON_LIBRARY)
          set (PYTHON_LIBRARY "-framework Python" CACHE FILEPATH "Python Framework" FORCE)
        endif()
        set(PYTHONLIBRARY_FOUND TRUE)
      endif()
    endif()

  endif()

  if(PYTHONLIBRARY_FOUND)
    set(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
    if(NOT PYTHONLIBRARY_FIND_QUIETLY)
      message(STATUS "Found Python executable: ${PYTHON_EXECUTABLE}")
      message(STATUS "Found Python version: ${PYTHON_LONG_VERSION}")
    endif()
  else()
    if(PYTHONLIBRARY_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Python")
    endif()
  endif()

endif()
