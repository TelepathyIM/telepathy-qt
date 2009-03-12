AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_QT4_CFLAGS)

noinst_PROGRAMS = roster

roster_LDADD = \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

roster_SOURCES = \
	main.cpp \
	roster-item.cpp \
	roster-item.h \
	roster-widget.cpp \
	roster-widget.h

nodist_roster_SOURCES = \
	_gen/roster-item.moc.hpp \
	_gen/roster-widget.moc.hpp

BUILT_SOURCES = \
	$(nodist_roster_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(mkdir_p) _gen
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
