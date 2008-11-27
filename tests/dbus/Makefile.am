# Tests each run under their own temporary dbus-daemon.

TESTS = \
    test-do-nothing

noinst_PROGRAMS = $(TESTS)

test_do_nothing_SOURCES = do-nothing.cpp
test_do_nothing_LDADD = $(TP_QT4_LIBS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(TP_QT4_CFLAGS)

TESTS_ENV = \
    abs_top_builddir=@abs_top_builddir@ \
    abs_top_srcdir=@abs_top_srcdir@ \
    XDG_DATA_HOME=@abs_top_builddir@/tests \
    XDG_DATA_DIRS=@abs_top_srcdir@/tests

TESTS_ENVIRONMENT = \
    $(TESTS_ENV) \
    sh $(top_srcdir)/tools/with-session-bus.sh --session --
