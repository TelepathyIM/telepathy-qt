# - Try to find GObject
# Once done this will define
#
#  GOBJECT_FOUND - system has GObject
#  GOBJECT_INCLUDE_DIR - the GObject include directory
#  GOBJECT_LIBRARIES - the libraries needed to use GObject
#  GOBJECT_DEFINITIONS - Compiler switches required for using GObject

# Copyright (c) 2008 Helio Chissini de Castro, <helio@kde.org>
#  (c)2006, Tim Beaulen <tbscope@gmail.com>


IF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   # in cache already
   SET(GObject_FIND_QUIETLY TRUE)
ELSE ()
   SET(GObject_FIND_QUIETLY FALSE)
ENDIF ()

IF (NOT WIN32)
   FIND_PACKAGE(PkgConfig REQUIRED)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   PKG_CHECK_MODULES(PKG_GOBJECT2 REQUIRED gobject-2.0)
   SET(GOBJECT_DEFINITIONS ${PKG_GOBJECT2_CFLAGS})
ENDIF ()

FIND_PATH(GOBJECT_INCLUDE_DIR gobject/gobject.h
    HINTS
    ${PKG_GOBJECT2_INCLUDE_DIRS}
    PATHS
    /usr/include/glib-2.0/
    PATH_SUFFIXES glib-2.0
)

FIND_LIBRARY(_GObjectLibs NAMES gobject-2.0
   HINTS
   ${PKG_GOBJECT2_LIBRARY_DIRS}
   )
FIND_LIBRARY(_GModuleLibs NAMES gmodule-2.0
   HINTS
   ${PKG_GOBJECT2_LIBRARY_DIRS}
   )
FIND_LIBRARY(_GThreadLibs NAMES gthread-2.0
   HINTS
   ${PKG_GOBJECT2_LIBRARY_DIRS}
   )
FIND_LIBRARY(_GLibs NAMES glib-2.0
   HINTS
   ${PKG_GOBJECT2_LIBRARY_DIRS}
   )

IF (WIN32)
SET (GOBJECT_LIBRARIES ${_GObjectLibs} ${_GModuleLibs} ${_GThreadLibs} ${_GLibs})
ELSE ()
SET (GOBJECT_LIBRARIES ${PKG_GOBJECT2_LIBRARIES})
ENDIF ()

IF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   SET(GOBJECT_FOUND TRUE)
ELSE ()
   SET(GOBJECT_FOUND FALSE)
ENDIF ()

IF (GOBJECT_FOUND)
   IF (NOT GObject_FIND_QUIETLY)
      MESSAGE(STATUS "Found GObject libraries: ${GOBJECT_LIBRARIES}")
      MESSAGE(STATUS "Found GObject includes : ${GOBJECT_INCLUDE_DIR}")
   ENDIF ()
ELSE ()
    IF (GObject_FIND_REQUIRED)
      MESSAGE(STATUS "Could NOT find GObject")
    ENDIF()
ENDIF ()

MARK_AS_ADVANCED(GOBJECT_INCLUDE_DIR GOBJECT_LIBRARIES)
