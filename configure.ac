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
m4_define([tp_qt4_minor_version], [0])
m4_define([tp_qt4_micro_version], [9999])
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

m4_define([tp_qt4_lt_current], [0])
m4_define([tp_qt4_lt_revision], [0])
m4_define([tp_qt4_lt_age], [0])

# Some magic
m4_define([tp_qt4_base_version],
          [tp_qt4_major_version.tp_qt4_minor_version.tp_qt4_micro_version])
m4_define([tp_qt4_version],
          [m4_if(tp_qt4_nano_version, 0, [tp_qt4_base_version],
              [tp_qt4_base_version].[tp_qt4_nano_version])])

AC_INIT([Telepathy-Qt4], [tp_qt4_version],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=Telepathy])
AC_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.9 -Wno-portability tar-ustar])
AM_CONFIG_HEADER(config.h)

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

dnl Likewise for the prototype library
PROTO_CFLAGS='-I${top_builddir} -I${top_srcdir}'
AC_SUBST(PROTO_CFLAGS)

dnl Check for telepathy-glib, and for Qt <-> GLib main loop integration:
dnl if we have both, we can run more tests
PKG_CHECK_MODULES(TP_GLIB, [telepathy-glib >= 0.7.20], [have_tp_glib=yes],
                  [have_tp_glib=no])
AC_SUBST(TP_GLIB_CFLAGS)
AC_SUBST(TP_GLIB_LIBS)
if test no = "$have_tp_glib"; then
  AC_MSG_WARN([telepathy-glib not found: not all tests will be run])
fi

have_qt_glib_main_loop=no
if ${PKG_CONFIG} --libs --static QtCore | grep ['[-]lglib-2.0'] >/dev/null; then
  have_qt_glib_main_loop=yes
else
  AC_MSG_WARN([Qt does not use the GLib main loop: not all tests will be run])
fi
AM_CONDITIONAL([ENABLE_TP_GLIB_TESTS],
  [test yes = "$have_tp_glib" && test yes = "$have_qt_glib_main_loop"])

AC_PATH_PROGS([PINOCCHIO], [telepathy-pinocchio], [none],
  [$PATH:/usr/local/libexec:/usr/libexec:/usr/lib/telepathy])
AC_PATH_PROGS([PINOCCHIO_CTL], [pinocchio-ctl], [none])
AM_CONDITIONAL([HAVE_PINOCCHIO],
  [test none != "$PINOCCHIO" && test none != "$PINOCCHIO_CTL"])

dnl Check for code generation tools
AM_PATH_PYTHON([2.5])

# Check for Python >= 2.5 with dbus, to run the tests
AC_MSG_CHECKING([for enough Python libraries to run tests])
if "$PYTHON" -c "import dbus.mainloop.glib" >/dev/null 2>&1; then
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
    TelepathyQt4/Prototype/Makefile
    TelepathyQt4/Prototype/TpQt4Prototype.pc
    TelepathyQt4/TelepathyQt4.pc
    doxygen.cfg
    examples/Makefile
    examples/extensions/Makefile
    m4/Makefile
    spec/Makefile
    tests/Makefile
    tests/dbus/Makefile
    tests/dbus-1/services/account-manager.service
    tests/lib/Makefile
    tests/pinocchio/Makefile
    tests/prototype/Makefile
    tools/Makefile
])
