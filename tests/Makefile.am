SUBDIRS = \
    dbus \
    pinocchio \
    prototype

TESTS = \
    test-key-file

noinst_PROGRAMS = $(TESTS)

test_key_file_SOURCES = key-file.cpp

BUILT_SOURCES = \
    key-file.moc.hpp

LDADD = \
    $(QTCORE_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

LIB_CFLAGS = \
    $(QTCORE_CFLAGS) \
    $(QTTEST_CFLAGS) \
    -I${top_srcdir}

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(LIB_CFLAGS)

CLEANFILES = \
    $(BUILT_SOURCES)

EXTRA_DIST = \
    README \
    dbus-1/services/spurious.service \
    telepathy/managers/spurious.manager \
    test-key-file.ini \
    test-key-file-format-error.ini

%.moc.hpp: %.cpp
	$(MOC) $(LIB_CFLAGS) -i $< -o $@
