include $(top_srcdir)/tools/telepathy-qt4.mk

SUBDIRS = python
DIST_SUBDIRS = glib python

if ENABLE_TP_GLIB_TESTS

SUBDIRS += glib

endif

AM_CPPFLAGS = \
    -I$(top_builddir) -I$(top_srcdir) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

AM_CXXFLAGS = $(ERROR_CXXFLAGS)

noinst_LTLIBRARIES = libtp-qt4-tests.la

BUILT_SOURCES = \
    _gen/test.h.moc.hpp
libtp_qt4_tests_la_SOURCES = \
    test.cpp \
    test.h
libtp_qt4_tests_la_LIBADD = \
    $(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

_gen/%.moc.hpp: %
	$(AM_V_at)$(mkdir_p) _gen
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@

CLEANFILES = $(BUILT_SOURCES)
