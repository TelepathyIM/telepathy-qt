protoincludedir=$(includedir)/tpqt4-prototype/TelepathyQt4/Prototype
protoclientincludedir=$(protoincludedir)/Client
genincludedir=$(protoincludedir)/_gen

pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = TpQt4Prototype.pc

EXTRA_DIST = \
    all.xml \
    channel-handler.xml

lib_LTLIBRARIES = libtpqt4-prototype.la

# The quoting here is unnecessary but harmless, and has the useful side-effect
# that vim quickfix mode (:make) doesn't interpret the libtool --mode=link
# command as an error message in a bizarrely named file
libtpqt4_prototype_la_LDFLAGS = \
    -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)" -static
libtpqt4_prototype_la_LIBADD = $(ALL_LIBS)
libtpqt4_prototype_la_DEPENDENCIES = Makefile.am

libtpqt4_prototype_la_SOURCES = \
    Account.cpp \
    AccountManager.cpp \
    AvatarManager.cpp \
    CapabilitiesManager.cpp \
    ChatChannel.cpp \
    Connection.cpp \
    ConnectionFacade.cpp \
    Contact.cpp \
    ContactManager.cpp \
    DBusInterface.cpp \
    PresenceManager.cpp \
    StreamedMediaChannel.cpp \
    cli-channel-handler.cpp \
    types.cpp

nodist_libtpqt4_prototype_la_SOURCES = \
    _gen/Account.h.moc \
    _gen/AccountManager.h.moc \
    _gen/AvatarManager.h.moc \
    _gen/CapabilitiesManager.h.moc \
    _gen/ChatChannel.h.moc \
    _gen/Connection.h.moc \
    _gen/ConnectionFacade.h.moc \
    _gen/Contact.h.moc \
    _gen/ContactManager.h.moc \
    _gen/DBusInterface.h.moc \
    _gen/PresenceManager.h.moc \
    _gen/StreamedMediaChannel.h.moc \
    _gen/cli-channel-handler-body.hpp \
    _gen/cli-channel-handler.moc.hpp \
    _gen/types-body.hpp

protoinclude_HEADERS = \
    Account.h \
    AccountManager.h \
    AvatarManager.h \
    CapabilitiesManager.h \
    ChatChannel.h \
    Connection.h \
    ConnectionFacade.h \
    Constants \
    Contact.h \
    ContactManager.h \
    DBusInterface.h \
    PresenceManager.h \
    StreamedMediaChannel.h \
    Types \
    constants.h \
    cli-channel-handler.h \
    types.h

protoclientinclude_HEADERS = \
    Client/ChannelHandler

nodist_geninclude_HEADERS = \
    _gen/cli-channel-handler.h \
    _gen/constants.h \
    _gen/types.h

BUILT_SOURCES = \
    $(nodist_libtpqt4_prototype_la_SOURCES) \
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
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(TP_QT4_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)

ALL_LIBS = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(TP_QT4_LIBS)

# Generated stuff

# Bootstrapping

_gen/spec-stamp: $(wildcard $(top_srcdir)/spec/*.xml)
	$(mkdir_p) _gen
	touch $@

_gen/stable-stamp: $(wildcard *.xml) _gen/spec-stamp
	touch $@

_gen/stable-spec.xml: all.xml _gen/stable-stamp \
	$(top_srcdir)/tools/xincludator.py \
	Makefile.am
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

# Things generated from the whole spec at once

_gen/constants.h: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-constants-gen.py \
		Makefile.am
	$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='TpPrototype' \
		--str-constant-prefix='TP_PROTOTYPE_' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/stable-spec.xml \
		$(top_srcdir)/tools/qt4-types-gen.py \
		Makefile.am
	$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='TpPrototype' \
		--declfile='_gen/types.h' \
		--implfile='_gen/types-body.hpp' \
		--realinclude='TelepathyQt4/Prototype/types.h' \
		--prettyinclude='TelepathyQt4/Prototype/Types' \
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
	namespace='TpPrototype::Client'; \
	group= ; \
	prettyinclude= ; \
	mainiface= ; \
	case $* in \
		channel-handler) \
			group='clientchannelhandler'; \
			prettyinclude='ChannelHandler';; \
	esac; \
	$(PYTHON) $(top_srcdir)/tools/qt4-client-gen.py \
		--group=$$group \
		--namespace=$$namespace \
		--typesnamespace='Telepathy' \
		--headerfile=_gen/cli-$*.h \
		--implfile=_gen/cli-$*-body.hpp \
		--realinclude='TelepathyQt4/Prototype/cli-$*.h' \
		--prettyinclude='TelepathyQt4/Prototype/Client/'$$prettyinclude \
		--specxml=_gen/stable-spec.xml \
		--ifacexml=$< \
		--extraincludes='<TelepathyQt4/Types>,<TelepathyQt4/Prototype/Types>' \
		$$mainiface

_gen/%.moc: % _gen/constants.h _gen/types.h
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@

_gen/%.moc.hpp: _gen/%.h _gen/constants.h _gen/types.h
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
