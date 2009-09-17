AM_CPPFLAGS = \
	-I$(top_srcdir) -I$(top_builddir) \
	$(QTCORE_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(QTNETWORK_CFLAGS) \
	$(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS)

noinst_PROGRAMS = sender receiver

sender_LDADD = \
	$(QTCORE_LIBS) \
	$(QTDBUS_LIBS) \
	$(QTNETWORK_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

sender_SOURCES = \
	sender.cpp

nodist_sender_SOURCES = \
	_gen/sender.moc.hpp

receiver_LDADD = \
	$(QTCORE_LIBS) \
	$(QTDBUS_LIBS) \
	$(QTNETWORK_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

receiver_SOURCES = \
	receiver.cpp \
	receiver-channel.cpp

nodist_receiver_SOURCES = \
	_gen/receiver.moc.hpp \
	_gen/receiver-channel.moc.hpp

BUILT_SOURCES = \
	$(nodist_sender_SOURCES) \
	$(nodist_receiver_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(mkdir_p) _gen
	$(MOC) $(AM_CPPFLAGS) -i $< -o $@
