#!/bin/bash

./bin/ycsb.sh load rocksdb -s -P workloads/workloada -P config.dat -p rocksdb.dir=/path/to/workloada -s > test_a.dat
./bin/ycsb.sh load rocksdb -s -P workloads/workloadb -P config.dat -p rocksdb.dir=/path/to/workloadb -s > test_b.dat
./bin/ycsb.sh load rocksdb -s -P workloads/workloadc -P config.dat -p rocksdb.dir=/path/to/workloadc -s > test_c.dat
