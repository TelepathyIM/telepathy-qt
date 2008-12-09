if HAVE_QTGUI

# Tests for the prototype code, run under their own dbus-daemon.

manual_tests = \
    test-prototype

# test-prototype is not run automatically because it isn't self-contained -
# it needs Gabble, a local Jabber server with an account already set up, and
# Mission Control 5
TESTS =
noinst_PROGRAMS = \
    $(TESTS) \
    $(manual_tests)

BUILT_SOURCES = \
    _gen/resources.cpp \
    _gen/prototype.h.moc

test_prototype_SOURCES = prototype.cpp prototype.h _gen/resources.cpp

EXTRA_DIST = images.qrc avatar.png

_gen/resources.cpp: images.qrc avatar.png
	( cd @srcdir@ && $(RCC) $< ) > $@.tmp
	mv -f $@.tmp $@

DEP_CFLAGS = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTGUI_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS) \
    $(PROTO_CFLAGS)

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTGUI_LIBS) \
    $(QTTEST_LIBS) \
    $(top_builddir)/TelepathyQt4/libtelepathy-qt4.la \
    $(top_builddir)/TelepathyQt4/Prototype/libtpqt4-prototype.la

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(DEP_CFLAGS) \
    -Wno-error=missing-declarations

TESTS_ENV = \
    abs_top_builddir=@abs_top_builddir@ \
    abs_top_srcdir=@abs_top_srcdir@ \
    XDG_DATA_HOME=@abs_top_builddir@/tests \
    XDG_DATA_DIRS=@abs_top_srcdir@/tests

TESTS_ENVIRONMENT = \
    $(TESTS_ENV) \
    sh $(top_srcdir)/tools/with-session-bus.sh --session --

CLEANFILES = \
    $(BUILT_SOURCES)

_gen/%.moc: %
	$(mkdir_p) _gen
	$(MOC) $(DEP_CFLAGS) -i $< -o $@

distclean-local:
	rm -rf _gen

endif
