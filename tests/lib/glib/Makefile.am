SUBDIRS =
DIST_SUBDIRS = \
    callable \
    contactlist \
    csh \
    echo \
    echo2 \
    future

AM_CPPFLAGS = \
    -I$(top_builddir) -I$(top_srcdir)

AM_CFLAGS = $(ERROR_CFLAGS)

if ENABLE_TP_GLIB_TESTS

SUBDIRS += callable contactlist csh echo echo2 future
AM_CPPFLAGS += $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES = libtp-glib-tests.la

libtp_glib_tests_la_SOURCES = \
    contacts-conn.c \
    contacts-conn.h \
    debug.h \
    params-cm.c \
    params-cm.h \
    simple-account.c \
    simple-account.h \
    simple-account-manager.c \
    simple-account-manager.h \
    simple-channel-dispatch-operation.c \
    simple-channel-dispatch-operation.h \
    simple-client.c \
    simple-client.h \
    simple-conn.c \
    simple-conn.h \
    simple-manager.c \
    simple-manager.h \
    textchan-group.c \
    textchan-group.h \
    textchan-null.c \
    textchan-null.h \
    util.c \
    util.h

libtp_glib_tests_la_LIBADD = \
    $(TP_GLIB_LIBS) \
    callable/libexample-cm-callable.la \
    contactlist/libexample-cm-contactlist.la \
    csh/libexample-cm-csh.la \
    echo/libexample-cm-echo.la \
    echo2/libexample-cm-echo-2.la \
    future/call/libexample-cm-call.la \
    future/conference/libexample-cm-conference.la \
    future/extensions/libtp-glib-tests-future-extensions.la

endif

