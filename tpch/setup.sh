#!/bin/bash

source constants.sh
LOGFILE="$PWD/tpch-setup.log"

function initialise_mysql() {
  pushd $MYSQL_INSTALLATION
  if [[ -d data ]]
  then
    echo "data directory exists. remove it for initialisation"
    exit
  fi

  bin/mysqld --initialize-insecure --user=`whoami`
  popd
}

function initialise_postgres() {
  pushd $POSTGRES_INSTALLATION
  if [[ -d data ]]
  then
    echo "data directory exists. remove it for initialisation"
    exit
  fi

  bin/pg_ctl -D $POSTGRES_INSTALLATION/data initdb
  popd
}

function setup_mysql() {

  restart_mysql
  print_log "Creating tables ..."
  $MYSQL_INSTALLATION/bin/mysql -u root --skip-password < sql-for-mysql/create.sql >> $LOGFILE 2>&1

  print_log "Loading data ..."

  echo "SET FOREIGN_KEY_CHECKS=0; \
  LOAD DATA INFILE '$TPCH_INSTALLATION/region.tbl' INTO TABLE REGION FIELDS TERMINATED BY '|'; \
  LOAD DATA INFILE '$TPCH_INSTALLATION/nation.tbl' INTO TABLE NATION FIELDS TERMINATED BY '|';" |\
  $MYSQL_INSTALLATION/bin/mysql -u root --skip-password -A tpch  >> $LOGFILE 2>&1

  for ((slice=1;slice<=CHUNKS;slice++))
  do
  echo "SET FOREIGN_KEY_CHECKS=0; SET UNIQUE_CHECKS=0; SET SQL_LOG_BIN=0; SET AUTOCOMMIT=0;\
        LOAD DATA INFILE '$TPCH_INSTALLATION/lineitem.tbl.$slice' INTO TABLE LINEITEM FIELDS TERMINATED BY '|';\
        LOAD DATA INFILE '$TPCH_INSTALLATION/customer.tbl.$slice' INTO TABLE CUSTOMER FIELDS TERMINATED BY '|';\
        LOAD DATA INFILE '$TPCH_INSTALLATION/supplier.tbl.$slice' INTO TABLE SUPPLIER FIELDS TERMINATED BY '|';\
        LOAD DATA INFILE '$TPCH_INSTALLATION/part.tbl.$slice' INTO TABLE PART FIELDS TERMINATED BY '|';\
        LOAD DATA INFILE '$TPCH_INSTALLATION/orders.tbl.$slice' INTO TABLE ORDERS FIELDS TERMINATED BY '|';\
        LOAD DATA INFILE '$TPCH_INSTALLATION/partsupp.tbl.$slice' INTO TABLE PARTSUPP FIELDS TERMINATED BY '|'; COMMIT;" |\
        $MYSQL_INSTALLATION/bin/mysql -u root --skip-password -A tpch  >> $LOGFILE 2>&1
  print_log "Slice $slice done"
  done;

  stop_mysql
  mv $MYSQL_INSTALLATION/data $MYSQL_INSTALLATION/data$SCALE_FACTOR
  print_log "Setup done"

}

function setup_postgres() {

  restart_postgres
  $POSTGRES_INSTALLATION/bin/createdb tpch

  print_log "Creating tables ..."
  $POSTGRES_INSTALLATION/bin/psql tpch < sql-for-postgres/create.sql >> $LOGFILE 2>&1

  print_log "Altering parameters ..."

  $POSTGRES_INSTALLATION/bin/psql tpch < sql-for-postgres/config.sql >> $LOGFILE 2>&1
  $POSTGRES_INSTALLATION/bin/pg_ctl restart -D $POSTGRES_INSTALLATION/data >> $LOGFILE 2>&1

  print_log "Loading data ..."

  echo "COPY NATION FROM '$TPCH_INSTALLATION/nation.tbl' WITH DELIMITER AS '|'; \
        COPY REGION FROM '$TPCH_INSTALLATION/region.tbl' WITH DELIMITER AS '|';"\
        | $POSTGRES_INSTALLATION/bin/psql tpch  >> $LOGFILE 2>&1

  for ((slice=1;slice<=CHUNKS;slice++))
  do
  echo "COPY LINEITEM FROM '$TPCH_INSTALLATION/lineitem.tbl.$slice' WITH DELIMITER AS '|';\
        COPY CUSTOMER FROM '$TPCH_INSTALLATION/customer.tbl.$slice' WITH DELIMITER AS '|';\
        COPY SUPPLIER FROM '$TPCH_INSTALLATION/supplier.tbl.$slice' WITH DELIMITER AS '|';\
        COPY PART FROM '$TPCH_INSTALLATION/part.tbl.$slice' WITH DELIMITER AS '|';\
        COPY ORDERS FROM '$TPCH_INSTALLATION/orders.tbl.$slice' WITH DELIMITER AS '|';\
        COPY PARTSUPP FROM '$TPCH_INSTALLATION/partsupp.tbl.$slice' WITH DELIMITER AS '|';"\
        | $POSTGRES_INSTALLATION/bin/psql tpch  >> $LOGFILE 2>&1

  print_log "Slice $slice done"
  done;

  print_log "Adding indexes ..."
  $POSTGRES_INSTALLATION/bin/psql tpch < sql-for-postgres/index.sql >> $LOGFILE 2>&1


  echo "ALTER SYSTEM RESET ALL; ANALYZE;" | $POSTGRES_INSTALLATION/bin/psql tpch >> $LOGFILE 2>&1
  stop_postgres
  mv $POSTGRES_INSTALLATION/data $POSTGRES_INSTALLATION/data$SCALE_FACTOR

  print_log "Setup done"

}

set_chunks

if [ $1 = postgres ]
then
  initialise_postgres
  setup_postgres
elif [ $1 = mysql ]
then
  initialise_mysql
  setup_mysql
else
  echo "Enter postgres or mysql"
fi
