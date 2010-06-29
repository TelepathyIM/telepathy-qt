noinst_LTLIBRARIES = libexample-cm-conference.la

libexample_cm_conference_la_SOURCES = \
    chan.c \
    chan.h

libexample_cm_conference_la_LIBADD = \
    $(GLIB_LIBS) \
    $(DBUS_LIBS) \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    -I$(top_srcdir)/tests/lib/glib/future \
    $(ERROR_CFLAGS) \
    $(DBUS_CFLAGS) \
    $(TP_GLIB_CFLAGS)
