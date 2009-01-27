AM_CFLAGS = $(ERROR_CFLAGS)
AM_CXXFLAGS = $(ERROR_CXXFLAGS)

if ENABLE_TP_GLIB_TESTS

AM_CFLAGS += $(TP_GLIB_CFLAGS)
AM_CXXFLAGS += $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES = libtp-glib-echo2-tests.la

libtp_glib_echo2_tests_la_SOURCES = \
    chan.c \
    chan.h \
    conn.c \
    conn.h \
    im-manager.c \
    im-manager.h

libtp_glib_echo2_tests_la_LIBADD = \
    $(TP_GLIB_LIBS)

endif
