dnl tp-cxx-compiler-flag.m4
dnl
dnl Autoconf macro for determining if the C++ compiler supports a given set of
dnl command-line flags. It requires AC_LANG([C++]). Adapted from an autostars
dnl (http://autostars.sourceforge.net) macro called AS_COMPILER_FLAG.
dnl
dnl TP_CXX_COMPILER_FLAG(CXXFLAGS, SUPPORTS, [DOESNT-SUPPORT])
dnl Tries to compile with the given CXXFLAGS.
dnl Runs SUPPORTS if the compiler can compile with the flags,
dnl and DOESNT-SUPPORT otherwise.

AC_DEFUN([TP_CXX_COMPILER_FLAG],
[
  AC_LANG_ASSERT([C++])
  AC_MSG_CHECKING([to see if the C++ compiler understands $1])

  save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS $1"

  AC_COMPILE_IFELSE([ ], [flag_ok=yes], [flag_ok=no])
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

