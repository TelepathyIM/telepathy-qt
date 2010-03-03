# Taken from telepathy-glib. The only change is to remove main.c and cut down
# the Makefile.am accordingly.
#
# PLEASE DO NOT MODIFY THIS CONNECTION MANAGER. Either subclass it,
# copy-and-modify (moving it to a better namespace), or make changes in the
# copy in telepathy-glib first.

noinst_LTLIBRARIES = libexample-cm-csh.la

libexample_cm_csh_la_SOURCES = \
    conn.c \
    conn.h \
    room.c \
    room.h \
    room-manager.c \
    room-manager.h

libexample_cm_csh_la_LIBADD = \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    -I$(top_builddir) -I$(top_srcdir) \
    $(ERROR_CFLAGS) \
    $(TP_GLIB_CFLAGS)
