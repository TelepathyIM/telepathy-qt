project(TelepathyQt4)

cmake_minimum_required(VERSION 2.6)

# Making releases:
#   set the new version number:
#     odd minor -> development series
#     even minor -> stable series
#     increment micro for each release within a series
#   set nano_version to 0
#   make the release, tag it
#   set nano_version to 1
set(TP_QT4_MAJOR_VERSION 0)
set(TP_QT4_MINOR_VERSION 1)
set(TP_QT4_MICRO_VERSION 2)
set(TP_QT4_NANO_VERSION  1)

set(PACKAGE_NAME telepathy-qt4)
set(PACKAGE_VERSION ${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION}.${TP_QT4_NANO_VERSION})
configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Default build type is Release for release versions and Debug for developement
# versions
if(NOT CMAKE_BUILD_TYPE)
    if(TP_QT4_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE Release)
    else(TP_QT4_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE Debug)
    endif(TP_QT4_NANO_VERSION EQUAL 0)
endif(NOT CMAKE_BUILD_TYPE)

include(TelepathyDefaults)
include(Doxygen)

# external dependencies

# Required dependencies
# Find qt4 version >= 4.5
find_package(Qt4 REQUIRED)
if(${QT_VERSION_MINOR} GREATER 4)
    message(STATUS "Qt 4.${QT_VERSION_MINOR} found.")
else(${QT_VERSION_MINOR} GREATER 4)
    message(SEND_ERROR "Qt 4.5 or greater is required")
endif(${QT_VERSION_MINOR} GREATER 4)

# add the -i option to QT4_GENERATE_MOC
macro(QT4_GENERATE_MOC_I infile outfile)
    qt4_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt4_create_moc_command(${abs_infile} ${outfile} "${moc_flags}" "-i")
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endmacro(QT4_GENERATE_MOC_I)

# Find python version >= 2.5
find_package(PythonLibrary REQUIRED)
set(REQUIRED_PY 2.5)
if(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})
    message(STATUS "Python ${PYTHON_SHORT_VERSION} found.")
else(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})
    message(SEND_ERROR "Python ${REQUIRED_PY} or greater is required.")
endif(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})

# Optional dependencies
find_package(PkgConfig)
pkg_check_modules(TP_FARSIGHT telepathy-farsight>=0.0.4 gstreamer-0.10 gstreamer-interfaces-0.10)
if(TP_FARSIGHT_FOUND)
    message(STATUS "telepathy-farsight and gstreamer libraries found, building additional examples")
else(TP_FARSIGHT_FOUND)
    message(STATUS "telepathy-farsight or gstreamer libraries not found, not building some examples")
endif(TP_FARSIGHT_FOUND)

add_subdirectory(TelepathyQt4)
add_subdirectory(examples)
add_subdirectory(tests)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
