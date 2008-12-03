#!/bin/sh
# git-which-branch.sh - output the name of the current git branch
#
# The canonical location of this program is the telepathy-spec tools/
# directory, please synchronize any changes with that copy.
#
# Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

default="$1"
if { ref="`git symbolic-ref HEAD 2>/dev/null`"; }; then
    echo ${ref#refs/heads/}
    exit 0
fi

if test -n "$default"; then
    echo "$default" >/dev/null
    exit 0
fi

echo "no git branch found" >&2
exit 1
