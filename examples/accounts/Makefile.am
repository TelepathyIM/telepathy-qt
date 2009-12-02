include $(top_srcdir)/tools/telepathy-qt4.mk

AM_CPPFLAGS = \
	-I$(top_srcdir) -I$(top_builddir) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_QT4_CFLAGS)

AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS)

noinst_PROGRAMS = accounts

accounts_LDADD = \
	$(QTCORE_LIBS) \
	$(QTGUI_LIBS) \
	$(QTDBUS_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

accounts_SOURCES = \
	main.cpp \
	account-item.cpp \
	account-item.h \
	accounts-window.cpp \
	accounts-window.h

nodist_accounts_SOURCES = \
	_gen/account-item.moc.hpp \
	_gen/accounts-window.moc.hpp

BUILT_SOURCES = \
	$(nodist_accounts_SOURCES)

CLEANFILES = \
	$(BUILT_SOURCES)

_gen/%.moc.hpp: %.h
	$(AM_V_at)$(mkdir_p) _gen
	$(TPQT4_V_MOC)$(MOC) $(AM_CPPFLAGS) -i $< -o $@
