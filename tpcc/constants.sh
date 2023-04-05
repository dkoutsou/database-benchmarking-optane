MYSQL_INSTALLATION=
VTUNE_INSTALLATION=
TPCC_INSTALLATION=
POSTGRES_INSTALLATION=

MAIN_DIR=$PWD
sf=1000
page_size=16k

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

function initialise_postgres() {
  pushd $POSTGRES_INSTALLATION
  if [[ -d data ]]
  then
    echo "data directory exists. remove it for initialisation"
    exit
  fi
  bin/pg_ctl -D $POSTGRES_INSTALLATION/data  --options="-U postgres --pwfile=pwfile"  initdb
  bin/pg_ctl -D $POSTGRES_INSTALLATION/data  start
  popd
}

function stop_postgres() {
  $POSTGRES_INSTALLATION/bin/pg_ctl stop -D $POSTGRES_INSTALLATION/data
}

function prepare_postgres_data() {

  pushd $POSTGRES_INSTALLATION
  rm -rf data

  if [ $config = 'ssd' ] || [ $config = 'pmem-cached' ]
  then
    cp -r data-${sf}w data
  elif [ $config = 'double-ssd' ]
  then
    cp -r data-${sf}w data
    rm -rf data/pg_wal
    ln -s /mnt/local/pg_wal data/pg_wal

  elif [ $config = 'pmem-ssd' ]
  then
    rm -rf /mnt/pmem0/postgres-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem0/postgres-tpcc-sf$sf
    rm -rf /mnt/pmem0/postgres-tpcc-sf$sf/pg_wal
    ln -s /mnt/local/pg_wal  /mnt/pmem0/postgres-tpcc-sf$sf/pg_wal
    ln -s /mnt/pmem0/postgres-tpcc-sf$sf data
  elif  [ $config = 'pmem-fsdax-dax' ] || [ $config = 'pmem-fsdax' ]
  then
    rm -rf /mnt/pmem0/postgres-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem0/postgres-tpcc-sf$sf
    ln -s /mnt/pmem0/postgres-tpcc-sf$sf data
  elif [[ $config == *"interleaved"* ]]
  then
    rm -rf /mnt/pmem2/postgres-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem2/postgres-tpcc-sf$sf
    ln -s /mnt/pmem2/postgres-tpcc-sf$sf data
  elif [ $config = 'pmem-sector' ]
  then
    echo "Im here"
    rm -rf /mnt/pmem1s/postgres-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem1s/postgres-tpcc-sf$sf
    ln -s /mnt/pmem1s/postgres-tpcc-sf$sf data
  else echo "Config $config not supported for postgres"; exit;
  fi

  popd

}

function start_postgres() {
  sync
  sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
  if [[ $config == *"interleaved"* ]]
  then
    numactl --cpunodebind=1 --membind=1 -- $POSTGRES_INSTALLATION/bin/pg_ctl start -o "-c config_file=${POSTGRES_INSTALLATION}/${postgres_config}.conf" -D $POSTGRES_INSTALLATION/data
  else
    numactl --cpunodebind=0 --membind=0 -- $POSTGRES_INSTALLATION/bin/pg_ctl start -o "-c config_file=${POSTGRES_INSTALLATION}/${postgres_config}.conf" -D $POSTGRES_INSTALLATION/data
  fi

}
###########################################3

function initialise_mysql() {
  pushd $MYSQL_INSTALLATION
  if [[ -d data ]]
  then
    echo "data directory exists. remove it for initialisation"
    exit
  fi

  bin/mysqld --initialize-insecure --user=`whoami` --innodb_page_size=${page_size}
  bin/mysqld --user=`whoami` --innodb_page_size=${page_size} &
  sleep 5
  while [[ `bin/mysqladmin -u root ping` != "mysqld is alive" ]]
  do
    echo "Mysql not alive yet ..."
    sleep 5
  done


  bin/mysqladmin -u root password mysql
  popd
}

function start_mysql() {
  pushd $MYSQL_INSTALLATION
  sync
  sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
  if [[ $config == *"interleaved"* ]]
  then
    numactl --cpunodebind=1 --membind=1 -- bin/mysqld --defaults-extra-file=${mysql_config}.cnf --innodb_page_size=${page_size} --user=`whoami`  &>> /dev/null &
  else
    numactl --cpunodebind=0 --membind=0 -- bin/mysqld --defaults-extra-file=${mysql_config}.cnf --innodb_page_size=${page_size} --user=`whoami`  &>> /dev/null &
  fi
  mysql_pid=$!
  sleep 5
  while [[ `bin/mysqladmin -u root -pmysql ping` != "mysqld is alive" ]]
  do
    echo "Mysql not alive yet ..."
    sleep 5
  done
  popd
}

function stop_mysql() {
  pushd $MYSQL_INSTALLATION
  if [[ `bin/mysqladmin -u root -pmysql ping` == "mysqld is alive" ]]
  then
    bin/mysqladmin -u root -pmysql shutdown
  fi
  sync
  popd
}

function prepare_mysql_data() {

  pushd $MYSQL_INSTALLATION
  rm -rf data

  if [ $config = 'ssd' ] || [ $config = 'pmem-cached' ]
  then cp -r data-${sf}w data
  elif [ $config = 'pmem-fsdax-dax' ] || [ $config = 'pmem-fsdax' ] || [ $config = 'pmem-fsdax-dax-xfs' ]
  then
    rm -rf /mnt/pmem0/mysql-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem0/mysql-tpcc-sf$sf
    ln -s /mnt/pmem0/mysql-tpcc-sf$sf data
  elif [[ $config == *"interleaved"* ]]
  then
    rm -rf /mnt/pmem2/mysql-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem2/mysql-tpcc-sf$sf
    ln -s /mnt/pmem2/mysql-tpcc-sf$sf data
  elif [ $config = 'pmem-sector' ]
  then
    echo "Im here"
    rm -rf /mnt/pmem1s/mysql-tpcc-sf$sf
    cp -r data-${sf}w /mnt/pmem1s/mysql-tpcc-sf$sf
    ln -s /mnt/pmem1s/mysql-tpcc-sf$sf data
  else echo "Config $config not supported for mysql"; exit;
  fi

  popd
}

function set_frequency() {
  sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
  sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
  sudo cpupower frequency-set -g performance >> /dev/null
  sudo sh -c "echo -1 > /proc/sys/kernel/perf_event_paranoid;"
}

