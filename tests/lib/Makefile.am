AM_CFLAGS = \
    $(ERROR_CFLAGS) \
    $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES =

if ENABLE_TP_GLIB_TESTS

noinst_LTLIBRARIES += libtp-glib-tests.la

libtp_glib_tests_la_SOURCES = \
    simple-conn.c \
    simple-conn.h \
    simple-manager.c \
    simple-manager.h
libtp_glib_tests_la_LIBADD = \
    $(TP_GLIB_LIBS)

endif
