# Taken from telepathy-glib. The only change is to remove main.c and cut down
# the Makefile.am accordingly.
#
# PLEASE DO NOT MODIFY THIS CONNECTION MANAGER. Either subclass it,
# copy-and-modify (moving it to a better namespace), or make changes in the
# copy in telepathy-glib first.

AM_CFLAGS = \
	$(ERROR_CFLAGS) \
	-I$(top_builddir) -I$(top_srcdir)
AM_CXXFLAGS = $(ERROR_CXXFLAGS)

if ENABLE_TP_GLIB_TESTS

AM_CFLAGS += $(TP_GLIB_CFLAGS)
AM_CXXFLAGS += $(TP_GLIB_CFLAGS)

noinst_LTLIBRARIES = libexample-cm-callable.la

libexample_cm_callable_la_SOURCES = \
    conn.c \
    conn.h \
    connection-manager.c \
    connection-manager.h \
    media-channel.c \
    media-channel.h \
    media-manager.c \
    media-manager.h \
    media-stream.c \
    media-stream.h

libexample_cm_callable_la_LIBADD = \
	$(TP_GLIB_LIBS)

EXTRA_DIST = manager-file.py

_gen/example_callable.manager _gen/param-spec-struct.h: \
		manager-file.py $(top_srcdir)/tools/manager-file.py
	$(AM_V_at)$(mkdir_p) _gen
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/manager-file.py $(srcdir)/manager-file.py _gen

BUILT_SOURCES = _gen/example_callable.manager _gen/param-spec-struct.h
CLEANFILES = $(BUILT_SOURCES)

clean-local:
	rm -rf _gen

endif
