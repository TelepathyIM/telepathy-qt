include $(top_srcdir)/tools/telepathy-qt4.mk

AM_CPPFLAGS = \
	-I$(top_srcdir) -I$(top_builddir) \
	$(QTCORE_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(QTNETWORK_CFLAGS) \
	$(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS)

noinst_PROGRAMS = protocols

protocols_LDADD = \
	$(QTCORE_LIBS) \
	$(QTDBUS_LIBS) \
	$(QTNETWORK_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

protocols_SOURCES = \
	cm-wrapper.cpp \
	cm-wrapper.h \
	main.cpp \
	protocols.cpp \
	protocols.h

nodist_protocols_SOURCES = \
	_gen/cm-wrapper.moc.hpp \
	_gen/protocols.moc.hpp

BUILT_SOURCES = \
	$(nodist_protocols_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(AM_V_at)$(mkdir_p) _gen
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@
