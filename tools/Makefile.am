EXTRA_DIST = \
    c-constants-gen.py \
    check-coding-style.mk \
    check-misc.sh \
    check-whitespace.sh \
    git-which-branch.sh \
    glib-ginterface-gen.py \
    glib-gtypes-generator.py \
    glib-interfaces-gen.py \
    glib-signals-marshal-gen.py \
    lcov.am \
    libtpcodegen.py \
    libglibcodegen.py \
    libqt4codegen.py \
    qt4-client-gen.py \
    qt4-constants-gen.py \
    qt4-types-gen.py \
    manager-file.py \
    telepathy.am \
    with-session-bus.sh \
    xincludator.py

CLEANFILES = libtpcodegen.pyc libtpcodegen.pyo libqt4codegen.pyc libqt4codegen.pyo

all: $(EXTRA_DIST)

libglibcodegen.py: libtpcodegen.py
	touch $@
libqt4codegen.py: libtpcodegen.py
	touch $@
c-constants-gen.py: libtpcodegen.py
	touch $@
glib-ginterface-gen.py: libtpcodegen.py libglibcodegen.py
	touch $@
glib-gtypes-generator.py: libtpcodegen.py libglibcodegen.py
	touch $@
glib-interfaces-gen.py: libtpcodegen.py libglibcodegen.py
	touch $@
glib-signals-marshal-gen.py: libtpcodegen.py libglibcodegen.py
	touch $@
qt4-client-gen.py: libtpcodegen.py libqt4codegen.py
	touch $@
qt4-constants-gen.py: libtpcodegen.py libqt4codegen.py
	touch $@
qt4-types-gen.py: libtpcodegen.py libqt4codegen.py
	touch $@

TELEPATHY_SPEC_SRCDIR = $(top_srcdir)/../telepathy-spec
maintainer-update-from-telepathy-spec:
	set -e && cd $(srcdir) && \
	for x in $(EXTRA_DIST); do \
		if test -f $(TELEPATHY_SPEC_SRCDIR)/tools/$$x; then \
			cp $(TELEPATHY_SPEC_SRCDIR)/tools/$$x $$x; \
		fi; \
	done

TELEPATHY_GLIB_SRCDIR = $(top_srcdir)/../telepathy-glib
maintainer-update-from-telepathy-glib:
	set -e && cd $(srcdir) && \
	for x in $(EXTRA_DIST); do \
		if test -f $(TELEPATHY_GLIB_SRCDIR)/tools/$$x; then \
			cp $(TELEPATHY_GLIB_SRCDIR)/tools/$$x $$x; \
		fi; \
	done
