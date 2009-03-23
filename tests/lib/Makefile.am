SUBDIRS =

AM_CPPFLAGS = \
    -I$(top_builddir) -I$(top_srcdir) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

AM_CFLAGS = $(ERROR_CFLAGS)
AM_CXXFLAGS = $(ERROR_CXXFLAGS)

noinst_LTLIBRARIES = libtp-qt4-tests.la

EXTRA_DIST = \
    account-manager.py

BUILT_SOURCES = \
    _gen/test.h.moc.hpp
CLEANFILES = $(BUILT_SOURCES)

libtp_qt4_tests_la_SOURCES = \
    test.cpp \
    test.h
libtp_qt4_tests_la_LIBADD = $(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

if ENABLE_TP_GLIB_TESTS

SUBDIRS += callable contactlist echo
AM_CPPFLAGS += $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES += libtp-glib-tests.la

libtp_glib_tests_la_SOURCES = \
    contacts-conn.c \
    contacts-conn.h \
    debug.h \
    simple-conn.c \
    simple-conn.h \
    simple-manager.c \
    simple-manager.h
libtp_glib_tests_la_LIBADD = \
    $(TP_GLIB_LIBS) \
    callable/libexample-cm-callable.la \
    echo/libexample-cm-echo.la \
    contactlist/libexample-cm-contactlist.la

# Currently, the csh and echo2 subdirectories contain hacked-up versions of
# the telepathy-glib examples. In future we should fix this divergence and
# make them behave like the echo subdirectory instead. However, until then,
# we need to compile this directory before those subdirectories.
SUBDIRS += . csh echo2

endif

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@
