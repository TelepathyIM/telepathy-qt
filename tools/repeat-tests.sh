#!/bin/sh

if [ $# -ne 2 ]
then
    echo "usage: $0 <target> <number of repetitions>"
    echo "example: $0 check-valgrind 100"
    echo "     or: $0 \"-j4 check\" 100"
    exit 1
fi

for i in `seq 1 $2`
do
    echo -n "Running test iteration ${i}... "
    log="test-round-${i}.log"
    make $1 > ${log} 2>&1
    if grep -q "FAIL" $log
    then
        echo "FAILED (log in $log)"
    else
        echo "PASSED"
        rm ${log}
    fi
done
