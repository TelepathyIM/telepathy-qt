# Tests each run under their own temporary dbus-daemon.

TESTS = \
    test-do-nothing

BUILT_SOURCES = \
    _gen/do-nothing.moc.hpp

_gen/%.moc.hpp: %.cpp
	$(mkdir_p) _gen
	$(MOC) $(LIB_CFLAGS) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_do_nothing_SOURCES = do-nothing.cpp
test_do_nothing_LDADD = $(TP_QT4_LIBS) $(QTTEST_LIBS)

LIB_CFLAGS = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(LIB_CFLAGS)

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

distclean-local:
	rm -rf _gen
