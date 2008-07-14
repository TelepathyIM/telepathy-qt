tpqt4includedir=$(includedir)/telepathy-1.0/TelepathyQt4
genincludedir=$(tpqt4includedir)/_gen

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = TelepathyQt4.pc

EXTRA_DIST = \
    channel.xml \
    connection.xml \
    connection-manager.xml \
    dbus-daemon.xml \
    dbus-introspectable.xml \
    dbus-peer.xml \
    dbus-properties.xml \
    generic.xml \
    media-session-handler.xml \
    media-stream-handler.xml \
    stable-interfaces.xml

lib_LTLIBRARIES = libtelepathy-qt4.la

# The quoting here is unnecessary but harmless, and has the useful side-effect
# that vim quickfix mode (:make) doesn't interpret the libtool --mode=link
# command as an error message in a bizarrely named file
libtelepathy_qt4_la_LDFLAGS = \
    -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
libtelepathy_qt4_la_LIBADD = $(ALL_LIBS)
libtelepathy_qt4_la_DEPENDENCIES = Makefile.am

libtelepathy_qt4_la_SOURCES = \
    types.cpp

nodist_libtelepathy_qt4_la_SOURCES = \
    _gen/types-body.hpp

tpqt4include_HEADERS = \
    Constants \
    Types \
    constants.h \
    types.h
nodist_geninclude_HEADERS = \
    _gen/constants.h \
    _gen/types.h

BUILT_SOURCES = \
    $(nodist_libtelepathy_qt4_la_SOURCES) \
    $(nodist_geninclude_HEADERS) \
    _gen/stable-spec.xml \
    _gen/spec-stamp \
    _gen/stable-stamp

CLEANFILES = \
    $(BUILT_SOURCES)

distclean-local:
	rm -rf _gen

include $(top_srcdir)/tools/check-coding-style.mk
check-local: check-coding-style

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    @QTCORE_CFLAGS@ \
    @QTDBUS_CFLAGS@ \
    -I$(top_builddir) \
    -I$(top_srcdir)

ALL_LIBS = \
    @QTCORE_LIBS@ \
    @QTDBUS_LIBS@

# Generated stuff

# Bootstrapping

_gen/spec-stamp: $(wildcard $(top_srcdir)/spec/*.xml)
	$(mkdir_p) _gen
	touch $@

_gen/stable-stamp: $(wildcard *.xml) _gen/spec-stamp
	touch $@

_gen/stable-spec.xml: stable-interfaces.xml _gen/stable-stamp \
	../tools/xincludator.py
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

# Things generated from the whole spec at once

_gen/constants.h: _gen/stable-spec.xml \
		../tools/qt4-constants-gen.py
	$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='Telepathy' \
		--str-constant-prefix='TP_' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/stable-spec.xml \
		../tools/qt4-types-gen.py
	$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='Telepathy' \
		--declfile='_gen/types.h' \
		--implfile='_gen/types-body.hpp' \
		--realinclude='TelepathyQt4/types.h' \
		--prettyinclude='TelepathyQt4/Types' \
		--specxml=$<

# Things generated per interface

_gen/tp-spec-%.xml: %.xml ../tools/xincludator.py _gen/spec-stamp
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

