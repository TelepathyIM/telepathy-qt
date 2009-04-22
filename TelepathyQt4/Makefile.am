# We need to build this directory before the Prototype subdirectory
DIST_SUBDIRS = Prototype Farsight
SUBDIRS = . Prototype

tpqt4includedir=$(includedir)/telepathy-1.0/TelepathyQt4
genincludedir=$(tpqt4includedir)/_gen

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = TelepathyQt4.pc

if ENABLE_TP_FARSIGHT
SUBDIRS += Farsight
pkgconfig_DATA += TelepathyQt4Farsight.pc
endif

EXTRA_DIST = \
    account.xml \
    account-manager.xml \
    async-model.dox \
    channel.xml \
    connection.xml \
    connection-manager.xml \
    dbus.xml \
    dbus-daemon.xml \
    dbus-introspectable.xml \
    dbus-peer.xml \
    dbus-properties.xml \
    examples.dox \
    main.dox \
    media-session-handler.xml \
    media-stream-handler.xml \
    properties.xml \
    shared-ptr.dox \
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
    abstract-interface.cpp \
    account.cpp \
    account-manager.cpp \
    channel.cpp \
    connection.cpp \
    connection-manager.cpp \
    connection-internal.h \
    connection-manager-internal.h \
    contact.cpp \
    contact-manager.cpp \
    dbus.cpp \
    dbus-proxy.cpp \
    debug.cpp \
    debug-internal.h \
    feature.cpp \
    file-transfer.cpp \
    key-file.cpp \
    manager-file.cpp \
    media-session-handler.cpp \
    media-stream-handler.cpp \
    message.cpp \
    optional-interface-factory.cpp \
    pending-account.cpp \
    pending-channel.cpp \
    pending-connection.cpp \
    pending-contact-attributes.cpp \
    pending-contacts.cpp \
    pending-handles.cpp \
    pending-operation.cpp \
    pending-ready.cpp \
    pending-string-list.cpp \
    properties.cpp \
    readiness-helper.cpp \
    ready-object.cpp \
    referenced-handles.cpp \
    room-list.cpp \
    streamed-media-channel.cpp \
    text-channel.cpp \
    types.cpp

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
    _gen/abstract-interface.moc.hpp \
    _gen/account.moc.hpp \
    _gen/account-manager.moc.hpp \
    _gen/channel.moc.hpp \
    _gen/connection.moc.hpp \
    _gen/connection-manager.moc.hpp \
    _gen/connection-internal.moc.hpp \
    _gen/connection-manager-internal.moc.hpp \
    _gen/contact.moc.hpp \
    _gen/contact-manager.moc.hpp \
    _gen/dbus-proxy.moc.hpp \
    _gen/file-transfer.moc.hpp \
    _gen/pending-account.moc.hpp \
    _gen/pending-channel.moc.hpp \
    _gen/pending-connection.moc.hpp \
    _gen/pending-contact-attributes.moc.hpp \
    _gen/pending-contacts.moc.hpp \
    _gen/pending-handles.moc.hpp \
    _gen/pending-operation.moc.hpp \
    _gen/pending-ready.moc.hpp \
    _gen/pending-string-list.moc.hpp \
    _gen/readiness-helper.moc.hpp \
    _gen/room-list.moc.hpp \
    _gen/simple-pending-operations.moc.hpp \
    _gen/streamed-media-channel.moc.hpp \
    _gen/text-channel.moc.hpp

