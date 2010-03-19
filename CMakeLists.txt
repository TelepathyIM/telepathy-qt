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
set(TP_QT4_MINOR_VERSION 3)
set(TP_QT4_MICRO_VERSION 1)
set(TP_QT4_NANO_VERSION  1)

set(PACKAGE_NAME telepathy-qt4)
set(PACKAGE_VERSION ${TP_QT4_MAJOR_VERSION}.${TP_QT4_MINOR_VERSION}.${TP_QT4_MICRO_VERSION}.${TP_QT4_NANO_VERSION})

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is
# checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Default build type is RelWithDebInfo for release versions and Debug for developement
# versions
if(NOT CMAKE_BUILD_TYPE)
    if(TP_QT4_NANO_VERSION EQUAL 0)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
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
    message(STATUS "Qt 4.${QT_VERSION_MINOR} found")
else(${QT_VERSION_MINOR} GREATER 4)
    message(SEND_ERROR "Qt >= 4.5 is required")
endif(${QT_VERSION_MINOR} GREATER 4)

# Check for Qt4 Glib support
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
set(CMAKE_REQUIRED_DEFINITIONS "")
set(CMAKE_REQUIRED_FLAGS "")

CHECK_CXX_SOURCE_COMPILES("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT4_GLIB_SUPPORT)

SET(CMAKE_REQUIRED_LIBRARIES ${QT_QTDBUS_LIBRARY})
CHECK_CXX_SOURCE_COMPILES("
#include <QtDBus>
int main()
{
QDBusVariant a; QDBusVariant b; if (a == b) { ; }
return 0;
}"
HAVE_QDBUSVARIANT_OPERATOR_EQUAL)
set(CMAKE_REQUIRED_LIBRARIES)

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
    message(STATUS "Python ${PYTHON_SHORT_VERSION} found")
else(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})
    message(SEND_ERROR "Python >= ${REQUIRED_PY} is required")
endif(${PYTHON_SHORT_VERSION} VERSION_GREATER ${REQUIRED_PY} OR ${PYTHON_SHORT_VERSION} VERSION_EQUAL ${REQUIRED_PY})

# Check for dbus-python
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import dbus.mainloop.glib"
                RESULT_VARIABLE PYTHON_DBUS_RESULT)
if(PYTHON_DBUS_RESULT EQUAL 0)
    message(STATUS "dbus-python installed, running additional tests")
    set(HAVE_TEST_PYTHON 1)
else(PYTHON_DBUS_RESULT EQUAL 0)
    message(STATUS "dbus-python not installed, skipping some tests")
    set(HAVE_TEST_PYTHON 0)
endif(PYTHON_DBUS_RESULT EQUAL 0)

# Optional dependencies
find_package(PkgConfig)

# Find tp-farsight
pkg_check_modules(TP_FARSIGHT telepathy-farsight>=0.0.4 gstreamer-0.10 gstreamer-interfaces-0.10)
if(TP_FARSIGHT_FOUND)
    message(STATUS "telepathy-farsight and gstreamer libraries found, building additional examples")
else(TP_FARSIGHT_FOUND)
    message(STATUS "telepathy-farsight or gstreamer libraries not found, not building some examples")
endif(TP_FARSIGHT_FOUND)

# Find telepathy-glib
set(REQUIRED_TP_GLIB 0.10.0)
pkg_check_modules(TPGLIB telepathy-glib>=${REQUIRED_TP_GLIB})

find_program(GLIB_GENMARSHAL glib-genmarshal)

# process results...
if(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)
    message(STATUS "Qt4 compiled with Glib support and telepathy-glib >= ${REQUIRED_TP_GLIB} found, running additional tests")
    set(ENABLE_TP_GLIB_TESTS 1)
else(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)
    message(STATUS "Qt4 not compiled with Glib support or telepathy-glib >= ${REQUIRED_TP_GLIB} not found, skipping some tests")
    set(ENABLE_TP_GLIB_TESTS 0)
endif(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)

add_subdirectory(TelepathyQt4)
add_subdirectory(examples)
add_subdirectory(tests)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
