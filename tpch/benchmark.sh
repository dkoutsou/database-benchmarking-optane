#!/bin/bash

source constants.sh
LOGFILE="$PWD/tpch-benchmark.log"

function mysql_profiling() {

  prepare_mysql_data
  set_frequency

  for i in {1..22}
  do

    restart_mysql
    full_query="set session internal_tmp_mem_storage_engine=Memory; "
    if [ $i -eq 15 ]
    then full_query+="`cat sql-for-mysql/15modified.sql`"
    else full_query+=" `cat sql-for-mysql/$i.sql` "
    fi

    result_file=$VTUNE_INSTALLATION/vpp/collector/results/mysql--q$i-sf$SCALE_FACTOR-$config-$mysql_config.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect start -d $VTUNE_INSTALLATION/vpp/collector/results
    echo "query $i" >>  mysql-sf$SCALE_FACTOR-$config-$mysql_config
    print_log "Vtune on Query$i starting ..."
    $MYSQL_INSTALLATION/bin/mysql -u root -A tpch <<< "$full_query" >>  mysql-sf$SCALE_FACTOR-$config-$mysql_config
    file=`sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/vpp/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d samples $result_file

  done
  stop_mysql
}

function postgres_profiling() {
  prepare_postgres_data
  set_frequency

  for i in {1..22}
  do
    restart_postgres
    print_log "Vtune on Query$i starting ..."

    if [ $i -eq 15 ]
    then full_query="`cat sql-for-postgres/15modified.sql`"
    else full_query="EXPLAIN (ANALYZE , BUFFERS) `cat sql-for-postgres/$i.sql` "
    fi

    result_file=$VTUNE_INSTALLATION/vpp/collector/results/postgres--q$i-sf$SCALE_FACTOR-$config-$postgres_config.tar.gz
    sudo rm $result_file
    sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect start -d $VTUNE_INSTALLATION/vpp/collector/results
    echo "query $i" >>  postgres-sf$SCALE_FACTOR-$config-$postgres_config
    $POSTGRES_INSTALLATION/bin/psql tpch <<< "$full_query" >> postgres-sf$SCALE_FACTOR-$config-$postgres_config
    file=`sudo $VTUNE_INSTALLATION/vpp/collector/vpp-collect stop | grep -o '/.*tar.gz'`
    sudo mv $file $result_file
    $VTUNE_INSTALLATION/vpp/collector/vpp-collect upload -q -n 127.0.0.1 -p 6543 -d samples $result_file
  done
  stop_postgres
}

function stop_everything() {
  stop_mysql
  stop_postgres
}

stop_everything
if [ $1 = postgres ]
then
  postgres_profiling
elif [ $1 = mysql ]
then
  mysql_profiling
else
  echo "Enter postgres or mysql"
fi

