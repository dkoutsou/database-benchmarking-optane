MYSQL_INSTALLATION=
POSTGRES_INSTALLATION=
VTUNE_INSTALLATION=
TPCH_INSTALLATION=
SCALE_FACTOR=100
config='ssd'
postgres_config='config1'
mysql_config='config1'
MAIN_DIR=$PWD


pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

function print_log() {
  local message=$1
  echo `date +"%Y-%m-%d %H:%M:%S"` "["`date +%s`"] : $message" >> $LOGFILE;
}

function set_chunks() {
  if [ $SCALE_FACTOR -eq 1 ]
  then
    CHUNKS=2
  else
    CHUNKS=$SCALE_FACTOR
  fi
}

function restart_mysql() {
  sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
  pushd $MYSQL_INSTALLATION
  if [[ `bin/mysqladmin -u root ping` == "mysqld is alive" ]]
  then
    print_log "Shutting down"
    bin/mysqladmin -u root shutdown >> $LOGFILE 2>&1
  fi
  numactl --cpunodebind=0 --membind=0 -- bin/mysqld --defaults-extra-file=${mysql_config}.cnf --user=`whoami` --secure-file-priv=""  &>> /dev/null &
  sleep 5
  while [[ `bin/mysqladmin -u root ping` != "mysqld is alive" ]]
  do
    print_log "Mysql not alive yet ..."
    sleep 5
  done
  popd
}

function stop_mysql() {
 $MYSQL_INSTALLATION/bin/mysqladmin -u root shutdown >> $LOGFILE 2>&1
}

function prepare_mysql_data() {
  stop_mysql
  pushd $MYSQL_INSTALLATION
  rm -rf data

  if [ $config = 'ssd' ] || [ $config = 'ssd-dramcache' ]
  then ln -s data$SCALE_FACTOR data
  elif [ $config = 'pmemasdisk' ] || [ $config = 'pmemasdisk-fsdax' ]
  then ln -s /mnt/pmem0/mysql-tpch-sf$SCALE_FACTOR data
  else echo "Config $config not supported for mysql"; exit;
  fi
  popd
}

function restart_postgres() {
  sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
  $POSTGRES_INSTALLATION/bin/pg_ctl stop -D $POSTGRES_INSTALLATION/data >> $LOGFILE 2>&1
  numactl --cpunodebind=0 --membind=0 -- $POSTGRES_INSTALLATION/bin/pg_ctl start -D $POSTGRES_INSTALLATION/data >> $LOGFILE 2>&1
  POSTGRES_PID=`ps -ef | grep postgres | head -1 | awk '{ print $2}'`
}

function stop_postgres() {
  $POSTGRES_INSTALLATION/bin/pg_ctl stop -D $POSTGRES_INSTALLATION/data >> $LOGFILE 2>&1
}

function setup_postgres_pmemforindex() {
  cp -rs $PWD/data$SCALE_FACTOR data
  restart_postgres
  pushd $MAIN_DIR
  index_filenodes=`$POSTGRES_INSTALLATION/bin/psql tpch < sql-for-postgres/index-filenodes.sql | tail -n +3 | head -n -2`
  tpch_dirname=`$POSTGRES_INSTALLATION/bin/psql tpch < sql-for-postgres/tpch-dirname.sql | tail -n +3 | head -n -2 | tr -d ' ' `
  popd
  stop_postgres
  for filenode in $index_filenodes;
    do
      files=`ls data/base/$tpch_dirname/$filenode*`
      for file in $files;
        do
          rm $file
          filename=$(basename $file)
          ln -s /mnt/pmem0/postgres-tpch-sf$SCALE_FACTOR/base/$tpch_dirname/$filename $file
      done
  done
}

function prepare_postgres_data(){
  stop_postgres
  pushd $POSTGRES_INSTALLATION
  rm -rf data

  if [ $config = 'ssd' ] || [ $config = 'ssd-dramcache' ] || [ $config = 'pmemasdram' ]
  then ln -s data$SCALE_FACTOR data
  elif [ $config = 'pmemasdisk' ] || [ $config = 'pmemasdisk-fsdax' ]
  then ln -s /mnt/pmem0/postgres-tpch-sf$SCALE_FACTOR data
  elif [ $config = 'pmemforindex' ]
  then setup_postgres_pmemforindex
  else echo "Config $config not supported for postgres"; exit;
  fi

  cp ${postgres_config}.conf data/postgresql.conf
  popd
}

function set_frequency() {
  sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
  sudo sh -c "echo 50 > /sys/devices/system/cpu/intel_pstate/max_perf_pct"
  sudo cpupower frequency-set -g performance >> /dev/null
}
