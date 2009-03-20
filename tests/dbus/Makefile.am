# Tests each run under their own temporary dbus-daemon.

TESTS = \
    test-do-nothing

BUILT_SOURCES = \
    _gen/do-nothing.cpp.moc.hpp

_gen/%.moc.hpp: %
	$(mkdir_p) _gen
	$(MOC) $(MOC_INCLUDES) -i $< -o $@

noinst_PROGRAMS = $(TESTS)

test_do_nothing_SOURCES = do-nothing.cpp

# A subset of the CXXFLAGS sufficient to run moc - -Istuff is a valid
# argument to moc but -Wstuff is not
MOC_INCLUDES = \
    $(QTCORE_CFLAGS) \
    $(QTDBUS_CFLAGS) \
    $(QTTEST_CFLAGS) \
    $(TP_QT4_CFLAGS)

if HAVE_TEST_PYTHON
# Tests which can only be run if we have suitable versions of Python and
# dbus-python
TESTS += \
    test-account-basics

BUILT_SOURCES += \
    _gen/account-basics.cpp.moc.hpp

test_account_basics_SOURCES = account-basics.cpp
endif

if ENABLE_TP_GLIB_TESTS
# Tests which can only be run if we have telepathy-glib, and Qt was compiled
# to use the GLib main loop
TESTS += \
    test-chan-basics \
    test-chan-group \
    test-cm-basics \
    test-conn-basics \
    test-conn-requests \
    test-conn-roster \
    test-contacts \
    test-handles \
    test-stateful-proxy \
    test-streamed-media-chan \
    test-text-chan

BUILT_SOURCES += \
    _gen/chan-basics.cpp.moc.hpp \
    _gen/chan-group.cpp.moc.hpp \
    _gen/cm-basics.cpp.moc.hpp \
    _gen/conn-basics.cpp.moc.hpp \
    _gen/conn-requests.cpp.moc.hpp \
    _gen/conn-roster.cpp.moc.hpp \
    _gen/contacts.cpp.moc.hpp \
    _gen/handles.cpp.moc.hpp \
    _gen/stateful-proxy.cpp.moc.hpp \
    _gen/streamed-media-chan.cpp.moc.hpp \
    _gen/text-chan.cpp.moc.hpp

MOC_INCLUDES += $(TP_GLIB_CFLAGS)

test_chan_basics_SOURCES = chan-basics.cpp
test_chan_basics_LDADD = $(LDADD) $(top_builddir)/tests/lib/echo2/libtp-glib-echo2-tests.la

test_chan_group_SOURCES = chan-group.cpp
test_chan_group_LDADD = $(LDADD) $(top_builddir)/tests/lib/csh/libtp-glib-csh-tests.la

test_cm_basics_SOURCES = cm-basics.cpp
test_cm_basics_LDADD = $(LDADD) $(top_builddir)/tests/lib/libtp-glib-tests.la

test_conn_basics_SOURCES = conn-basics.cpp
test_conn_basics_LDADD = $(LDADD) $(top_builddir)/tests/lib/libtp-glib-tests.la

test_conn_requests_SOURCES = conn-requests.cpp
test_conn_requests_LDADD = $(LDADD) $(top_builddir)/tests/lib/echo2/libtp-glib-echo2-tests.la

test_conn_roster_SOURCES = conn-roster.cpp
test_conn_roster_LDADD = $(LDADD) $(top_builddir)/tests/lib/contactlist/libexample-cm-contactlist.la

test_contacts_SOURCES = contacts.cpp
test_contacts_LDADD = $(LDADD) $(top_builddir)/tests/lib/libtp-glib-tests.la

test_handles_SOURCES = handles.cpp
test_handles_LDADD = $(LDADD) $(top_builddir)/tests/lib/libtp-glib-tests.la

test_stateful_proxy_SOURCES = stateful-proxy.cpp
test_stateful_proxy_LDADD = $(LDADD) $(top_builddir)/tests/lib/libtp-glib-tests.la

test_streamed_media_chan_SOURCES = streamed-media-chan.cpp
test_streamed_media_chan_LDADD = $(LDADD) $(top_builddir)/tests/lib/callable/libexample-cm-callable.la

test_text_chan_SOURCES = text-chan.cpp
test_text_chan_LDADD = $(LDADD) \
                       $(top_builddir)/tests/lib/libtp-glib-tests.la \
                       $(top_builddir)/tests/lib/echo2/libtp-glib-echo2-tests.la
endif

LDADD = \
    $(QTCORE_LIBS) \
    $(QTDBUS_LIBS) \
    $(QTTEST_LIBS) \
    ${top_builddir}/tests/lib/libtp-qt4-tests.la \
    ${top_builddir}/TelepathyQt4/libtelepathy-qt4.la

AM_CXXFLAGS = \
    $(ERROR_CXXFLAGS) \
    $(MOC_INCLUDES)

TESTS_ENV = \
    abs_top_builddir=@abs_top_builddir@ \
    abs_top_srcdir=@abs_top_srcdir@ \
    XDG_DATA_HOME=@abs_top_builddir@/tests \
    XDG_DATA_DIRS=@abs_top_srcdir@/tests

TESTS_ENVIRONMENT = \
    $(TESTS_ENV) \
    sh $(top_srcdir)/tools/with-session-bus.sh --session --

CLEANFILES = \
    $(BUILT_SOURCES)

distclean-local:
	rm -rf _gen
