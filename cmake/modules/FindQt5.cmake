# - Find Qt5
# This module can be used to find Qt5.
# The most important issues are that Qt5 pkgconfig files are installed with PKG_CONFIG_PATH properly
# set, and that Qt5 qmake is available via the system path.
# This module defines a number of key variables and macros.
#
#  Below is a detailed list of variables that FindQt5.cmake sets.
#  QT_FOUND                     If false, don't try to use Qt.
#  QT5_FOUND                    If false, don't try to use Qt5.
#
#  QT_VERSION_MAJOR             The major version of Qt found.
#  QT_VERSION_MINOR             The minor version of Qt found.
#  QT_VERSION_PATCH             The patch version of Qt found.
#
#  QT_BINARY_DIR                Path to "bin" of Qt5
#  QT_DOC_DIR                   Path to "doc" of Qt5
#
#  QT_INCLUDES                  List of paths to all include directories of Qt5.
#
#  QT_LIBRARIES                 List of paths to all libraries of Qt5.
#  QT_QTCORE_LIBRARY            The QtCore library
#  QT_QTDBUS_LIBRARY            The QtDBus library
#  QT_QTGUI_LIBRARY             The QtGui library
#  QT_QTNETWORK_LIBRARY         The QtNetwork library
#  QT_QTTEST_LIBRARY            The QtTest library
#  QT_QTWIDGETS_LIBRARY         The QtWidgets library
#  QT_QTXML_LIBRARY             The QtXml library
#
# also defined, but NOT for general use are
#  QT_MOC_EXECUTABLE            Where to find the moc tool
#  QT_CONFIG_FLAGS              Flags used when building Qt

# Copyright (C) 2001-2009 Kitware, Inc.
# Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright (C) 2011 Nokia Corporation
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF(QT_INCLUDES AND QT_LIBRARIES AND QT_MAJOR_VERSION MATCHES 5)
  # Already in cache, be silent
  SET(QT_FOUND TRUE)
  SET(QT5_FOUND TRUE)
  RETURN()
ENDIF(QT_INCLUDES AND QT_LIBRARIES AND QT_MAJOR_VERSION MATCHES 5)

IF(NOT QT_QMAKE_EXECUTABLE)
  FIND_PROGRAM(QT_QMAKE_EXECUTABLE_FINDQT NAMES qmake qmake5 qmake-qt5
               PATHS "${QT_SEARCH_PATH}/bin" "$ENV{QTDIR}/bin")
  SET(QT_QMAKE_EXECUTABLE ${QT_QMAKE_EXECUTABLE_FINDQT} CACHE PATH "Qt qmake program.")
ENDIF(NOT QT_QMAKE_EXECUTABLE)

EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_VERSION" OUTPUT_VARIABLE QTVERSION)
IF(NOT QTVERSION MATCHES "5.*")
  SET(QT_FOUND FALSE)
  SET(QT5_FOUND FALSE)
  IF(Qt5_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "CMake was unable to find Qt5, put qmake in your path or set QTDIR/QT_QMAKE_EXECUTABLE.")
  ENDIF(Qt5_FIND_REQUIRED)
  RETURN()
ENDIF(NOT QTVERSION MATCHES "5.*")

FIND_PACKAGE(PkgConfig REQUIRED)

FIND_PACKAGE(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core DBus Gui Network Test Widgets Xml)

# Copy includes and library names into the same style as pkgconfig used for Qt4
set(QT_INCLUDES ${Qt5Core_INCLUDE_DIRS} ${Qt5DBus_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Test_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

set(QT_QTCORE_LIBRARY ${Qt5Core_LIBRARIES})
set(QT_QTDBUS_LIBRARY ${Qt5DBus_LIBRARIES})
set(QT_QTGUI_LIBRARY ${Qt5Gui_LIBRARIES})
set(QT_QTNETWORK_LIBRARY ${Qt5Network_LIBRARIES})
set(QT_QTTEST_LIBRARY ${Qt5Test_LIBRARIES})
set(QT_QTWIDGETS_LIBRARY ${Qt5Widgets_LIBRARIES})
set(QT_QTXML_LIBRARY ${Qt5Xml_LIBRARIES})

set(QT_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} ${QT_QTWIDGETS_LIBRARY} ${QT_QTXML_LIBRARY})

STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" QT_VERSION_MAJOR "${QTVERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+.*" "\\1" QT_VERSION_MINOR "${QTVERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" QT_VERSION_PATCH "${QTVERSION}")

IF(NOT QT_INCLUDE_DIR)
  EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_HEADERS" OUTPUT_VARIABLE QTHEADERS)
  SET(QT_INCLUDE_DIR ${QTHEADERS} CACHE INTERNAL "" FORCE)
ENDIF(NOT QT_INCLUDE_DIR)

IF(NOT QT_LIBRARY_DIR)
  EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_LIBS" OUTPUT_VARIABLE QTLIBS)
  SET(QT_LIBRARY_DIR ${QTLIBS} CACHE INTERNAL "" FORCE)
ENDIF(NOT QT_LIBRARY_DIR)

IF(NOT QT_BINARY_DIR)
  EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_BINS" OUTPUT_VARIABLE QTBINS)
  SET(QT_BINARY_DIR ${QTBINS} CACHE INTERNAL "" FORCE)
ENDIF(NOT QT_BINARY_DIR)

IF(NOT QT_DOC_DIR)
  EXEC_PROGRAM(${QT_QMAKE_EXECUTABLE} ARGS "-query QT_INSTALL_DOCS" OUTPUT_VARIABLE QTDOCS)
  SET(QT_DOC_DIR ${QTDOCS} CACHE INTERNAL "" FORCE)
ENDIF(NOT QT_DOC_DIR)

IF(NOT QT_MOC_EXECUTABLE)
  FIND_PROGRAM(QT_MOC_EXECUTABLE NAMES moc moc5 moc-qt5 PATHS ${QT_BINARY_DIR}
               NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
ENDIF(NOT QT_MOC_EXECUTABLE)

MARK_AS_ADVANCED(QT_INCLUDES QT_INCLUDE_DIR
                 QT_LIBRARIES QT_LIBRARY_DIR
                 QT_BINARY_DIR
                 QT_DOC_DIR
                 QT_QMAKE_EXECUTABLE_FINDQT QT_QMAKE_EXECUTABLE QT_MOC_EXECUTABLE)

# Invokes pkgconfig, cleans up the result and sets variables
EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable qt_config QtCore
    OUTPUT_VARIABLE _pkgconfig_flags
    RESULT_VARIABLE _pkgconfig_failed)
STRING(REPLACE " " ";" QT_CONFIG_FLAGS "${_pkgconfig_flags}")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

INCLUDE(Qt5Macros)

SET(QT_FOUND TRUE)
SET(QT5_FOUND TRUE)
