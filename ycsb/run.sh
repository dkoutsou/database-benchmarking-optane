#!/bin/bash
DATA_PATH=/path/to/data
VTUNE_INSTALLATION=/path/to/vtune
RESULT_DIR=/path/to/data

for wk in {a,b,c}
do
    for threads in {1,4,8,16,32,48}
    do
        for run in {1..3}
        do
            # set CPU governor
            sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
            sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
            sudo cpupower frequency-set -g performance >> /dev/null
            # drop caches
            sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
            # run workload
            echo "Running workload $wk ..."
            numactl --cpunodebind=0 --membind=0 ./bin/ycsb.sh run rocksdb -s -P workloads/workload$wk -P config.dat -threads $threads -p rocksdb.dir=$DATA_PATH-$wk
        done;

        $VTUNE_INSTALLATION/server/vpp-server start --webserver-port 6543 --database-port 8086 --data-dir \<\>

        # set CPU governor
        sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
        sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
        sudo cpupower frequency-set -g performance >> /dev/null
        # drop caches
        sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
        # run queries
        echo "Running workload $wk ..."
        result_file=/path/to/result/workload$wk-threads$threads.tar.gz
        sudo rm $result_file
        sudo $VTUNE_INSTALLATION/collector/vpp-collect start -d $RESULT_DIR/
        echo "Vtune on workload $wk starting ..."
        numactl --cpunodebind=0 --membind=0 ./bin/ycsb.sh run rocksdb -s -P workloads/workload$wk -P config.dat -threads $threads -p rocksdb.dir=$DATA_PATH-$wk
        file=`sudo $VTUNE_INSTALLATION/collector/vpp-collect stop | grep -o '/.*tar.gz'`
        sudo mv $file $result_file
        $VTUNE_INSTALLATION/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d samples $result_file
    done;
done;
$VTUNE_INSTALLATION/server/vpp-server stop
