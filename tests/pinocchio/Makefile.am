# Tests that need telepathy-pinocchio, run under their own dbus-daemon

if HAVE_PINOCCHIO

EXTRA_DIST = \
    accounts/empty/contacts.xml \
    lib.h

TESTS = \
    test-chan-basics \
    test-cm-basics \
    test-conn-basics \
    test-do-nothing

BUILT_SOURCES = \
    _gen/chan-basics.cpp.moc.hpp \
    _gen/cm-basics.cpp.moc.hpp \
    _gen/conn-basics.cpp.moc.hpp \
    _gen/do-nothing.cpp.moc.hpp \
    _gen/lib.h.moc.hpp

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(MOC_INCLUDES) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_chan_basics_SOURCES = chan-basics.cpp lib.cpp

test_cm_basics_SOURCES = cm-basics.cpp lib.cpp

test_conn_basics_SOURCES = conn-basics.cpp lib.cpp

test_do_nothing_SOURCES = do-nothing.cpp lib.cpp

# A subset of the CXXFLAGS sufficient to run moc - -Istuff is a valid
# argument to moc but -Wstuff is not
MOC_INCLUDES = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(MOC_INCLUDES)

# FIXME: XDG_DATA_DIRS should not depend on external setup
TESTS_ENV = \
    abs_top_builddir=@abs_top_builddir@ \
    abs_top_srcdir=@abs_top_srcdir@ \
    XDG_DATA_HOME=@abs_top_builddir@/tests \
    XDG_DATA_DIRS=@abs_top_srcdir@/tests:$(XDG_DATA_DIRS) \
    PINOCCHIO=@PINOCCHIO@ \
    PINOCCHIO_CTL=@PINOCCHIO_CTL@ \
    PINOCCHIO_DATA_DIR=@abs_srcdir@ \
    PINOCCHIO_SAVE_DIR=@abs_builddir@/_tmp

TESTS_ENVIRONMENT = \
    $(TESTS_ENV) \
    sh $(top_srcdir)/tools/with-session-bus.sh --session --

CLEANFILES = \
    $(BUILT_SOURCES)

distclean-local:
	rm -rf _gen _tmp

endif
