#!/usr/bin/python
import random
import sys

# def my_function(keys_per_block, total_block_num, hot_ratio):
#     print(keys_per_block, total_block_num, hot_ratio)
#     total_keys = keys_per_block * total_block_num
#     hot_key_num = int(hot_ratio * total_keys)
#     hot_keys = random.sample(xrange(total_keys), hot_key_num)
#     hot_block_set = set()
#     for hot_key in hot_keys:
#         hot_block = hot_key / keys_per_block
#         hot_block_set.add(hot_block)
#     print len(hot_block_set), 1. * len(hot_block_set) / total_block_num
#
# my_function(100, 1000000, 0.01)


def my_function(keys_per_block, total_block_num):
    total_key_num = keys_per_block * total_block_num
    keys = range(total_key_num)
    random.shuffle(keys)

    hot_block_set = set()
    for i in xrange(total_key_num):
        hot_key = keys[i]
        hot_block = hot_key / keys_per_block
        hot_block_set.add(hot_block)
        if ((i + 1) % (total_key_num / 1000) == 0):
            print i + 1, total_key_num, total_block_num,  1. * (i + 1) / total_key_num, 1. * len(hot_block_set) / total_block_num
        # if (1. * (i + 1) / total_key_num > 0.2):
        #     break

# for total_block_num in [100, 1000, 10000, 100000, 1000000]:
#     my_function(100, total_block_num)



if __name__ == "__main__":
    keys_per_block = int(sys.argv[1])
    total_block_num = int(sys.argv[2])
    my_function(keys_per_block, total_block_num)

