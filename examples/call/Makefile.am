include $(top_srcdir)/tools/telepathy-qt4.mk

AM_CPPFLAGS = \
	-I$(top_srcdir) -I$(top_builddir) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_GLIB_CFLAGS) \
	$(TP_FARSIGHT_CFLAGS) \
	$(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS)

noinst_PROGRAMS = call

call_LDADD = \
	$(QTCORE_LIBS) \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(TP_GLIB_LIBS) \
	$(TP_FARSIGHT_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la \
	$(top_builddir)/TelepathyQt4/Farsight/libtelepathy-qt4-farsight.la \
	$(top_builddir)/examples/roster/libtelepathy-qt4-examples-roster.la

call_SOURCES = \
	main.cpp \
	call-handler.cpp \
	call-handler.h \
	call-roster-widget.cpp \
	call-roster-widget.h \
	call-widget.cpp \
	call-widget.h \
	call-window.cpp \
	call-window.h \
	farsight-channel.cpp \
	farsight-channel.h \
	video-widget.cpp \
	video-widget.h

nodist_call_SOURCES = \
	_gen/call-handler.moc.hpp \
	_gen/call-roster-widget.moc.hpp \
	_gen/call-widget.moc.hpp \
	_gen/call-window.moc.hpp \
	_gen/farsight-channel.moc.hpp \
	_gen/video-widget.moc.hpp

BUILT_SOURCES = \
	$(nodist_call_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(AM_V_at)$(mkdir_p) _gen
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@
