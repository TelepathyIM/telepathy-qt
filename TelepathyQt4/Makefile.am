# We need to build this directory before the Prototype subdirectory
SUBDIRS = . Prototype

tpqt4includedir=$(includedir)/telepathy-1.0/TelepathyQt4
tpqt4clientincludedir=$(tpqt4includedir)/Client
genincludedir=$(tpqt4includedir)/_gen

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = TelepathyQt4.pc

EXTRA_DIST = \
    account.xml \
    account-manager.xml \
    channel.xml \
    connection.xml \
    connection-manager.xml \
    dbus.xml \
    dbus-daemon.xml \
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
    -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)" -static
libtelepathy_qt4_la_LIBADD = $(ALL_LIBS)
libtelepathy_qt4_la_DEPENDENCIES = Makefile.am

libtelepathy_qt4_la_SOURCES = \
    Client/account.cpp \
    Client/account-manager.cpp \
    Client/account-manager-internal.h \
    Client/channel.cpp \
    Client/connection.cpp \
    Client/connection-manager.cpp \
    Client/connection-manager-internal.h \
    Client/dbus.cpp \
    Client/dbus-proxy.cpp \
    Client/media-session-handler.cpp \
    Client/media-stream-handler.cpp \
    Client/optional-interface-factory.cpp \
    Client/pending-channel.cpp \
    Client/pending-handles.cpp \
    Client/pending-operation.cpp \
    Client/pending-string-list.cpp \
    Client/properties.cpp \
    Client/referenced-handles.cpp \
    debug.cpp \
    debug-internal.h \
    key-file.cpp \
    manager-file.cpp \
    types.cpp \
    header-compile-test.cpp

nodist_libtelepathy_qt4_la_SOURCES = \
    _gen/cli-account-body.hpp \
    _gen/cli-account.moc.hpp \
    _gen/cli-account-manager-body.hpp \
    _gen/cli-account-manager.moc.hpp \
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
    _gen/types-body.hpp \
    Client/_gen/account-manager.moc.hpp \
    Client/_gen/account-manager-internal.moc.hpp \
    Client/_gen/channel.moc.hpp \
    Client/_gen/connection.moc.hpp \
    Client/_gen/connection-manager.moc.hpp \
    Client/_gen/connection-manager-internal.moc.hpp \
    Client/_gen/dbus-proxy.moc.hpp \
    Client/_gen/pending-channel.moc.hpp \
    Client/_gen/pending-handles.moc.hpp \
    Client/_gen/pending-operation.moc.hpp \
    Client/_gen/pending-string-list.moc.hpp \
    Client/_gen/simple-pending-operations.moc.hpp

tpqt4include_HEADERS = \
    Constants \
    Debug \
    KeyFile \
    ManagerFile \
    Types \
    constants.h \
    debug.h \
    key-file.h \
    manager-file.h \
    types.h

tpqt4clientinclude_HEADERS = \
    Client/Account \
    Client/AccountManager \
    Client/Channel \
    Client/Connection \
    Client/ConnectionManager \
    Client/DBus \
    Client/DBusProxy \
    Client/MediaSessionHandler \
    Client/MediaStreamHandler \
    Client/OptionalInterfaceFactory \
    Client/PendingChannel \
    Client/PendingFailure \
    Client/PendingOperation \
    Client/PendingSuccess \
    Client/PendingStringList \
    Client/PendingVoidMethodCall \
    Client/Properties \
    Client/ReferencedHandles \
    Client/ReferencedHandlesIterator \
    Client/StatelessDBusProxy \
    Client/StatefulDBusProxy \
    Client/AccountInterface \
    Client/AccountManagerInterface \
    Client/ChannelInterface \
    Client/ChannelInterfaceCallStateInterface \
    Client/ChannelInterfaceChatStateInterface \
    Client/ChannelInterfaceDTMFInterface \
    Client/ChannelInterfaceGroupInterface \
    Client/ChannelInterfaceHoldInterface \
    Client/ChannelInterfaceMediaSignallingInterface \
    Client/ChannelInterfacePasswordInterface \
    Client/ChannelTypeContactListInterface \
    Client/ChannelTypeRoomListInterface \
    Client/ChannelTypeStreamedMediaInterface \
    Client/ChannelTypeTextInterface \
    Client/ChannelTypeTubesInterface \
    Client/ConnectionInterface \
    Client/ConnectionInterfaceAliasingInterface \
    Client/ConnectionInterfaceAvatarsInterface \
    Client/ConnectionInterfaceCapabilitiesInterface \
    Client/ConnectionInterfaceContactsInterface \
    Client/ConnectionInterfacePresenceInterface \
    Client/ConnectionInterfaceRequestsInterface \
    Client/ConnectionInterfaceSimplePresenceInterface \
    Client/ConnectionManagerInterface \
    Client/DBusDaemonInterface \
    Client/IntrospectableInterface \
    Client/MediaSessionHandlerInterface \
    Client/MediaStreamHandlerInterface \
    Client/PeerInterface \
    Client/PropertiesInterface \
    Client/PropertiesInterfaceInterface \
    Client/account.h \
    Client/account-manager.h \
    Client/channel.h \
    Client/connection.h \
    Client/connection-manager.h \
    Client/dbus.h \
    Client/dbus-proxy.h \
    Client/media-session-handler.h \
    Client/media-stream-handler.h \
    Client/optional-interface-factory.h \
    Client/pending-channel.h \
    Client/pending-handles.h \
    Client/pending-operation.h \
    Client/pending-string-list.h \
    Client/properties.h \
    Client/referenced-handles.h \
    Client/simple-pending-operations.h

