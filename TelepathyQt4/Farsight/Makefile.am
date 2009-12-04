AM_CPPFLAGS = \
	-DBUILDING_TELEPATHY_QT4_FARSIGHT \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(QTCORE_CFLAGS) \
	$(QTDBUS_CFLAGS) \
	$(TP_GLIB_CFLAGS) \
	$(TP_FARSIGHT_CFLAGS) \
	$(TP_QT4_CFLAGS)

lib_LTLIBRARIES = libtelepathy-qt4-farsight.la

# The quoting here is unnecessary but harmless, and has the useful side-effect
# that vim quickfix mode (:make) doesn't interpret the libtool --mode=link
# command as an error message in a bizarrely named file
libtelepathy_qt4_farsight_la_LDFLAGS = \
    -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"
libtelepathy_qt4_farsight_la_LIBADD = \
	$(QTCORE_LIBS) \
	$(QTDBUS_LIBS) \
	$(TP_GLIB_LIBS) \
	$(TP_FARSIGHT_LIBS) \
	$(top_builddir)/TelepathyQt4/libtelepathy-qt4.la

libtelepathy_qt4_farsight_la_DEPENDENCIES = Makefile.am

libtelepathy_qt4_farsight_la_SOURCES = \
    channel.cpp

tpqt4farsightincludedir = \
    $(includedir)/telepathy-1.0/TelepathyQt4/Farsight
tpqt4farsightinclude_HEADERS = \
    Channel \
    channel.h \
    global.h
