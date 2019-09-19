#!/bin/bash

./db_bench --benchmarks="fillrandom" -num=100000000 -cache_numshardbits=0 -cache_size=16384 -uni_cache_size=1048576 -compression_ratio=1  -block_restart_interval=1

./db_bench --benchmarks="readrandom,stats" -use_existing_db -num=1000000 -cache_numshardbits=0 -cache_size=0 -uni_cache_size=1048576 -compression_ratio=1 -block_restart_interval=1

./db_bench --benchmarks="fillrandom,readrandom,stats" -num=1000000 -cache_numshardbits=0 -cache_size=0 -uni_cache_size=1048576 -compression_ratio=1 -block_restart_interval=1 -statistics
