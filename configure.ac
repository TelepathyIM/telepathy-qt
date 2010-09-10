AC_PREREQ([2.59])

# Making releases:
#   set the new version number:
#     odd minor -> development series
#     even minor -> stable series
#     increment micro for each release within a series
#   set nano_version to 0
#   make the release, tag it
#   set nano_version to 1

m4_define([tp_qt4_major_version], [0])
m4_define([tp_qt4_minor_version], [3])
m4_define([tp_qt4_micro_version], [8])
m4_define([tp_qt4_nano_version], [1])

# If library source has changed since last release, increment revision
# If interfaces have been added, removed or changed since last release,
#  increment current and set revision to 0
# If interfaces have been added since last release, increment age
# If interfaces have been removed since last release, set age to 0
#
# If interfaces have been added, release builds will fail unless you add a new
# file like TelepathyQt4/versions/0.1.1.abi to add them to the official ABI.
# This also forces binaries built against devel versions to be rebuilt
# (we don't guarantee that we won't add ABI then remove it again, if it was
# never seen in a release).

m4_define([tp_qt4_lt_current], [9])
m4_define([tp_qt4_lt_revision], [0])
m4_define([tp_qt4_lt_age], [9])

# Some magic
m4_define([tp_qt4_base_version],
          [tp_qt4_major_version.tp_qt4_minor_version.tp_qt4_micro_version])
m4_define([tp_qt4_version],
          [m4_if(tp_qt4_nano_version, 0, [tp_qt4_base_version],
              [tp_qt4_base_version].[tp_qt4_nano_version])])

AC_INIT([Telepathy-Qt4], [tp_qt4_version],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=Telepathy&component=telepathy-qt4])
AC_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 -Wno-portability tar-ustar])
AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

dnl check for tools
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
AM_PROG_MKDIR_P
# pkg-config 0.21 adds CFLAGS from Requires.private, which we need
PKG_PROG_PKG_CONFIG([0.21])

dnl check for a C compiler too, for code taken from telepathy-glib
AC_LANG_PUSH([C])
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_LANG_POP([C])

LT_CURRENT=tp_qt4_lt_current
LT_REVISION=tp_qt4_lt_revision
LT_AGE=tp_qt4_lt_age
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

dnl optimizations, etc.
COMPILER_OPTIMISATIONS
COMPILER_COVERAGE
LINKER_OPTIMISATIONS
LINKER_VERSION_SCRIPT
TP_VISIBILITY_HIDDEN

ifelse(tp_qt4_nano_version, 0,
    [ official_release=yes ],
    [ official_release=no ])
AM_CONDITIONAL([OFFICIAL_RELEASE], [test "x$official_release" = xyes])

TP_COMPILER_WARNINGS([ERROR_CXXFLAGS], [test "x$official_release" = xno],
  [all \
   extra \
   sign-compare \
   pointer-arith \
   format-security \
   init-self],
   [missing-field-initializers \
    unused-parameter])
ERROR_CXXFLAGS="$ERROR_CXXFLAGS -DQT_NO_CAST_FROM_ASCII"
AC_SUBST([ERROR_CXXFLAGS])

# For C code taken from telepathy-glib, we want different warnings
AC_LANG_PUSH([C])
TP_COMPILER_WARNINGS([ERROR_CFLAGS], [test "x$official_release" = xno],
  [all \
   extra \
   declaration-after-statement \
   shadow \
   strict-prototypes \
   missing-prototypes \
   sign-compare \
   nested-externs \
   pointer-arith \
   format-security \
   init-self],
   [missing-field-initializers \
    unused-parameter])
AC_SUBST([ERROR_CFLAGS])
AC_LANG_POP([C])

AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--disable-debug],[compile without debug code]),
    enable_debug=$enableval, enable_debug=yes )

