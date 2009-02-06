AM_CFLAGS = $(ERROR_CFLAGS)
AM_CXXFLAGS = $(ERROR_CXXFLAGS)

if ENABLE_TP_GLIB_TESTS

AM_CFLAGS += $(TP_GLIB_CFLAGS)
AM_CXXFLAGS += $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES = libtp-glib-csh-tests.la

libtp_glib_csh_tests_la_SOURCES = \
    conn.c \
    conn.h \
    room.c \
    room.h \
    room-manager.c \
    room-manager.h

libtp_glib_csh_tests_la_LIBADD = \
    $(TP_GLIB_LIBS)

endif
