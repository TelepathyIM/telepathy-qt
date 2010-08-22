#!/bin/sh

if [ $# -ne 2 ]
then
    echo "usage: $0 <command> <number of repetitions>"
    echo "example: $0 \"make check-valgrind\" 100"
    echo "     or: $0 \"make -j4 check\" 100"
    exit 1
fi

for i in `seq 1 $2`
do
    echo -n "Running test iteration ${i}... "
    log="test-round-${i}.log"
    $1 > ${log} 2>&1
    if grep -q "FAIL" $log
    then
        echo "FAILED (log in $log)"
    else
        echo "PASSED"
        rm ${log}
    fi
done
