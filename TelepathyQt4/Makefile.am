include $(top_srcdir)/tools/telepathy-qt4.mk

DIST_SUBDIRS = Farsight
# We need to build this directory before the Farsight subdirectory
SUBDIRS = .

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
    channel-dispatcher.xml \
    channel-dispatch-operation.xml \
    channel-request.xml \
    client.xml \
    connection.xml \
    connection-manager.xml \
    dbus.xml \
    dbus-daemon.xml \
    dbus-introspectable.xml \
    dbus-peer.xml \
    dbus-properties.xml \
    examples.dox \
    future-channel.xml \
    future-interfaces.xml \
    future-misc.xml \
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
    -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
libtelepathy_qt4_la_LIBADD = $(ALL_LIBS)
libtelepathy_qt4_la_DEPENDENCIES = Makefile.am

libtelepathy_qt4_la_SOURCES = \
    abstract-client.cpp \
    abstract-interface.cpp \
    account.cpp \
    account-manager.cpp \
    capabilities-base.cpp \
    channel.cpp \
    channel-factory.cpp \
    channel-factory.h \
    channel-request.cpp \
    channel-dispatcher.cpp \
    channel-dispatch-operation.cpp \
    client.cpp \
    client-registrar.cpp \
    client-registrar-internal.h \
    connection.cpp \
    connection-capabilities.cpp \
    connection-manager.cpp \
    connection-internal.h \
    connection-manager-internal.h \
    contact.cpp \
    contact-capabilities.cpp \
    contact-manager.cpp \
    dbus.cpp \
    dbus-proxy.cpp \
    debug.cpp \
    debug-internal.h \
    feature.cpp \
    file-transfer-channel.cpp \
    file-transfer-channel-creation-properties.cpp \
    future.cpp \
    future-internal.h \
    incoming-file-transfer-channel.cpp \
    key-file.cpp \
    manager-file.cpp \
    media-session-handler.cpp \
    media-stream-handler.cpp \
    message.cpp \
    optional-interface-factory.cpp \
    outgoing-file-transfer-channel.cpp \
    pending-account.cpp \
    pending-channel.cpp \
    pending-channel-request.cpp \
    pending-channel-request-internal.h \
    pending-connection.cpp \
    pending-contact-attributes.cpp \
    pending-contacts.cpp \
    pending-handles.cpp \
    pending-operation.cpp \
    pending-ready.cpp \
    pending-string-list.cpp \
    pending-variant.cpp \
    properties.cpp \
    readiness-helper.cpp \
    ready-object.cpp \
    referenced-handles.cpp \
    room-list-channel.cpp \
    streamed-media-channel.cpp \
    streamed-media-channel-internal.h \
    text-channel.cpp \
    types.cpp

nodist_libtelepathy_qt4_la_SOURCES = \
    _gen/cli-account-body.hpp \
    _gen/cli-account.moc.hpp \
    _gen/cli-account-manager-body.hpp \
    _gen/cli-account-manager.moc.hpp \
    _gen/cli-channel-body.hpp \
    _gen/cli-channel.moc.hpp \
    _gen/cli-channel-dispatcher-body.hpp \
    _gen/cli-channel-dispatcher.moc.hpp \
    _gen/cli-channel-dispatch-operation-body.hpp \
    _gen/cli-channel-dispatch-operation.moc.hpp \
    _gen/cli-channel-request-body.hpp \
    _gen/cli-channel-request.moc.hpp \
    _gen/cli-client-body.hpp \
    _gen/cli-client.moc.hpp \
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
    _gen/channel-dispatch-operation.moc.hpp \
    _gen/channel-request.moc.hpp \
    _gen/client.moc.hpp \
    _gen/client-registrar.moc.hpp \
    _gen/client-registrar-internal.moc.hpp \
    _gen/connection.moc.hpp \
    _gen/connection-manager.moc.hpp \
    _gen/connection-internal.moc.hpp \
    _gen/connection-manager-internal.moc.hpp \
    _gen/contact.moc.hpp \
    _gen/contact-manager.moc.hpp \
    _gen/contact-manager-internal.moc.hpp \
    _gen/dbus-proxy.moc.hpp \
    _gen/file-transfer-channel.moc.hpp \
    _gen/incoming-file-transfer-channel.moc.hpp \
    _gen/outgoing-file-transfer-channel.moc.hpp \
    _gen/pending-account.moc.hpp \
    _gen/pending-channel.moc.hpp \
    _gen/pending-channel-request.moc.hpp \
    _gen/pending-channel-request-internal.moc.hpp \
    _gen/pending-connection.moc.hpp \
    _gen/pending-contact-attributes.moc.hpp \
    _gen/pending-contacts.moc.hpp \
    _gen/pending-handles.moc.hpp \
    _gen/pending-operation.moc.hpp \
    _gen/pending-ready.moc.hpp \
    _gen/pending-string-list.moc.hpp \
    _gen/pending-variant.moc.hpp \
    _gen/readiness-helper.moc.hpp \
    _gen/room-list-channel.moc.hpp \
    _gen/simple-pending-operations.moc.hpp \
    _gen/streamed-media-channel.moc.hpp \
    _gen/streamed-media-channel-internal.moc.hpp \
    _gen/text-channel.moc.hpp \
    _gen/future-channel.h \
    _gen/future-channel.moc.hpp \
    _gen/future-channel-body.hpp \
    _gen/future-constants.h \
    _gen/future-misc.h \
    _gen/future-misc.moc.hpp \
    _gen/future-misc-body.hpp \
    _gen/future-types.h \
    _gen/future-types-body.hpp

