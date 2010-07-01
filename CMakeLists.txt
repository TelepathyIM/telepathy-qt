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
set(TP_QT4_MICRO_VERSION 5)
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
include(MacroLogFeature)

# external dependencies

# Required dependencies
# Find qt4 version >= 4.5
set (QT_MIN_VERSION "4.5.0")
find_package(Qt4 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${QT_INCLUDES})

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
macro_log_feature(QT4_GLIB_SUPPORT "Qt4 Glib Support"
                  "QtCore library using Glib's main event loop"
                  "http://qt.nokia.com/" FALSE ""
                  "Needed, together with Telepathy-Glib, to build some additional test cases")

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

# helper macro to set up a moc rule
MACRO (QT4_CREATE_MOC_COMMAND_TARGET_DEPS infile outfile moc_flags moc_options)
  # For Windows, create a parameters file to work around command line length limit
  GET_FILENAME_COMPONENT(_moc_outfile_name "${outfile}" NAME)
  IF (WIN32)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    GET_FILENAME_COMPONENT(_moc_outfile_dir "${outfile}" PATH)
    IF(_moc_outfile_dir)
      SET(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    ENDIF(_moc_outfile_dir)
    SET (_moc_parameters_file ${outfile}_parameters)
    SET (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    FILE (REMOVE ${_moc_parameters_file})
    FOREACH(arg ${_moc_parameters})
      FILE (APPEND ${_moc_parameters_file} "${arg}\n")
    ENDFOREACH(arg)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE} @${_moc_outfile_name}_parameters
                       DEPENDS ${infile}
                       ${_moc_working_dir}
                       VERBATIM)
  ELSE (WIN32)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE}
                       ARGS ${moc_flags} ${moc_options} -o ${outfile} ${infile}
                       DEPENDS ${infile})
  ENDIF (WIN32)

  add_custom_target(moc-${_moc_outfile_name} DEPENDS ${outfile})
  add_dependencies(moc-${_moc_outfile_name} ${ARGN})
ENDMACRO (QT4_CREATE_MOC_COMMAND_TARGET_DEPS)

# add the -i option to QT4_GENERATE_MOC
macro(QT4_GENERATE_MOC_I infile outfile)
    qt4_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt4_create_moc_command(${abs_infile} ${outfile} "${moc_flags}" "-i")
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endmacro(QT4_GENERATE_MOC_I)

macro(QT4_GENERATE_MOC_I_TARGET_DEPS infile outfile)
    qt4_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt4_create_moc_command_target_deps(${abs_infile} ${outfile} "${moc_flags}" "-i" ${ARGN})
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endmacro(QT4_GENERATE_MOC_I_TARGET_DEPS)

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
    set(HAVE_TEST_PYTHON 1)
else(PYTHON_DBUS_RESULT EQUAL 0)
    set(HAVE_TEST_PYTHON 0)
endif(PYTHON_DBUS_RESULT EQUAL 0)
macro_log_feature(HAVE_TEST_PYTHON "dbus-python"
                  "GLib-based Python DBus support"
                  "http://www.python.org/" FALSE ""
                  "Needed to build some additional test cases")

# Find GLib2, GObject, DBus and LibXml2
# Those are needed for the insane include dir dependency hell
find_package(GLIB2)
find_package(GObject)
find_package(DBus)
find_package(LibXml2)

# Find tp-farsight
set(TELEPATHY_FARSIGHT_MIN_VERSION "0.0.4")
find_package(TelepathyFarsight)
macro_log_feature(TELEPATHYFARSIGHT_FOUND "Telepathy-Farsight"
                  "A Framework for dealing with audio/video conferencing protocols"
                  "http://farsight.freedesktop.org/wiki/" FALSE "0.0.4"
                  "Needed, together with GStreamer, to build telepathy-qt4-farsight and some additional examples")
find_package(GStreamer)
macro_log_feature(GSTREAMER_FOUND "GStreamer"
                  "An open source multimedia framework"
                  "Needed, together with Tp-Farsight, to build telepathy-qt4-farsight and some additional examples"
                  "http://www.gstreamer.net/" FALSE)
if (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)
    set (FARSIGHT_COMPONENTS_FOUND 1)
else (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)
    set (FARSIGHT_COMPONENTS_FOUND 0)
endif (TELEPATHYFARSIGHT_FOUND AND GSTREAMER_FOUND AND GLIB2_FOUND AND GOBJECT_FOUND AND DBUS_FOUND AND LIBXML2_FOUND)

# Find telepathy-glib
set(TELEPATHY_GLIB_MIN_VERSION 0.11.3)
find_package(TelepathyGlib)
macro_log_feature(TELEPATHYGLIB_FOUND "Telepathy-glib"
                  "Glib bindings for Telepathy"
                  "http://telepathy.freedesktop.org/" FALSE "0.11.3"
                  "Needed, together with Qt Glib integration, to build some additional test cases")

find_program(GLIB_GENMARSHAL glib-genmarshal)

# process results...
if(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
    set(ENABLE_TP_GLIB_TESTS 1)
else(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)
    set(ENABLE_TP_GLIB_TESTS 0)
endif(QT4_GLIB_SUPPORT AND TELEPATHYGLIB_FOUND AND GLIB2_FOUND AND DBUS_FOUND)

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

macro_display_feature_log()
