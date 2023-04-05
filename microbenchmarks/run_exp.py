#!/bin/bash


    echo "sequential test"
    ./benchmark 1 1 0 0 0 >> results/write_latency.txt
    ./benchmark 1 1 1 0 0 >> results/read_latency.txt
    ./benchmark 1 0 0 0 0 >> results/write_bandwidth.txt
    ./benchmark 8 0 0 0 0 >> results/write_bandwidth.txt
    ./benchmark 1 0 1 0 0 >> results/read_bandwidth.txt
    ./benchmark 8 0 1 0 0 >> results/read_bandwidth.txt
    echo "Random test"
    ./benchmark 1 1 0 0 1 >> results/write_latency.txt
    ./benchmark 1 1 1 0 1 >> results/read_latency.txt
    ./benchmark 1 0 0 0 1 >> results/write_bandwidth.txt
    ./benchmark 8 0 0 0 1 >> results/write_bandwidth.txt
    ./benchmark 1 0 1 0 1 >> results/read_bandwidth.txt
    ./benchmark 8 0 1 0 1 >> results/read_bandwidth.txt


