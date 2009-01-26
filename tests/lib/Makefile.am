AM_CFLAGS = $(ERROR_CFLAGS)

MOC_INCLUDES = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

AM_CXXFLAGS = $(ERROR_CXXFLAGS) $(MOC_INCLUDES)

noinst_LTLIBRARIES = libtp-qt4-tests.la

EXTRA_DIST = \
    account-manager.py

BUILT_SOURCES = \
    _gen/test.h.moc.hpp

libtp_qt4_tests_la_SOURCES = \
    test.cpp \
    test.h
libtp_qt4_tests_la_LIBADD = $(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

if ENABLE_TP_GLIB_TESTS

AM_CFLAGS += $(TP_GLIB_CFLAGS)
AM_CXXFLAGS += $(TP_GLIB_CFLAGS)

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
    $(TP_GLIB_LIBS)

endif

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(MOC_INCLUDES) -i $< -o $@
