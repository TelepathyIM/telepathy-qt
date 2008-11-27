# Tests that need telepathy-pinocchio, run under their own dbus-daemon

if HAVE_PINOCCHIO

TESTS = \
    test-do-nothing

BUILT_SOURCES = \
    _gen/do-nothing.cpp.moc.hpp \
    _gen/lib.h.moc.hpp

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(LIB_CFLAGS) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_do_nothing_SOURCES = do-nothing.cpp
test_do_nothing_LDADD = $(TP_QT4_LIBS) $(QTTEST_LIBS) $(noinst_LTLIBRARIES)

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
    XDG_DATA_DIRS=@abs_top_srcdir@/tests \
    PINOCCHIO=@PINOCCHIO@ \
    PINOCCHIO_CTL=@PINOCCHIO_CTL@ \
    PINOCCHIO_DATA_DIR=@abs_srcdir@ \
    PINOCCHIO_SAVE_DIR=@abs_builddir@

TESTS_ENVIRONMENT = \
    $(TESTS_ENV) \
    sh $(top_srcdir)/tools/with-session-bus.sh --session --

noinst_LTLIBRARIES = libpinocchio-tests.la

libpinocchio_tests_la_SOURCES = \
    lib.cpp \
    lib.h

endif
