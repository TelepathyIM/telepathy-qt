# tp-visibility-hidden.m4 - detect support for -fvisibility-hidden
# Based on linker.m4 by Scott James Remnant.
#
# Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
# Copyright © 2009 Collabora Ltd. <http://www.collabora.co.uk/>
# Copyright © 2009 Nokia Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# TP_VISIBILITY_HIDDEN
# --------------------------
# Detect whether the (C/C++) compiler and linker both support
# -fvisibility=hidden.
#
# If so, put it in $(VISIBILITY_HIDDEN_FLAGS).
AC_DEFUN([TP_VISIBILITY_HIDDEN],
[AC_MSG_CHECKING([for compiler/linker symbol-hiding argument])
tp_old_ldflags="$LDFLAGS"
tp_old_cflags="$CFLAGS"
tp_old_cxxflags="$CXXFLAGS"
for tp_try_arg in "-fvisibility=hidden"; do
	LDFLAGS="$tp_old_ldflags $tp_try_arg"
	CFLAGS="$tp_old_cflags $tp_try_arg"
	CXXFLAGS="$tp_old_cxxflags $tp_try_arg"
	AC_TRY_LINK([], [], [
		AC_MSG_RESULT([$tp_try_arg])
		AC_SUBST([VISIBILITY_HIDDEN_FLAGS], [$tp_try_arg])
		break
	])
done
LDFLAGS="$tp_old_ldflags"
CFLAGS="$tp_old_cflags"
CXXFLAGS="$tp_old_cxxflags"
AM_CONDITIONAL(HAVE_VISIBILITY_HIDDEN_FLAGS, [test -n "$VISIBILITY_HIDDEN_FLAGS"])
if test -z "$VISIBILITY_HIDDEN_FLAGS"; then
	AC_MSG_RESULT([not supported])
fi
])dnl
