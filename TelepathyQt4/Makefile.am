tpqt4includedir=$(includedir)/telepathy-1.0/TelepathyQt4
tpqt4clientincludedir=$(tpqt4includedir)/Client
genincludedir=$(tpqt4includedir)/_gen

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = TelepathyQt4.pc

EXTRA_DIST = \
    channel.xml \
    connection.xml \
    connection-manager.xml \
    dbus.xml \
    dbus-introspectable.xml \
    dbus-peer.xml \
    dbus-properties.xml \
    media-session-handler.xml \
    media-stream-handler.xml \
    properties.xml \
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
    cli-channel.cpp \
    cli-connection.cpp \
    cli-connection-manager.cpp \
    cli-dbus.cpp \
    cli-media-session-handler.cpp \
    cli-media-stream-handler.cpp \
    cli-properties.cpp \
    debug.cpp \
    debug-internal.hpp \
    types.cpp \
    header-compile-test.cpp

nodist_libtelepathy_qt4_la_SOURCES = \
    _gen/cli-channel-body.hpp \
    _gen/cli-channel.moc.hpp \
    _gen/cli-connection-body.hpp \
    _gen/cli-connection.moc.hpp \
    _gen/cli-connection-manager-body.hpp \
    _gen/cli-connection-manager.moc.hpp \
    _gen/cli-dbus-body.hpp \
    _gen/cli-dbus.moc.hpp \
    _gen/cli-media-session-handler-body.hpp \
    _gen/cli-media-session-handler.moc.hpp \
    _gen/cli-media-stream-handler-body.hpp \
    _gen/cli-media-stream-handler.moc.hpp \
    _gen/cli-properties-body.hpp \
    _gen/cli-properties.moc.hpp \
    _gen/types-body.hpp

tpqt4include_HEADERS = \
    Constants \
    Debug \
    Types \
    cli-channel.h \
    cli-connection.h \
    cli-connection-manager.h \
    cli-dbus.h \
    cli-media-session-handler.h \
    cli-media-stream-handler.h \
    cli-properties.h \
    constants.h \
    debug.h \
    types.h

tpqt4clientinclude_HEADERS = \
    Client/Channel \
    Client/Connection \
    Client/ConnectionManager \
    Client/DBus \
    Client/MediaSessionHandler \
    Client/MediaStreamHandler \
    Client/Properties

nodist_geninclude_HEADERS = \
    _gen/cli-channel.h \
    _gen/cli-connection.h \
    _gen/cli-connection-manager.h \
    _gen/cli-dbus.h \
    _gen/cli-media-session-handler.h \
    _gen/cli-media-stream-handler.h \
    _gen/cli-properties.h \
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
		--str-constant-prefix='TELEPATHY_' \
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

_gen/spec-%.xml: %.xml ../tools/xincludator.py _gen/spec-stamp
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/cli-%.h _gen/cli-%-body.hpp: _gen/spec-%.xml \
		_gen/stable-spec.xml \
		../tools/qt4-client-gen.py
	set -e; \
	namespace='Telepathy::Client'; \
	group= ; \
	prettyinclude= ; \
	mainiface= ; \
	case $* in \
		channel) \
			group='clientchannel'; \
			prettyinclude='Channel'; \
			mainiface='--mainiface=/Channel';; \
		connection) \
			group='clientconn'; \
			prettyinclude='Connection'; \
			mainiface='--mainiface=/Connection';; \
		connection-manager) \
			group='clientcm'; \
			prettyinclude='ConnectionManager'; \
			mainiface='--mainiface=/Connection_Manager';; \
		dbus) \
			group='clientdbus'; \
			namespace='Telepathy::Client::DBus'; \
			prettyinclude='DBus';; \
		media-session-handler) \
			group='clientmsesh'; \
			prettyinclude='MediaSessionHandler'; \
			mainiface='--mainiface=/Media_Session_Handler';; \
		media-stream-handler) \
			group='clientmstrh'; \
			prettyinclude='MediaStreamHandler'; \
			mainiface='--mainiface=/Media_Stream_Handler';; \
		properties) \
			group='clientprops'; \
			prettyinclude='Properties';; \
	esac; \
	$(PYTHON) $(srcdir)/../tools/qt4-client-gen.py \
		--group=$$group \
		--namespace=$$namespace \
		--typesnamespace='Telepathy' \
		--headerfile=_gen/cli-$*.h \
		--implfile=_gen/cli-$*-body.hpp \
		--realinclude='TelepathyQt4/cli-$*.h' \
		--prettyinclude='TelepathyQt4/Client/'$$prettyinclude \
		--specxml=_gen/stable-spec.xml \
		--ifacexml=$< \
		--typesinclude='TelepathyQt4/Types' \
		$$mainiface

%.moc.hpp: %.h _gen/constants.h _gen/types.h
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
