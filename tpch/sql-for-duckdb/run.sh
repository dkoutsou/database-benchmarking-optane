#!/bin/bash
DB_NAME=/path/to/db/
VTUNE_INSTALLATION=/path/to/vtune
RESULT_DIR=/path/to/results/
SCALE_FACTOR=100

for run in {1..3}
do
    for i in {1..22}
    do
        # set CPU governor
        sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
        sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
        sudo cpupower frequency-set -g performance >> /dev/null
        # drop caches
        sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
        # run queries
        echo "Running query $i..."
        sed "s/{number}/$i/" setup_run.sql > run.sql
        numactl --cpunodebind=0 --membind=0 ./duckdb $DB_NAME < run.sql >> simple_$run.txt
    done;
done;

for i in {1..22}
do
    # set CPU governor
    sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
    sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
    sudo cpupower frequency-set -g performance >> /dev/null
    # drop caches
    sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
    # run queries
    echo "Running query $i..."
    sed "s/{number}/$i/" setup_run_explained.sql > run.sql
    numactl --cpunodebind=0 --membind=0 ./duckdb $DB_NAME < run.sql >> explained.txt
done;

$VTUNE_INSTALLATION/server/vpp-server start --webserver-port 6543 --database-port 8086 --data-dir \<\>

for i in {1..22};
do
    # set CPU governor
    sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
    sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
    sudo cpupower frequency-set -g performance >> /dev/null
    # drop caches
    sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
    # run queries
    echo "Running query $i..."
    result_file=/path/to/results/duckdb-memory-q$i-sf$SCALE_FACTOR.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/collector/vpp-collect start -d $RESULT_DIR/
    echo "Vtune on Query $i starting ..."
    sed "s/{number}/$i/" setup_run.sql > run.sql
    numactl --cpunodebind=0 --membind=0 ./duckdb $DB_NAME < run.sql
    file=`sudo $VTUNE_INSTALLATION/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d samples $result_file
done;

$VTUNE_INSTALLATION/server/vpp-server stop
