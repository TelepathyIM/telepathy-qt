EXTRA_DIST = \
    check-coding-style.mk \
    check-misc.sh \
    check-whitespace.sh \
    lcov.am \
    libtpcodegen.py \
    libqt4codegen.py \
    qt4-client-gen.py \
    qt4-constants-gen.py \
    qt4-types-gen.py \
    telepathy.am \
    with-session-bus.sh \
    xincludator.py

CLEANFILES = libtpcodegen.pyc libtpcodegen.pyo libqt4codegen.pyc libqt4codegen.pyo

all: $(EXTRA_DIST)

libqt4codegen.py: libtpcodegen.py
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
