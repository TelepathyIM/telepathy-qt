SUBDIRS = \
    dbus \
    pinocchio \
    prototype

TESTS = \
    test-key-file \
    test-manager-file

noinst_PROGRAMS = $(TESTS)

test_key_file_SOURCES = \
    key-file.cpp

test_manager_file_SOURCES = \
    manager-file.cpp

BUILT_SOURCES = \
    key-file.moc.hpp \
    manager-file.moc.hpp

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

LIB_CFLAGS = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    -I${top_srcdir}

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(LIB_CFLAGS)

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
    account-manager.py \
    dbus-1/services/account-manager.service.in \
    dbus-1/services/spurious.service \
    telepathy/managers/spurious.manager \
    telepathy/managers/test-manager-file.manager \
    telepathy/managers/test-manager-file-invalid.manager \
    test-key-file.ini \
    test-key-file-format-error.ini

%.moc.hpp: %.cpp
	$(MOC) $(LIB_CFLAGS) -i $< -o $@
