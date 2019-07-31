#!/usr/bin/python
import random
import sys

def my_function(keys_per_block, total_block_num, hot_ratio):
    print(keys_per_block, total_block_num, hot_ratio)
    total_keys = keys_per_block * total_block_num
    hot_key_num = int(hot_ratio * total_keys)
    hot_keys = random.sample(xrange(total_keys), hot_key_num)
    hot_block_set = set()
    for hot_key in hot_keys:
        hot_block = hot_key / keys_per_block
        hot_block_set.add(hot_block)
    print len(hot_block_set), 1. * len(hot_block_set) / total_block_num

my_function(100, 1000000, 0.0001)

