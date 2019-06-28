#!/bin/bash

#testing different cache sizes and getting the random seek results

num_entries=200000000
echo $num_entries
./db_bench --benchmarks="fillrandom" -num=${num_entries} -use_direct_reads=true
:>result_seek.log

for size_of_cache in 20000000 200000000 2000000000;
do
   ./db_bench --benchmarks="seekrandom" -num=${num_entries} -use_existing_db=true -use_direct_reads=True -cache_size ${size_of_cache} -statistics > randseek_cache_size_${size_of_cahce}.log
   cat randseek_cache_size_${size_of_cache}.log | grep micros/op >> result_seek.log
   cat randseek_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.miss >> result_seek.log
   cat randseek_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.hit >> result_seek.log
   cat randseek_cache_size_${size_of_cache}.log | grep rocksdb.block.cache.add >> result_seek.log
done
