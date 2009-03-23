AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_QT4_CFLAGS)

noinst_LTLIBRARIES = libtelepathy-qt4-examples-roster.la

libtelepathy_qt4_examples_roster_la_LIBADD = \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

libtelepathy_qt4_examples_roster_la_SOURCES = \
	roster-item.cpp \
	roster-item.h \
	roster-widget.cpp \
	roster-widget.h

nodist_libtelepathy_qt4_examples_roster_la_SOURCES = \
	_gen/roster-item.moc.hpp \
	_gen/roster-widget.moc.hpp

noinst_PROGRAMS = roster

roster_LDADD = \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la \
	libtelepathy-qt4-examples-roster.la

roster_SOURCES = \
	main.cpp \
	roster-window.cpp \
	roster-window.h

nodist_roster_SOURCES = \
	_gen/roster-window.moc.hpp

BUILT_SOURCES = \
	$(nodist_libtelepathy_qt4_examples_roster_la_SOURCES) \
	$(nodist_roster_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(mkdir_p) _gen
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
