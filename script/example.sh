#!/bin/bash

echo hello world!

# ./db_bench --key_size 10 > tmp.log
# cat tmp.log | grep micros | awk '{print $3, $5, $7}'

:> result.log

for key_size in 10 15 20; do
    echo ${key_size}
    ./db_bench --key_size ${key_size} --benchmarks "fillrandom,readrandom,seekrandom" > tmp.log
    cat tmp.log | grep micros | awk '{print $3, $5, $7}' >> result.log
done
