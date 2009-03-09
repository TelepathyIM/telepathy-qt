AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_FARSIGHT_CFLAGS) \
	$(TP_QT4_CFLAGS)

noinst_PROGRAMS = call

call_LDADD = \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(TP_FARSIGHT_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

call_SOURCES = \
	main.cpp \
	farsight-glue.cpp \
	farsight-glue.h

nodist_call_SOURCES =

BUILT_SOURCES = \
	$(nodist_call_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(mkdir_p) _gen
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ @TP_FARSIGHT_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
