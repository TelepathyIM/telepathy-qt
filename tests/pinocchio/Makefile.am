# Tests that need telepathy-pinocchio, run under their own dbus-daemon

if HAVE_PINOCCHIO

TESTS = \
    test-chan-basics \
    test-conn-basics \
    test-do-nothing

BUILT_SOURCES = \
    _gen/chan-basics.cpp.moc.hpp \
    _gen/conn-basics.cpp.moc.hpp \
    _gen/do-nothing.cpp.moc.hpp \
    _gen/lib.h.moc.hpp

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(LIB_CFLAGS) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_chan_basics_SOURCES = chan-basics.cpp lib.cpp
test_chan_basics_LDADD = $(ALL_LIBS)

test_conn_basics_SOURCES = conn-basics.cpp lib.cpp
test_conn_basics_LDADD = $(ALL_LIBS)

test_do_nothing_SOURCES = do-nothing.cpp lib.cpp
test_do_nothing_LDADD = $(ALL_LIBS)

LIB_CFLAGS = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)
ALL_LIBS = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    $(TP_QT4_LIBS)

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

CLEANFILES = \
    $(BUILT_SOURCES)

distclean-local:
	rm -rf _gen

endif