nodist_geninclude_HEADERS = \
    _gen/cli-account.h \
    _gen/cli-account-manager.h \
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

_gen/stable-spec.xml: $(srcdir)/stable-interfaces.xml _gen/stable-stamp \
	$(top_srcdir)/tools/xincludator.py \
	Makefile.am
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

# Things generated from the whole spec at once

_gen/constants.h: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-constants-gen.py \
		Makefile.am
	$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='Telepathy' \
		--str-constant-prefix='TELEPATHY_' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-types-gen.py \
		Makefile.am
	$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='Telepathy' \
		--declfile='_gen/types.h' \
		--implfile='_gen/types-body.hpp' \
		--realinclude='TelepathyQt4/types.h' \
		--prettyinclude='TelepathyQt4/Types' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		--specxml=$<

# Things generated per interface group

_gen/spec-%.xml: %.xml $(top_srcdir)/tools/xincludator.py _gen/spec-stamp
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/cli-%.h _gen/cli-%-body.hpp: _gen/spec-%.xml \
		_gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-client-gen.py \
		Makefile.am
	set -e; \
	namespace='Telepathy::Client'; \
	group= ; \
	prettyinclude= ; \
	mainiface= ; \
	case $* in \
		channel) \
			group='clientchannel'; \
			prettyinclude='Channel'; \
			mainiface='--mainiface=Telepathy::Client::ChannelInterface';; \
		account) \
			group='clientaccount'; \
			prettyinclude='AccountManager'; \
			mainiface='--mainiface=Telepathy::Client::AccountInterface';; \
		account-manager) \
			group='clientam'; \
			prettyinclude='AccountManager'; \
			mainiface='--mainiface=Telepathy::Client::AccountManagerInterface';; \
		connection) \
			group='clientconn'; \
			prettyinclude='Connection'; \
			mainiface='--mainiface=Telepathy::Client::ConnectionInterface';; \
		connection-manager) \
			group='clientcm'; \
			prettyinclude='ConnectionManager'; \
			mainiface='--mainiface=Telepathy::Client::ConnectionManagerInterface';; \
		dbus) \
			group='clientdbus'; \
			namespace='Telepathy::Client::DBus'; \
			prettyinclude='DBus';; \
		media-session-handler) \
			group='clientmsesh'; \
			prettyinclude='MediaSessionHandler'; \
			mainiface='--mainiface=Telepathy::Client::MediaSessionHandlerInterface';; \
		media-stream-handler) \
			group='clientmstrh'; \
			prettyinclude='MediaStreamHandler'; \
			mainiface='--mainiface=Telepathy::Client::MediaStreamHandlerInterface';; \
		properties) \
			group='clientprops'; \
			prettyinclude='Properties';; \
	esac; \
	$(PYTHON) $(top_srcdir)/tools/qt4-client-gen.py \
		--group=$$group \
		--namespace=$$namespace \
		--typesnamespace='Telepathy' \
		--headerfile=_gen/cli-$*.h \
		--implfile=_gen/cli-$*-body.hpp \
		--realinclude='TelepathyQt4/Client/$*.h' \
		--prettyinclude='TelepathyQt4/Client/'$$prettyinclude \
		--specxml=_gen/stable-spec.xml \
		--ifacexml=$< \
		--extraincludes='<TelepathyQt4/Types>' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		$$mainiface

Client/_gen/%.moc.hpp: $(srcdir)/Client/%.h _gen/constants.h _gen/types.h
	$(mkdir_p) Client/_gen
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@

_gen/%.moc.hpp: _gen/%.h _gen/constants.h _gen/types.h
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
