#!/bin/bash

echo hello

for total_block_num in 100 1000 10000 100000 1000000; do
    echo $total_block_num
    ./hot-block.py 100 $total_block_num > hot-block-${total_block_num}.csv
done

paste \
    hot-block-100.csv \
    hot-block-1000.csv \
    hot-block-10000.csv \
    hot-block-100000.csv \
    hot-block-1000000.csv > hot-block-total.csv

			   
