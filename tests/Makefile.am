# compile lib/ first, so it can be used by tests in the other directories
SUBDIRS = \
    lib \
    dbus \
    pinocchio

# Standalone tests that don't touch D-Bus
TESTS = \
    test-key-file \
    test-manager-file

noinst_PROGRAMS = $(TESTS)

test_key_file_SOURCES = \
    key-file.cpp

test_manager_file_SOURCES = \
    manager-file.cpp

BUILT_SOURCES = \
    _gen/key-file.cpp.moc.hpp \
    _gen/manager-file.cpp.moc.hpp

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

AM_CPPFLAGS = \
    -I$(top_srcdir) -I$(top_builddir) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS)

TESTS_ENV = \
    abs_top_builddir=@abs_top_builddir@ \
    abs_top_srcdir=@abs_top_srcdir@ \
    XDG_DATA_HOME=@abs_top_builddir@/tests \
    XDG_DATA_DIRS=@abs_top_srcdir@/tests

TESTS_ENVIRONMENT = \
    $(TESTS_ENV)

CLEANFILES = \
    $(BUILT_SOURCES)

EXTRA_DIST = \
    README \
    dbus-1/services/account-manager.service.in \
    dbus-1/services/spurious.service \
    telepathy/managers/spurious.manager \
    telepathy/managers/test-manager-file.manager \
    telepathy/managers/test-manager-file-invalid.manager \
    test-key-file.ini \
    test-key-file-format-error.ini

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@

distclean-local:
	rm -rf _gen
