#!/bin/bash

source ../constants.sh
set_frequency

function build() {
  initialise_postgres
  pushd $TPCC_INSTALLATION
  numactl --cpunodebind=0 --membind=0 --  ./hammerdbcli auto $MAIN_DIR/build.tcl $sf &> /dev/null
  popd
  stop_postgres
  mv $POSTGRES_INSTALLATION/data $POSTGRES_INSTALLATION/data${sf}w
}

function set() {
  stop_postgres
  prepare_postgres_data
  start_postgres
  if [[ $config == *"interleaved"* ]]
  then
    numactl --cpunodebind=1 --membind=1 --  ./hammerdbcli auto $MAIN_DIR/warmup.tcl 3 &> /dev/null
  else
    numactl --cpunodebind=0 --membind=0 --  ./hammerdbcli auto $MAIN_DIR/warmup.tcl 3 &> /dev/null
  fi
}

function benchmark() {
  pushd $TPCC_INSTALLATION
  set
  for users in 8 16 32 64 128
  do
    echo "################################################ $users  ############################################################"
    name=sf$sf-users$users-$config-$postgres_config-${sf}
    result_file=$VTUNE_INSTALLATION/vpp/collector/results/${name}.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect start -d $VTUNE_INSTALLATION/vpp/collector/results
    if [[ $config == *"interleaved"* ]]
    then
      numactl --cpunodebind=1 --membind=1 --  ./hammerdbcli auto $MAIN_DIR/benchmark.tcl $users &> /dev/null
    else
      numactl --cpunodebind=0 --membind=0 --  ./hammerdbcli auto $MAIN_DIR/benchmark.tcl $users > /dev/null
    fi
    cat /tmp/hdbxtprofile.log | tail --lines=18
    rm /tmp/hdbxtprofile.log
    file=`sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/vpp/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d postgres-tpcc $result_file
  done
  popd
}

if [ $1 = build ]
then
  build
elif [ $1 = benchmark ]
then
  config=$2
  postgres_config=$3
  benchmark
else
  echo "Enter build or benchmark"
fi
