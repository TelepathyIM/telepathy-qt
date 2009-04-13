cmake_minimum_required(VERSION 2.6)

if(POLICY CMP0011)
    cmake_policy(SET CMP0011 OLD)
endif(POLICY CMP0011)

# Enable testing using CTest
enable_testing()

# Always include srcdir and builddir in include path
# This saves typing ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} in about every subdir
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# put the include dirs which are in the source or build tree
# before all other include dirs, so the headers in the sources
# are prefered over the already installed ones
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

# Use colored output
set(CMAKE_COLOR_MAKEFILE ON)

# Set compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DQT_NO_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DQT_NO_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG          "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline -DENABLE_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline -DENABLE_DEBUG")
    set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs -DENABLE_DEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g -DQT_NO_DEBUG")
    set(CMAKE_C_FLAGS_RELEASE          "-O2 -DQT_NO_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG            "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline -DENABLE_DEBUG")
    set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline -DENABLE_DEBUG")
    set(CMAKE_C_FLAGS_PROFILE          "-g3 -fno-inline -ftest-coverage -fprofile-arcs -DENABLE_DEBUG")

    if(CMAKE_SYSTEM_NAME MATCHES Linux)
        add_definitions(-D_BSD_SOURCE)
    endif(CMAKE_SYSTEM_NAME MATCHES Linux)

    # gcc under Windows
    if(MINGW)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols -Wl,--disable-auto-import")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--export-all-symbols -Wl,--disable-auto-import")

        # we always link against the release version of QT with mingw
        # (even for debug builds). So we need to define QT_NO_DEBUG
        # or else QPluginLoader rejects plugins because it thinks
        # they're built against the wrong QT.
        add_definitions(-DQT_NO_DEBUG)
    endif(MINGW)
endif(CMAKE_COMPILER_IS_GNUCXX)

if(MSVC)
    set(ESCAPE_CHAR ^)
endif(MSVC)

set(LIB_SUFFIX "" CACHE STRING "Define suffix of library directory name (32/64)" )
set(LIB_INSTALL_DIR     "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}"  CACHE PATH "The subdirectory relative to the install prefix where libraries will be installed (default is ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})" FORCE)
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include"           CACHE PATH "The subdirectory to the header prefix" FORCE)
set(DISABLE_WERROR 0 CACHE BOOL "compile without -Werror (normally enabled in development builds)")

include(CompilerWarnings)

if(${CMAKE_BUILD_TYPE} STREQUAL Release)
    set(NOT_RELEASE 0)
else(${CMAKE_BUILD_TYPE} STREQUAL Release)
    set(NOT_RELEASE 1)
endif(${CMAKE_BUILD_TYPE} STREQUAL Release)

set(desired 
    all
    extra
    sign-compare
    pointer-arith
    format-security
    init-self)
set(undesired
    missing-field-initializers
    unused-parameter)
compiler_warnings(CMAKE_CXX_FLAGS cxx ${NOT_RELEASE} "${desired}" "${undesired}")

set(desired_c 
    all 
    extra 
    declaration-after-statement 
    shadow
    strict-prototypes
    missing-prototypes
    sign-compare
    nested-externs
    pointer-arith
    format-security
    init-self)
set(undesired_c
    missing-field-initializers
    unused-parameter )
compiler_warnings(CMAKE_C_FLAGS c ${NOT_RELEASE} "${desired_c}" "${undesired_c}")

include(TelepathyDist)
