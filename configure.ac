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

dnl decide error flags
TP_CXX_COMPILER_FLAG(-Wall, ERROR_CXXFLAGS="-Wall", ERROR_CXXFLAGS="")
TP_CXX_COMPILER_FLAG(-Werror, werror=yes, werror=no)

AC_ARG_ENABLE(Werror,
  AC_HELP_STRING([--disable-Werror],[compile without -Werror (normally enabled in development builds)]),
    werror=$enableval, :)

TP_CXX_COMPILER_FLAG([$ERROR_CXXFLAGS -Wextra -Wno-missing-field-initializers -Wno-unused-parameter], wextra=yes, wextra=no)

ifelse(tp_qt4_nano_version, 0,
    [ official_release=yes ],
    [
        official_release=no
        if test x$werror = xyes; then
            ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror"
        fi
    ])

TP_CXX_COMPILER_FLAG([$ERROR_CXXFLAGS -Wmissing-declarations], ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Wmissing-declarations")

AC_SUBST(ERROR_CXXFLAGS)
AM_CONDITIONAL([OFFICIAL_RELEASE], [test "x$official_release" = xyes])

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

dnl Check for QtDBus
PKG_CHECK_MODULES(QTDBUS, [QtDBus >= 4.5])

AC_SUBST(QTDBUS_CFLAGS)
AC_SUBST(QTDBUS_LIBS)

dnl We want the examples to be able to use TP_QT4_CFLAGS, TP_QT4_LIBS
dnl as though they were independent programs that got them from pkgconfig
TP_QT4_CFLAGS='-I${top_builddir} -I${top_srcdir}'
TP_QT4_LIBS='${top_builddir}/TelepathyQt4/libtelepathy-qt4.la'
AC_SUBST(TP_QT4_CFLAGS)
AC_SUBST(TP_QT4_LIBS)

AC_PATH_PROGS([PINOCCHIO], [telepathy-pinocchio], [none],
  [$PATH:/usr/local/libexec:/usr/libexec:/usr/lib/telepathy])
AM_CONDITIONAL([HAVE_PINOCCHIO], [! test none = "$PINOCCHIO"])

dnl Check for code generation tools
PYTHON=
AC_CHECK_PROGS([PYTHON], [python2.5 python])
if test -z "$PYTHON"; then
  AC_MSG_ERROR([Python is required to compile this package])
fi

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
    tests/pinocchio/Makefile
    tools/Makefile
])
