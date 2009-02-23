# Taken from telepathy-glib. The only change is to remove main.c and cut down
# the Makefile.am accordingly.
#
# PLEASE DO NOT MODIFY THIS CONNECTION MANAGER. Either subclass it,
# copy-and-modify (moving it to a better namespace), or make changes in the
# copy in telepathy-glib first.

noinst_LTLIBRARIES = libexample-cm-echo.la

libexample_cm_echo_la_SOURCES = \
    chan.c \
    chan.h \
    conn.c \
    conn.h \
    connection-manager.c \
    connection-manager.h \
    im-manager.c \
    im-manager.h

libexample_cm_echo_la_LIBADD = \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    $(ERROR_CFLAGS) \
    $(TP_GLIB_CFLAGS)
