# Taken from telepathy-glib. The only change is to remove main.c and cut down
# the Makefile.am accordingly.
#
# PLEASE DO NOT MODIFY THIS CONNECTION MANAGER. Either subclass it,
# copy-and-modify (moving it to a better namespace), or make changes in the
# copy in telepathy-glib first.

# Example connection manager with audio/video calls.

noinst_LTLIBRARIES = libexample-cm-call.la

libexample_cm_call_la_SOURCES = \
    cm.c \
    cm.h \
    conn.c \
    conn.h \
    call-channel.c \
    call-channel.h \
    call-manager.c \
    call-manager.h \
    call-content.c \
    call-content.h \
    call-stream.c \
    call-stream.h

libexample_cm_call_la_LIBADD = \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    -I$(top_srcdir)/tests/lib/glib/future \
    $(ERROR_CFLAGS) \
    $(TP_GLIB_CFLAGS)

EXTRA_DIST = manager-file.py

_gen/example_call.manager _gen/param-spec-struct.h: \
		manager-file.py $(top_srcdir)/tools/manager-file.py
	$(AM_V_at)$(mkdir_p) _gen
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/manager-file.py $(srcdir)/manager-file.py _gen

BUILT_SOURCES = _gen/example_call.manager _gen/param-spec-struct.h
CLEANFILES = $(BUILT_SOURCES)

clean-local:
	rm -rf _gen
