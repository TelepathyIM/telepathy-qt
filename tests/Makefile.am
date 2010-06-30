include $(top_srcdir)/tools/telepathy-qt4.mk

# compile lib/ and this dir first, so they can be used by dbus tests
SUBDIRS = \
    lib \
    . \
    dbus

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
    dbus-1/session.conf \
    _gen/key-file.cpp.moc.hpp \
    _gen/manager-file.cpp.moc.hpp

dbus-1/%.conf: $(srcdir)/dbus-1/%.conf.in
	$(AM_V_at)$(mkdir_p) dbus-1
	$(AM_V_GEN)sed -e "s|[@]abs_top_builddir[@]|@abs_top_builddir@|g" $< > $@

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

include $(top_srcdir)/tools/valgrind.mk

VALGRIND_TESTS_ENVIRONMENT = \
    $(TESTS_ENVIRONMENT) \
    env G_SLICE=always-malloc \
    $(top_builddir)/libtool --mode=execute \
    $(VALGRIND) $(VALGRIND_FLAGS)

check-valgrind:
	$(MAKE) check-TESTS \
		maybe_gc_friendly=,gc-friendly \
		TESTS_ENVIRONMENT="$(VALGRIND_TESTS_ENVIRONMENT)"
	$(MAKE) -C dbus check-valgrind

CLEANFILES = \
    $(BUILT_SOURCES)

EXTRA_DIST = \
    README \
    dbus-1/session.conf.in \
    dbus-1/services/account-manager.service.in \
    dbus-1/services/spurious.service \
    telepathy/managers/spurious.manager \
    telepathy/managers/test-manager-file.manager \
    telepathy/managers/test-manager-file-invalid-signature.manager \
    telepathy/managers/test-manager-file-malformed-keyfile.manager \
    test-key-file.ini \
    test-key-file-format-error.ini

_gen/%.moc.hpp: %
	$(AM_V_at)$(mkdir_p) _gen
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@

distclean-local:
	rm -rf _gen
