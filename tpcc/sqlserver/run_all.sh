#!/bin/bash

VTUNE_INSTALLATION=
RESULT_DIR=
MAIN_DIR=

$VTUNE_INSTALLATION/server/vpp-server start --webserver-port 6543 --database-port 8086 --data-dir \<\>

for users in 8 16 32 64 128
do
    # set CPU governor
    sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
    sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
    sudo cpupower frequency-set -g performance >> /dev/null
    # drop caches
    sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
    sudo systemctl restart mssql-server
    # drop the modified database
    sqlcmd -S localhost -U SA -P passworD1 -i $MAIN_DIR/drop.sql
    # attach the original database
    sqlcmd -S localhost -U SA -P passworD1 -i $MAIN_DIR/attach.sql
    # attach the database
    sqlcmd -S localhost -U SA -P passworD1 -i $MAIN_DIR/clear_caches.sql
    # reduce buffer pool size and set socket to 0
    sqlcmd -S localhost -U SA -P passworD1 -i $MAIN_DIR/setup.sql
    result_file=$VTUNE_INSTALLATION/vpp/collector/results/config.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/collector/vpp-collect start -d $RESULT_DIR
    # start warmup
    numactl --cpunodebind=0 --membind=0 --  ./hammerdbcli auto warmup.tcl
    # start benchmark
    numactl --cpunodebind=0 --membind=0 --  ./hammerdbcli auto benchmark.tcl $users
    cat /tmp/hdbxtprofile.log | tail --lines=18
    rm /tmp/hdbxtprofile.log
    file=`sudo $VTUNE_INSTALLATION/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d sqlserver-tpcc $result_file
done
