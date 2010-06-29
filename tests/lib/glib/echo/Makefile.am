# Taken from telepathy-glib. The only change is to remove main.c and cut down
# the Makefile.am accordingly.
#
# PLEASE DO NOT MODIFY THIS CONNECTION MANAGER. Either subclass it,
# copy-and-modify (moving it to a better namespace), or make changes in the
# copy in telepathy-glib first.

noinst_LTLIBRARIES = libexample-cm-echo.la

libexample_cm_echo_la_SOURCES = \
    chan.c \
    chan.h \
    conn.c \
    conn.h \
    connection-manager.c \
    connection-manager.h \
    im-manager.c \
    im-manager.h

# In an external project you'd use $(TP_GLIB_LIBS) (obtained from
# pkg-config via autoconf) instead of the .la path
libexample_cm_echo_la_LIBADD = \
    $(GLIB_LIBS) \
    $(DBUS_LIBS) \
    $(TP_GLIB_LIBS)

AM_CFLAGS = \
    $(ERROR_CFLAGS) \
    $(DBUS_CFLAGS) \
    $(GLIB_CFLAGS) \
    $(TP_GLIB_CFLAGS)

EXTRA_DIST = manager-file.py

_gen/example_echo.manager _gen/param-spec-struct.h: \
		manager-file.py $(top_srcdir)/tools/manager-file.py
	$(AM_V_at)$(mkdir_p) _gen
	$(AM_V_GEN)$(PYTHON) $(top_srcdir)/tools/manager-file.py $(srcdir)/manager-file.py _gen

BUILT_SOURCES = _gen/example_echo.manager _gen/param-spec-struct.h
CLEANFILES = $(BUILT_SOURCES)

clean-local:
	rm -rf _gen
