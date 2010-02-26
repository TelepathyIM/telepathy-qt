# This directory re-uses telepathy-glib's code generation mechanisms to
# generate code for interfaces that aren't stable enough for telepathy-glib
# yet, so we can start to adapt example code to use them. Some of that example
# code is used by telepathy-qt4's regression tests.

tools_dir = $(top_srcdir)/tools

AM_CFLAGS = \
    $(ERROR_CFLAGS) \
    $(DBUS_CFLAGS) \
    $(GLIB_CFLAGS) \
    $(TP_GLIB_CFLAGS)

EXTRA_DIST = \
    all.xml \
    channel.xml \
    misc.xml

if HAVE_TP_GLIB

noinst_LTLIBRARIES = libtp-glib-tests-future-extensions.la

libtp_glib_tests_future_extensions_la_LIBADD = \
    $(GLIB_LIBS) \
    $(DBUS_LIBS) \
    $(TP_GLIB_LIBS)

libtp_glib_tests_future_extensions_la_SOURCES = \
    extensions.c \
    extensions.h

nodist_libtp_glib_tests_future_extensions_la_SOURCES = \
    _gen/signals-marshal.c \
    _gen/signals-marshal.h \
    _gen/signals-marshal.list \
    _gen/enums.h \
    _gen/gtypes.h \
    _gen/gtypes-body.h \
    _gen/interfaces.h \
    _gen/interfaces-body.h \
    _gen/svc-channel.c \
    _gen/svc-channel.h \
    _gen/svc-misc.c \
    _gen/svc-misc.h

BUILT_SOURCES = \
    _gen/all.xml \
    _gen/channel.xml \
    _gen/misc.xml \
    $(nodist_libtp_glib_tests_future_extensions_la_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

XSLTPROCFLAGS = --nonet --novalid

# Generated files which can be generated for all categories simultaneously

_gen/all.xml: all.xml $(wildcard *.xml) $(tools_dir)/xincludator.py
	$(mkdir_p) _gen
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/xincludator.py $< > $@

_gen/gtypes.h _gen/gtypes-body.h: _gen/all.xml \
	$(top_srcdir)/tools/glib-gtypes-generator.py
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/glib-gtypes-generator.py \
		$< _gen/gtypes Future

_gen/signals-marshal.list: _gen/all.xml \
	$(tools_dir)/glib-signals-marshal-gen.py
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/glib-signals-marshal-gen.py $< > $@

_gen/signals-marshal.h: _gen/signals-marshal.list Makefile.am
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=_future_ext_marshal $< > $@

_gen/signals-marshal.c: _gen/signals-marshal.list Makefile.am
	$(AM_V_GEN){ echo '#include "_gen/signals-marshal.h"' && \
	$(GLIB_GENMARSHAL) --body --prefix=_future_ext_marshal $< ; } > $@

_gen/enums.h: _gen/all.xml \
	$(tools_dir)/c-constants-gen.py
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/c-constants-gen.py \
		Future \
		$< > $@

_gen/interfaces-body.h _gen/interfaces.h: _gen/all.xml \
	$(tools_dir)/glib-interfaces-gen.py
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/glib-interfaces-gen.py \
		Future _gen/interfaces-body.h _gen/interfaces.h $<

_gen/%.xml: %.xml $(wildcard *.xml) $(tools_dir)/xincludator.py
	$(mkdir_p) _gen
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/xincludator.py $< > $@

_gen/svc-%.c _gen/svc-%.h: _gen/%.xml \
	$(tools_dir)/glib-ginterface-gen.py
	$(AM_V_GEN)$(PYTHON) $(tools_dir)/glib-ginterface-gen.py \
		--filename=_gen/svc-$* \
		--signal-marshal-prefix=_future_ext \
		--include='<telepathy-glib/dbus.h>' \
		--include='"_gen/signals-marshal.h"' \
		--not-implemented-func='tp_dbus_g_method_return_not_implemented' \
		--allow-unstable \
		$< Future_Svc_

endif # HAVE_TP_GLIB
