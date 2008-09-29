# This directory is an example of how to build extensions to the spec.
# Typically this would be in a top-level extensions/ directory.

# In this example we build an optional interface for Telepathy Connections,
# so we specify Telepathy::Client::Connection as the main interface for the
# generated proxies with "--mainiface=Telepathy::Client::Connection'. The
# generated proxies will have a convenience constructors for associating the
# proxy with the same remote object an instance of the main interface class
# is associated with. We could instead have made an optional interface for any
# other class, or by leaving that option out entirely we could have made an
# extension that will work on QDBusAbstractInterface or any subclass of it.
#
# For stand-alone interfaces (for which the interface itself should be considered
# the main interface) --mainiface should be specified as fully namespaced name
# of the interface class itself.

tools_dir = $(top_srcdir)/tools

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    @QTCORE_CFLAGS@ \
    @QTDBUS_CFLAGS@ \
    $(TP_QT4_CXXFLAGS)

EXTRA_DIST = \
    all.xml \
    connection.xml \
    Connection_Interface_Hats.xml

noinst_LTLIBRARIES = libexample-extensions.la

libexample_extensions_la_LIBADD = \
    $(TP_QT4_LIBS)

libexample_extensions_la_SOURCES = \
    cli-connection.cpp \
    cli-connection.h \
    types.cpp \
    types.h

nodist_libexample_extensions_la_SOURCES = \
    _gen/constants.h \
    _gen/cli-connection.h \
    _gen/cli-connection-body.hpp \
    _gen/cli-connection.moc.hpp \
    _gen/types.h \
    _gen/types-body.hpp

BUILT_SOURCES = \
    _gen/all.xml \
    _gen/connection.xml \
    $(nodist_libexample_extensions_la_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

# Generated files which can be generated for all categories simultaneously

_gen/all.xml: all.xml $(wildcard *.xml)
	$(mkdir_p) _gen
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/constants.h: _gen/all.xml \
		$(top_srcdir)/tools/qt4-constants-gen.py
	$(PYTHON) $(top_srcdir)/tools/qt4-constants-gen.py \
		--namespace='Example' \
		--str-constant-prefix='EXAMPLE_' \
		--specxml=$< \
		> $@.tmp && mv $@.tmp $@

_gen/types.h _gen/types-body.hpp: _gen/all.xml \
		$(top_srcdir)/tools/qt4-types-gen.py
	$(PYTHON) $(top_srcdir)/tools/qt4-types-gen.py \
		--namespace='Example' \
		--declfile='_gen/types.h' \
		--implfile='_gen/types-body.hpp' \
		--realinclude='types.h' \
		--specxml=$< \
		--extraincludes='<TelepathyQt4/Types>'

# Things generated per interface group

_gen/connection.xml: connection.xml $(wildcard *.xml)
	$(mkdir_p) _gen
	$(PYTHON) $(top_srcdir)/tools/xincludator.py \
		$< > $@

_gen/cli-connection-body.hpp _gen/cli-connection.h: _gen/connection.xml \
	_gen/all.xml $(tools_dir)/qt4-client-gen.py
	$(PYTHON) $(tools_dir)/qt4-client-gen.py \
		--namespace='Example::Client' \
		--typesnamespace='Example' \
		--headerfile=_gen/cli-connection.h \
		--implfile=_gen/cli-connection-body.hpp \
		--realinclude='../cli-connection.h' \
		--specxml=_gen/all.xml \
		--ifacexml=$< \
		--extraincludes='<TelepathyQt4/Client/Connection>,"types.h"' \
		--mainiface='Telepathy::Client::ConnectionInterface'

%.moc.hpp: %.h _gen/constants.h _gen/types.h
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