tpqt4include_HEADERS = \
    AbstractInterface \
    Account \
    AccountManager \
    Channel \
    Connection \
    ConnectionManager \
    Constants \
    Contact \
    ContactManager \
    Debug \
    DBus \
    DBusProxy \
    Feature \
    FileTransfer \
    KeyFile \
    ManagerFile \
    MediaSessionHandler \
    MediaStreamHandler \
    Message \
    OptionalInterfaceFactory \
    PendingAccount \
    PendingChannel \
    PendingConnection \
    PendingContacts \
    PendingContactAttributes \
    PendingFailure \
    PendingHandles \
    PendingOperation \
    PendingReady \
    PendingSuccess \
    PendingStringList \
    PendingVoidMethodCall \
    Properties \
    ReadinessHelper \
    ReadyObject \
    ReceivedMessage \
    ReferencedHandles \
    ReferencedHandlesIterator \
    RoomList \
    SharedData \
    SharedPtr \
    StatelessDBusProxy \
    StatefulDBusProxy \
    StreamedMediaChannel \
    TextChannel \
    Types \
    WeakPtr \
    AccountInterface \
    AccountManagerInterface \
    ChannelInterface \
    ChannelInterfaceCallStateInterface \
    ChannelInterfaceChatStateInterface \
    ChannelInterfaceDTMFInterface \
    ChannelInterfaceGroupInterface \
    ChannelInterfaceHoldInterface \
    ChannelInterfaceMediaSignallingInterface \
    ChannelInterfacePasswordInterface \
    ChannelTypeContactListInterface \
    ChannelTypeRoomListInterface \
    ChannelTypeStreamedMediaInterface \
    ChannelTypeTextInterface \
    ChannelTypeTubesInterface \
    ConnectionInterface \
    ConnectionInterfaceAliasingInterface \
    ConnectionInterfaceAvatarsInterface \
    ConnectionInterfaceCapabilitiesInterface \
    ConnectionInterfaceContactsInterface \
    ConnectionInterfacePresenceInterface \
    ConnectionInterfaceRequestsInterface \
    ConnectionInterfaceSimplePresenceInterface \
    ConnectionManagerInterface \
    DBusDaemonInterface \
    IntrospectableInterface \
    MediaSessionHandlerInterface \
    MediaStreamHandlerInterface \
    PeerInterface \
    PropertiesInterface \
    PropertiesInterfaceInterface \
    abstract-interface.h \
    account.h \
    account-manager.h \
    channel.h \
    connection.h \
    connection-manager.h \
    constants.h \
    contact.h \
    contact-manager.h \
    dbus.h \
    dbus-proxy.h \
    debug.h \
    feature.h \
    file-transfer.h \
    key-file.h \
    manager-file.h \
    media-session-handler.h \
    media-stream-handler.h \
    message.h \
    optional-interface-factory.h \
    pending-account.h \
    pending-channel.h \
    pending-connection.h \
    pending-contact-attributes.h \
    pending-contacts.h \
    pending-handles.h \
    pending-operation.h \
    pending-ready.h \
    pending-string-list.h \
    properties.h \
    readiness-helper.h \
    ready-object.h \
    referenced-handles.h \
    room-list.h \
    shared-ptr.h \
    simple-pending-operations.h \
    streamed-media-channel.h \
    text-channel.h \
    types.h

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

AM_CPPFLAGS = \
    -I$(top_srcdir) -I$(top_builddir) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS)

ALL_LIBS = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS)

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
		--namespace='Tp' \
		--str-constant-prefix='TELEPATHY_' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-types-gen.py \
		Makefile.am
	$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='Tp' \
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
	namespace='Tp::Client'; \
	group= ; \
	prettyinclude= ; \
	mainiface= ; \
	case $* in \
		channel) \
			group='clientchannel'; \
			prettyinclude='Channel'; \
			mainiface='--mainiface=Tp::Client::ChannelInterface';; \
		account) \
			group='clientaccount'; \
			prettyinclude='AccountManager'; \
			mainiface='--mainiface=Tp::Client::AccountInterface';; \
		account-manager) \
			group='clientam'; \
			prettyinclude='AccountManager'; \
			mainiface='--mainiface=Tp::Client::AccountManagerInterface';; \
		connection) \
			group='clientconn'; \
			prettyinclude='Connection'; \
			mainiface='--mainiface=Tp::Client::ConnectionInterface';; \
		connection-manager) \
			group='clientcm'; \
			prettyinclude='ConnectionManager'; \
			mainiface='--mainiface=Tp::Client::ConnectionManagerInterface';; \
		dbus) \
			group='clientdbus'; \
			namespace='Tp::Client::DBus'; \
			prettyinclude='DBus';; \
		media-session-handler) \
			group='clientmsesh'; \
			prettyinclude='MediaSessionHandler'; \
			mainiface='--mainiface=Tp::Client::MediaSessionHandlerInterface';; \
		media-stream-handler) \
			group='clientmstrh'; \
			prettyinclude='MediaStreamHandler'; \
			mainiface='--mainiface=Tp::Client::MediaStreamHandlerInterface';; \
		properties) \
			group='clientprops'; \
			prettyinclude='Properties';; \
	esac; \
	$(PYTHON) $(top_srcdir)/tools/qt4-client-gen.py \
		--group=$$group \
		--namespace=$$namespace \
		--typesnamespace='Tp' \
		--headerfile=_gen/cli-$*.h \
		--implfile=_gen/cli-$*-body.hpp \
		--realinclude='TelepathyQt4/$*.h' \
		--prettyinclude='TelepathyQt4/'$$prettyinclude \
		--specxml=_gen/stable-spec.xml \
		--ifacexml=$< \
		--extraincludes='<TelepathyQt4/Types>' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		$$mainiface

_gen/%.moc.hpp: %.h _gen/constants.h _gen/types.h
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@

_gen/cli-%.moc.hpp: _gen/cli-%.h _gen/constants.h _gen/types.h
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@
