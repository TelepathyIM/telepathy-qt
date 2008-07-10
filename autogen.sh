#!/bin/sh
set -e

if test -n "$AUTOMAKE"; then
    : # don't override an explicit user request
elif automake-1.8 --version >/dev/null 2>/dev/null && \
     aclocal-1.8 --version >/dev/null 2>/dev/null; then
    # If we have automake-1.8, use it. This helps to ensure that our build
    # system doesn't accidentally grow automake-1.9 dependencies.
    AUTOMAKE=automake-1.8
    export AUTOMAKE
    ACLOCAL=aclocal-1.8
    export ACLOCAL
fi

autoreconf -i -f

run_configure=true
for arg in $*; do
    case $arg in
        --no-configure)
            run_configure=false
            ;;
        *)
            ;;
    esac
done

if test $run_configure = true; then
    ./configure "$@"
fi
