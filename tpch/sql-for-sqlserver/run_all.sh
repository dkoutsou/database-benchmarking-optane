#!/bin/bash

VTUNE_INSTALLATION=
RESULT_DIR=
SCALE_FACTOR=100;

$VTUNE_INSTALLATION/server/vpp-server start --webserver-port 6543 --database-port 8086 --data-dir \<\>
sudo systemctl restart mssql-server

for i in {1..22};
do
    # set CPU governor
    sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
    sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
    sudo cpupower frequency-set -g performance >> /dev/null
    # drop caches
    sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
    # move tempdb
    sqlcmd -S localhost -U SA -P passworD1 -i temp.sql
    sudo systemctl restart mssql-server
    sqlcmd -S localhost -U SA -P passworD1 -i clear_caches.sql
    # reduce buffer pool size and set socket to 0
    sqlcmd -S localhost -U SA -P passworD1 -i setup.sql
    # run queries
    echo "Running query $i..."
    result_file=$RESULT_DIR/mode-q$i-sf$SCALE_FACTOR.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/collector/vpp-collect start -d $RESULT_DIR/mode
    echo "Vtune on Query $i starting ..."
    sqlcmd -S localhost -U SA -P passworD1 -i queries/$i.sql -o output_$i.txt
    file=`sudo $VTUNE_INSTALLATION/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d samples $result_file
done;
