#!/bin/bash

#testing different cache sizes and getting the random seek results

./db_bench --benchmarks="fillrandom" -num=200000000 -use_direct_reads=true
:>result_seek.log

for size_of_cache in 20000000 200000000 2000000000;
do
   ./db_bench --benchmarks="seekrandom" -use_existing_db=true -use_direct_reads=True -cache_size ${size_of_cache} -statistics > tmp.log
   cat tmp.log | grep micros/op >> result_seek.log
   cat tmp.log | grep rocksdb.block.cache.miss >> result_seek.log
   cat tmp.log | grep rocksdb.block.cache.hit >> result_seek.log
   cat tmp.log | grep rocksdb.block.cache.add >> result_seek.log
done