ifelse(tp_qt4_nano_version, 0,
    [ # tp-qt4 is version x.y.z - disable coding style checks by default
AC_ARG_ENABLE(coding-style-checks,
  AC_HELP_STRING([--enable-coding-style-checks],
                 [check coding style using grep]),
    [ENABLE_CODING_STYLE_CHECKS=$enableval], [ENABLE_CODING_STYLE_CHECKS=no] )
    ],
    [ # tp-qt4 is version x.y.z.1 - enable coding style checks by default
AC_ARG_ENABLE(coding-style-checks,
  AC_HELP_STRING([--disable-coding-style-checks],
                 [don't check coding style using grep]),
    [ENABLE_CODING_STYLE_CHECKS=$enableval], [ENABLE_CODING_STYLE_CHECKS=yes])
    ])

if test x$enable_debug = xyes; then
  AC_DEFINE(ENABLE_DEBUG, [], [Enable debug code])
fi

AC_SUBST([ENABLE_CODING_STYLE_CHECKS])

dnl Check for QtCore
PKG_CHECK_MODULES(QTCORE, [QtCore >= 4.5])
AC_SUBST(QTCORE_CFLAGS)
AC_SUBST(QTCORE_LIBS)

MOC=`$PKG_CONFIG --variable=moc_location QtCore`
AC_SUBST(MOC)

dnl We shouldn't need RCC at all - this is a *library* - but for the short term, let's at least
dnl make the RCC-using tests compile even when rcc isn't in $PATH.
pkgconfig_rcc=`pkg-config --variable=rcc_location QtCore`
if test -n "$pkgconfig_rcc"; then
    RCC=$pkgconfig_rcc
else
    RCC=`dirname $MOC`/rcc
fi

AC_SUBST(RCC)

dnl Check for QtDBus
PKG_CHECK_MODULES(QTDBUS, [QtDBus >= 4.5])
AC_SUBST(QTDBUS_CFLAGS)
AC_SUBST(QTDBUS_LIBS)

save_CFLAGS="$CFLAGS"
save_CXXFLAGS="$CXXFLAGS"
CFLAGS="$CFLAGS $QTDBUS_CFLAGS"
CXXFLAGS="$CXXFLAGS $QTDBUS_CFLAGS"
AC_TRY_COMPILE([#include <QtDBus>],
    [ QDBusVariant a; QDBusVariant b; if (a == b) { ; } ],
    [have_qdbusvariant_operator_equal=yes],
    [have_qdbusvariant_operator_equal=no])
CFLAGS="$save_CFLAGS"
CXXFLAGS="$save_CXXFLAGS"

if test x$have_qdbusvariant_operator_equal = xyes; then
  AC_DEFINE(HAVE_QDBUSVARIANT_OPERATOR_EQUAL, [], [Have QDBusVariant operator==])
fi

save_CFLAGS="$CFLAGS"
save_CXXFLAGS="$CXXFLAGS"
CFLAGS="$CFLAGS $QTDBUS_CFLAGS"
CXXFLAGS="$CXXFLAGS $QTDBUS_CFLAGS"
AC_MSG_CHECKING([if QDBus has the QDBusServiceWatcher API])
AC_TRY_COMPILE([#include <QtDBus>],
    [ QDBusServiceWatcher watcher; ],
    [have_qdbusservicewatcher=yes],
    [have_qdbusservicewatcher=no])
CFLAGS="$save_CFLAGS"
CXXFLAGS="$save_CXXFLAGS"

if test x$have_qdbusservicewatcher = xyes; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_QDBUSSERVICEWATCHER, [], [Have QDBusServiceWatcher])
else
  AC_MSG_RESULT([no])
fi

dnl Check for QtNetwork
PKG_CHECK_MODULES(QTNETWORK, [QtNetwork >= 4.5])
AC_SUBST(QTNETWORK_CFLAGS)
AC_SUBST(QTNETWORK_LIBS)

dnl Check for QtGui
have_qt_gui=
PKG_CHECK_MODULES(QTGUI, [QtGui >= 4.5], [have_qt_gui=yes], [have_qt_gui=no])
AC_SUBST(QTGUI_CFLAGS)
AC_SUBST(QTGUI_LIBS)
AM_CONDITIONAL([HAVE_QTGUI], [test yes = "$have_qt_gui"])

dnl Check for QtTest
PKG_CHECK_MODULES(QTTEST, [QtTest >= 4.5])
AC_SUBST(QTTEST_CFLAGS)
AC_SUBST(QTTEST_LIBS)

dnl We want examples/tests to be able to use TP_QT4_CFLAGS as though they
dnl were independent programs that got them from pkgconfig; AC_SUBST'ing
dnl TP_QT4_LIBS would interfere with dependency tracking, so don't do that
TP_QT4_CFLAGS='-I${top_builddir} -I${top_srcdir}'
AC_SUBST(TP_QT4_CFLAGS)

dnl Check for telepathy-glib, and for Qt <-> GLib main loop integration:
dnl if we have both, we can run more tests
PKG_CHECK_MODULES(TP_GLIB, [telepathy-glib >= 0.11.11], [have_tp_glib=yes],
                  [have_tp_glib=no])
TP_GLIB_CFLAGS="$TP_GLIB_CFLAGS -DQT_NO_KEYWORDS"
AC_SUBST(TP_GLIB_CFLAGS)
AC_SUBST(TP_GLIB_LIBS)
if test no = "$have_tp_glib"; then
  AC_MSG_WARN([telepathy-glib not found: not all tests will be run])
else
  GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
  AC_SUBST(GLIB_GENMARSHAL)
fi
AM_CONDITIONAL([HAVE_TP_GLIB], [test yes = "$have_tp_glib"])

have_qt_glib_main_loop=no
if ${PKG_CONFIG} --libs --static QtCore | grep ['[-]lglib-2.0'] >/dev/null; then
  have_qt_glib_main_loop=yes
else
  AC_MSG_WARN([Qt does not use the GLib main loop: not all tests will be run])
fi
AM_CONDITIONAL([ENABLE_TP_GLIB_TESTS],
  [test yes = "$have_tp_glib" && test yes = "$have_qt_glib_main_loop"])

dnl Check for telepathy-farsight... if we have that too, we can build an
dnl audio/video calling example
PKG_CHECK_MODULES(TP_FARSIGHT, [telepathy-farsight >= 0.0.4 gstreamer-0.10 gstreamer-interfaces-0.10],
                  [have_tp_fs=yes],
                  [have_tp_fs=no])
TP_FARSIGHT_CFLAGS="$TP_FARSIGHT_CFLAGS -DQT_NO_KEYWORDS"
AC_SUBST(TP_FARSIGHT_CFLAGS)
AC_SUBST(TP_FARSIGHT_LIBS)
if test no = "$have_tp_fs"; then
  AC_MSG_WARN([telepathy-farsight not found: not all examples will be built])
fi
# tp-qt4-farsight does not require the last telepathy-glib
PKG_CHECK_MODULES(TP_GLIB, [telepathy-glib >= 0.8.1], [have_old_tp_glib=yes],
                  [have_old_tp_glib=no])
AM_CONDITIONAL([ENABLE_TP_FARSIGHT],
  [test yes = "$have_old_tp_glib" && test yes = "$have_tp_fs" &&
   test yes = "$have_qt_glib_main_loop"])

dnl Check for code generation tools
AM_PATH_PYTHON([2.5])

# Check for Python >= 2.5 with dbus, to run the tests
AC_MSG_CHECKING([for enough Python libraries to run tests])
if "$PYTHON" -c "import dbus.mainloop.glib" >/dev/null 2>&1 &&
   "$PYTHON" -c "import gobject" >/dev/null 2>&1; then
  AC_MSG_RESULT([yes])
  TEST_PYTHON="$PYTHON"
else
  AC_MSG_RESULT([no])
  TEST_PYTHON=false
fi

AC_SUBST([TEST_PYTHON])
AM_CONDITIONAL([HAVE_TEST_PYTHON], test false != "$TEST_PYTHON")

dnl Doxygen
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN(TelepathyQt4, doxygen.cfg, doc)

AC_OUTPUT([
    Makefile
    TelepathyQt4/Makefile
    TelepathyQt4/Farsight/Makefile
    TelepathyQt4/TelepathyQt4.pc
    TelepathyQt4/TelepathyQt4-uninstalled.pc
    TelepathyQt4/TelepathyQt4Farsight.pc
    TelepathyQt4/TelepathyQt4Farsight-uninstalled.pc
    doxygen.cfg
    examples/Makefile
    examples/accounts/Makefile
    examples/call/Makefile
    examples/extensions/Makefile
    examples/file-transfer/Makefile
    examples/protocols/Makefile
    examples/roster/Makefile
    m4/Makefile
    spec/Makefile
    tests/Makefile
    tests/dbus/Makefile
    tests/dbus-1/services/account-manager.service
    tests/lib/Makefile
    tests/lib/glib/Makefile
    tests/lib/glib/callable/Makefile
    tests/lib/glib/contactlist/Makefile
    tests/lib/glib/csh/Makefile
    tests/lib/glib/echo/Makefile
    tests/lib/glib/echo2/Makefile
    tests/lib/glib/future/Makefile
    tests/lib/glib/future/call/Makefile
    tests/lib/glib/future/conference/Makefile
    tests/lib/glib/future/extensions/Makefile
    tests/lib/python/Makefile
    tools/Makefile
])
