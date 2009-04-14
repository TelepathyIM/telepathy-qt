#!/bin/sh

flags=
debug=
profile=

for i in $*; do
    case $i in
    --enable-debug)
        debug="yes"
        ;;
    --disable-debug)
        debug="no"
        ;;
    --enable-compiler-coverage)
        profile=true
        ;;
    --prefix=*)
        prefix=`echo $i | sed 's/[-a-zA-Z0-9]*=//'`
        flags="$flags -DCMAKE_BUILD_PREFIX=$prefix"
        ;;
    esac
done

mkdir -p build
cd build

if [ "$profile" ]; then
    flags="$flags -DCMAKE_BUILD_TYPE=Profile"
elif [ "$debug" == "yes" ]; then
    flags="$flags -DCMAKE_BUILD_TYPE=Debug"
elif [ "$debug" == "no" ]; then
    flags="$flags -DCMAKE_BUILD_TYPE=Release"
fi

echo "running cmake $flags .."
cmake $flags ..
