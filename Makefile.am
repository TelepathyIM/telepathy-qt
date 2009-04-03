ACLOCAL_AMFLAGS = -I m4

SUBDIRS = m4 tools spec TelepathyQt4 examples tests

DISTCHECK_CONFIGURE_FLAGS = --disable-debug

EXTRA_DIST = \
    autogen.sh \
    doc/html \
    doxygen.css \
    doxygen-footer.html \
    doxygen-header.html \
    $(DX_CONFIG)

CLEANFILES = FIXME.out
MOSTLYCLEANFILES = $(DX_CLEANFILES)

check-local::
	egrep -A 5 '[F]IXME|[T]ODO|[X]XX' $(srcdir)/TelepathyQt4/*.[ch]* \
		$(srcdir)/TelepathyQt4/*.[ch]* \
		> FIXME.out || true

doc/html: doxygen-doc

BRANCH = $(shell sh tools/git-which-branch.sh misc | tr -d '\n' | tr -C "[:alnum:]" _)
UPLOAD_BRANCH_TO = people.freedesktop.org:public_html/telepathy-qt4

upload-branch-docs: doc/html
	rsync -rtzvPp --chmod=a+rX doc/html/ \
		$(UPLOAD_BRANCH_TO)-$(BRANCH)/

maintainer-upload-release: _maintainer-upload-release-local
_maintainer-upload-release-local: _maintainer-upload-release-check
	rsync -rtvzPp --chmod=Dg+s,ug+rwX,o=rX doc/html/ \
	telepathy.freedesktop.org:/srv/telepathy.freedesktop.org/www/doc/telepathy-qt4/

include doxygen.am
include tools/lcov.am
include tools/telepathy.am