tpqt4include_HEADERS = \
    AbstractClient \
    AbstractClientApprover \
    AbstractClientHandler \
    AbstractClientObserver \
    AbstractInterface \
    Account \
    AccountManager \
    CapabilitiesBase \
    Channel \
    ChannelDispatcher \
    ChannelDispatchOperation \
    ChannelRequest \
    Client \
    ClientRegistrar \
    Connection \
    ConnectionCapabilities \
    ConnectionManager \
    Constants \
    Contact \
    ContactCapabilities \
    ContactManager \
    Debug \
    DBus \
    DBusProxy \
    Feature \
    FileTransferChannel \
    FileTransferChannelCreationProperties \
    Global \
    IncomingFileTransferChannel \
    KeyFile \
    ManagerFile \
    MediaSessionHandler \
    MediaStreamHandler \
    Message \
    MethodInvocationContext \
    OptionalInterfaceFactory \
    OutgoingFileTransferChannel \
    PendingAccount \
    PendingChannel \
    PendingChannelRequest \
    PendingComposite \
    PendingConnection \
    PendingContacts \
    PendingContactAttributes \
    PendingFailure \
    PendingHandles \
    PendingOperation \
    PendingReady \
    PendingSuccess \
    PendingStringList \
    PendingVariant \
    PendingVoid \
    Properties \
    ReadinessHelper \
    ReadyObject \
    ReceivedMessage \
    RefCounted \
    ReferencedHandles \
    ReferencedHandlesIterator \
    RoomListChannel \
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
    ChannelDispatcherInterface \
    ChannelDispatchOperationInterface \
    ChannelRequestInterface \
    ChannelTypeContactListInterface \
    ChannelTypeRoomListInterface \
    ChannelTypeStreamedMediaInterface \
    ChannelTypeTextInterface \
    ChannelTypeTubesInterface \
    ChannelTypeTubeInterface \
    ChannelTypeStreamTubeInterface \
    ClientHandlerInterface \
    ClientApproverInterface \
    ClientObserverInterface \
    ClientInterface \
    ClientInterfaceRequestsInterface \
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
    abstract-client.h \
    abstract-interface.h \
    account.h \
    account-manager.h \
    capabilities-base.h \
    channel.h \
    channel-dispatcher.h \
    channel-dispatch-operation.h \
    channel-request.h \
    client.h \
    client-registrar.h \
    connection.h \
    connection-capabilities.h \
    connection-manager.h \
    constants.h \
    contact.h \
    contact-capabilities.h \
    contact-manager.h \
    contact-manager-internal.h \
    dbus.h \
    dbus-proxy.h \
    debug.h \
    feature.h \
    file-transfer-channel.h \
    file-transfer-channel-creation-properties.h \
    global.h \
    incoming-file-transfer-channel.h \
    key-file.h \
    manager-file.h \
    media-session-handler.h \
    media-stream-handler.h \
    message.h \
    method-invocation-context.h \
    optional-interface-factory.h \
    outgoing-file-transfer-channel.h \
    pending-account.h \
    pending-channel.h \
    pending-channel-request.h \
    pending-connection.h \
    pending-contact-attributes.h \
    pending-contacts.h \
    pending-handles.h \
    pending-operation.h \
    pending-ready.h \
    pending-string-list.h \
    pending-variant.h \
    properties.h \
    readiness-helper.h \
    ready-object.h \
    referenced-handles.h \
    room-list-channel.h \
    shared-ptr.h \
    simple-pending-operations.h \
    streamed-media-channel.h \
    streamed-media-channel-internal.h \
    text-channel.h \
    types.h

nodist_geninclude_HEADERS = \
    _gen/cli-account.h \
    _gen/cli-account-manager.h \
    _gen/cli-channel.h \
    _gen/cli-channel-dispatcher.h \
    _gen/cli-channel-dispatch-operation.h \
    _gen/cli-channel-request.h \
    _gen/cli-client.h \
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
    -DBUILDING_TELEPATHY_QT4 \
    -I$(top_srcdir) -I$(top_builddir) \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTNETWORK_CFLAGS)

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(VISIBILITY_HIDDEN_FLAGS)

ALL_LIBS = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTNETWORK_LIBS)

# Generated stuff

# Bootstrapping

