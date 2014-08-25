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

# Add an option to decide where to install the config files
if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_GREATER 2.6.2)
    option(USE_COMMON_CMAKE_PACKAGE_CONFIG_DIR "Prefer to install the <package>Config.cmake files to lib/cmake/<package> instead of lib/<package>/cmake" TRUE)
endif (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION} VERSION_GREATER 2.6.2)

# Set compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG          "-ggdb -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
    set(CMAKE_CXX_FLAGS_DEBUGFULL      "-O0 -g3 -ggdb -fno-inline")
    set(CMAKE_CXX_FLAGS_PROFILE        "-pg -g3 -ggdb -DNDEBUG")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -ggdb")
    set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG            "-ggdb -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
    set(CMAKE_C_FLAGS_DEBUGFULL        "-O0 -g3 -ggdb -fno-inline")
    set(CMAKE_C_FLAGS_PROFILE          "-pg -g3 -ggdb -DNDEBUG")

    set(CMAKE_EXE_LINKER_FLAGS_PROFILE    "-pg -ggdb")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "-pg -ggdb")

    set(DISABLE_WERROR 0 CACHE BOOL "compile without -Werror (normally enabled in development builds)")

    include(CompilerWarnings)
    include(TestCXXAcceptsFlag)

    CHECK_CXX_ACCEPTS_FLAG("-fvisibility=hidden" CXX_FVISIBILITY_HIDDEN)
    if (CXX_FVISIBILITY_HIDDEN)
        set(VISIBILITY_HIDDEN_FLAGS "-fvisibility=hidden")
    else (CXX_FVISIBILITY_HIDDEN)
        set(VISIBILITY_HIDDEN_FLAGS)
    endif (CXX_FVISIBILITY_HIDDEN)

    CHECK_CXX_ACCEPTS_FLAG("-fvisibility-inlines-hidden" CXX_FVISIBILITY_INLINES_HIDDEN)
    if (CXX_FVISIBILITY_INLINES_HIDDEN)
        set(VISIBILITY_HIDDEN_FLAGS "${VISIBILITY_HIDDEN_FLAGS} -fvisibility-inlines-hidden")
    endif (CXX_FVISIBILITY_INLINES_HIDDEN)

    CHECK_CXX_ACCEPTS_FLAG("-Wdeprecated-declarations" CXX_DEPRECATED_DECLARATIONS)
    if (CXX_DEPRECATED_DECLARATIONS)
        set(DEPRECATED_DECLARATIONS_FLAGS "-Wdeprecated-declarations -DTP_QT_DEPRECATED_WARNINGS")
    else (CXX_DEPRECATED_DECLARATIONS)
        set(DEPRECATED_DECLARATIONS_FLAGS)
    endif (CXX_DEPRECATED_DECLARATIONS)

    if(${TP_QT_NANO_VERSION} EQUAL 0)
        set(NOT_RELEASE 0)
    else(${TP_QT_NANO_VERSION} EQUAL 0)
        set(NOT_RELEASE 1)
    endif(${TP_QT_NANO_VERSION} EQUAL 0)

    set(desired
        all
        extra
        sign-compare
        pointer-arith
        format-security
        init-self
        non-virtual-dtor)
    set(undesired
        missing-field-initializers
        unused-parameter
        unused-local-typedefs)
    compiler_warnings(CMAKE_CXX_FLAGS_WARNINGS cxx ${NOT_RELEASE} "${desired}" "${undesired}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_WARNINGS}")

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
        unused-parameter
        unused-local-typedefs)
    compiler_warnings(CMAKE_C_FLAGS_WARNINGS c ${NOT_RELEASE} "${desired_c}" "${undesired_c}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_WARNINGS}")

    # Link development builds with -Wl,--no-add-needed
    # TODO: binutils 2.21 renames the flag to --no-copy-dt-needed-entries, though it keeps the old
    # one as a deprecated alias.
    if(${NOT_RELEASE} EQUAL 1)
        set(CMAKE_EXE_LINKER_FLAGS            "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-add-needed")
        set(CMAKE_SHARED_LINKER_FLAGS         "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-add-needed")
    endif(${NOT_RELEASE} EQUAL 1)

    if(CMAKE_SYSTEM_NAME MATCHES Linux)
        add_definitions(-D_BSD_SOURCE)
    endif(CMAKE_SYSTEM_NAME MATCHES Linux)

    # Compiler coverage
    set(ENABLE_COMPILER_COVERAGE OFF CACHE BOOL "Enables compiler coverage tests through lcov. Enabling this option will build
Telepathy-Qt as a static library.")

    if (ENABLE_COMPILER_COVERAGE)
        check_cxx_accepts_flag("-fprofile-arcs -ftest-coverage" CXX_FPROFILE_ARCS)
        check_cxx_accepts_flag("-ftest-coverage" CXX_FTEST_COVERAGE)

        if (CXX_FPROFILE_ARCS AND CXX_FTEST_COVERAGE)
            find_program(LCOV lcov)
            find_program(LCOV_GENHTML genhtml)
            if (NOT LCOV OR NOT LCOV_GENHTML)
                message(FATAL_ERROR "You chose to use compiler coverage tests, but lcov or genhtml could not be found in your PATH.")
            else (NOT LCOV OR NOT LCOV_GENHTML)
                message(STATUS "Compiler coverage tests enabled - Telepathy-Qt will be compiled as a static library")
                set(COMPILER_COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage")
            endif (NOT LCOV OR NOT LCOV_GENHTML)
        else (CXX_FPROFILE_ARCS AND CXX_FTEST_COVERAGE)
            message(FATAL_ERROR "You chose to use compiler coverage tests, but it appears your compiler is not able to support them.")
        endif (CXX_FPROFILE_ARCS AND CXX_FTEST_COVERAGE)
    else (ENABLE_COMPILER_COVERAGE)
        set(COMPILER_COVERAGE_FLAGS)
    endif (ENABLE_COMPILER_COVERAGE)

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
set(LIB_INSTALL_DIR     "lib${LIB_SUFFIX}"  CACHE PATH "The subdirectory where libraries will be installed (default is ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})" FORCE)
set(INCLUDE_INSTALL_DIR "include"           CACHE PATH "The subdirectory where header files will be installed (default is ${CMAKE_INSTALL_PREFIX}/include)" FORCE)
set(DATA_INSTALL_DIR    "share/telepathy"   CACHE PATH "The subdirectory where data files will be installed (default is ${CMAKE_INSTALL_PREFIX}/share/telepathy)" FORCE)
