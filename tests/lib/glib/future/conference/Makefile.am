noinst_LTLIBRARIES = libexample-cm-conference.la

libexample_cm_conference_la_SOURCES = \
    chan.c \
    chan.h

libexample_cm_conference_la_LIBADD = \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    -I$(top_srcdir)/tests/lib/glib/future \
    $(ERROR_CFLAGS) \
    $(TP_GLIB_CFLAGS)