_gen/spec-stamp: $(wildcard $(top_srcdir)/spec/*.xml)
	$(AM_V_at)$(mkdir_p) _gen
	$(AM_V_GEN)touch $@

_gen/stable-stamp: $(wildcard *.xml) _gen/spec-stamp
	$(AM_V_GEN)touch $@

_gen/stable-spec.xml: $(srcdir)/stable-interfaces.xml _gen/stable-stamp \
	$(top_srcdir)/tools/xincludator.py \
	Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/future-stamp: $(srcdir)/future-interfaces.xml
	$(AM_V_at)$(mkdir_p) _gen
	$(AM_V_GEN)touch $@

_gen/future-spec.xml: $(srcdir)/future-interfaces.xml _gen/future-stamp \
	$(top_srcdir)/tools/xincludator.py \
	Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

# Things generated from the whole spec at once

_gen/constants.h: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-constants-gen.py \
		Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='Tp' \
		--str-constant-prefix='TELEPATHY_' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-types-gen.py \
		Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='Tp' \
		--declfile='_gen/types.h' \
		--implfile='_gen/types-body.hpp' \
		--realinclude='TelepathyQt4/types.h' \
		--prettyinclude='TelepathyQt4/Types' \
		--must-define='IN_TELEPATHY_QT4_HEADER' \
		--visibility=TELEPATHY_QT4_EXPORT \
		--specxml=$<

_gen/future-constants.h: _gen/future-spec.xml \
		$(top_srcdir)/tools/qt4-constants-gen.py \
		Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='TpFuture' \
		--str-constant-prefix='TP_FUTURE_' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/future-types.h _gen/future-types-body.hpp: _gen/future-spec.xml \
		$(top_srcdir)/tools/qt4-types-gen.py \
		Makefile.am
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='TpFuture' \
		--declfile='_gen/future-types.h' \
		--implfile='_gen/future-types-body.hpp' \
		--realinclude='TelepathyQt4/future-internal.h' \
		--prettyinclude='TelepathyQt4/future-internal.h' \
		--specxml=$<

# Things generated per interface group

_gen/spec-%.xml: %.xml $(top_srcdir)/tools/xincludator.py _gen/spec-stamp
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/cli-%.h _gen/cli-%-body.hpp: _gen/spec-%.xml \
		_gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-client-gen.py \
		Makefile.am
	$(AM_V_GEN)set -e; \
	namespace='Tp::Client'; \
	group= ; \
	prettyinclude= ; \
	mainiface= ; \
	case $* in \
		channel) \
			group='clientchannel'; \
			prettyinclude='Channel'; \
			mainiface='--mainiface=Tp::Client::ChannelInterface';; \
		channel-dispatcher) \
			group='clientchanneldispatcher'; \
			prettyinclude='ChannelDispatcher'; \
			mainiface='--mainiface=Tp::Client::ChannelDispatcherInterface';; \
		channel-dispatch-operation) \
			group='clientchanneldispatchoperation'; \
			prettyinclude='ChannelDispatchOperation'; \
			mainiface='--mainiface=Tp::Client::ChannelDispatchOperationInterface';; \
		channel-request) \
			group='clientchannelrequest'; \
			prettyinclude='ChannelRequest'; \
			mainiface='--mainiface=Tp::Client::ChannelRequestInterface';; \
		client) \
			group='clientclient'; \
			prettyinclude='Client'; \
			mainiface='--mainiface=Tp::Client::ClientInterface';; \
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
		--visibility=TELEPATHY_QT4_EXPORT \
		$$mainiface

_gen/future-%.xml: $(srcdir)/future-%.xml \
	$(top_srcdir)/tools/xincludator.py _gen/future-stamp
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/future-%.h _gen/future-%-body.hpp: _gen/future-%.xml \
		_gen/future-spec.xml \
		$(top_srcdir)/tools/qt4-client-gen.py \
		Makefile.am
	$(AM_V_GEN)set -e; \
	namespace='TpFuture::Client'; \
	mainiface= ; \
	case $* in \
		channel) \
			mainiface='--mainiface=Tp::Client::ChannelInterface';; \
	esac; \
	$(PYTHON) $(top_srcdir)/tools/qt4-client-gen.py \
		--namespace=$$namespace \
		--typesnamespace='TpFuture' \
		--headerfile=_gen/future-$*.h \
		--implfile=_gen/future-$*-body.hpp \
		--realinclude='TelepathyQt4/future-internal.h' \
		--prettyinclude='TelepathyQt4/future-internal.h' \
		--specxml=_gen/future-spec.xml \
		--ifacexml=$< \
		--extraincludes='<TelepathyQt4/Types>' \
		--extraincludes='<TelepathyQt4/future-internal.h>' \
		--visibility=TELEPATHY_QT4_NO_EXPORT \
		$$mainiface

_gen/%.moc.hpp: %.h _gen/constants.h _gen/types.h
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@

_gen/cli-%.moc.hpp: _gen/cli-%.h _gen/constants.h _gen/types.h
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@

_gen/future-%.moc.hpp: _gen/future-%.h \
	_gen/future-constants.h \
	_gen/future-types.h
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@
