ACLOCAL_AMFLAGS = -I m4

SUBDIRS = m4 tools spec TelepathyQt4

DISTCHECK_CONFIGURE_FLAGS = --disable-debug

include doxygen.am

EXTRA_DIST = \
    autogen.sh \
		doc/html \
		$(DX_CONFIG)

CLEANFILES = FIXME.out
MOSTLYCLEANFILES = $(DX_CLEANFILES)

check-local::
	egrep -A 5 '[F]IXME|[T]ODO|[X]XX' $(srcdir)/TelepathyQt4/*.[ch] \
		> FIXME.out || true

dist-hook:
	chmod u+w ${distdir}/ChangeLog
	if test -d .git; then git log >${distdir}/ChangeLog; fi

