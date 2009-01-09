# Tests each run under their own temporary dbus-daemon.

TESTS = \
    test-do-nothing

BUILT_SOURCES = \
    _gen/do-nothing.moc.hpp

_gen/%.moc.hpp: %.cpp
	$(mkdir_p) _gen
	$(MOC) $(MOC_INCLUDES) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_do_nothing_SOURCES = do-nothing.cpp

if HAVE_TEST_PYTHON
# Tests which can only be run if we have suitable versions of Python and
# dbus-python
TESTS += \
    test-account-basics

BUILT_SOURCES += \
    _gen/account-basics.moc.hpp

test_account_basics_SOURCES = account-basics.cpp
endif

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

# A subset of the CXXFLAGS sufficient to run moc - -Istuff is a valid
# argument to moc but -Wstuff is not
MOC_INCLUDES = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(MOC_INCLUDES)

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
