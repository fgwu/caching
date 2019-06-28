#!/bin/bash

#testing different cache sizes and getting the read random results

num_entries=200000000
./db_bench --benchmarks="fillrandom" -num=$num_entries -use_direct_reads=true
:>result_read.log

for size_of_cache in  20000000 200000000 2000000000;
do
   ./db_bench --benchmarks="readrandom" -num=$num_entries -use_existing_db=true -use_direct_reads=True -cache_size ${size_of_cache} -statistics > randread_cache_size_${size_of_cahce}.log
   cat randread_cache_size_${size_of_cache}.log | grep micros/op >> result_read.log
   cat randread_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.miss >> result_read.log
   cat randread_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.hit >> result_read.log
   cat randread_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.add >> result_read.log
done
