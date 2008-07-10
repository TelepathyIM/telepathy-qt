#!/bin/sh

fail=0

( . "${tools_dir}"/check-whitespace.sh ) || fail=$?

if egrep '(Free\s*Software\s*Foundation.*02139|02111-1307)' "$@"
then
  echo "^^^ The above files contain the FSF's old address in GPL headers"
  fail=1
fi

exit $fail
