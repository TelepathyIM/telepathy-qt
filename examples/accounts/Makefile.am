AM_CXXFLAGS = \
	$(ERROR_CXXFLAGS) \
	$(QTCORE_CFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_QT4_CFLAGS)

noinst_PROGRAMS = accounts

accounts_LDADD = \
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
	$(mkdir_p) _gen
	$(MOC) @QTCORE_CFLAGS@ @QTDBUS_CFLAGS@ -I$(top_builddir) -I$(top_srcdir) -i $< -o $@
