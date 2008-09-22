dnl as-compiler-flag.m4 0.1.0

dnl autostars m4 macro for detection of compiler flags, ported to C++ by
dnl Olli Salli <olli.salli@collabora.co.uk>

dnl David Schleef <ds@schleef.org>

dnl $Id: as-compiler-flag.m4,v 1.1 2005/06/18 18:02:46 burgerman Exp $

dnl AS_COMPILER_FLAG(CXXFLAGS, ACTION-IF-ACCEPTED, [ACTION-IF-NOT-ACCEPTED])
dnl Tries to compile with the given CXXFLAGS.
dnl Runs ACTION-IF-ACCEPTED if the compiler can compile with the flags,
dnl and ACTION-IF-NOT-ACCEPTED otherwise.

AC_DEFUN([AS_COMPILER_FLAG],
[
  AC_LANG_ASSERT([C++])
  AC_MSG_CHECKING([to see if the C++ compiler understands $1])

  save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $1"

  AC_TRY_COMPILE([ ], [], [flag_ok=yes], [flag_ok=no])
  CXXFLAGS="$save_CXXFLAGS"

  if test "X$flag_ok" = Xyes ; then
    $2
    true
  else
    $3
    true
  fi
  AC_MSG_RESULT([$flag_ok])
])

