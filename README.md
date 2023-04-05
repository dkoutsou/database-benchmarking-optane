# Database installation
## MySql
[Download](https://dev.mysql.com/downloads/mysql)

```bash
cmake ..
make
make install DESTDIR="<install-location>"
bin/mysqld --initialize-insecure --user=<user>
bin/mysqld --user=<user> --secure-file-priv=""
bin/mysqladmin -u root shutdown
```

## Postgres

[Download](https://www.postgresql.org/ftp/source/)

```bash
./configure --prefix="<install-location>"
make
make install
bin/pg_ctl -D <absolute-data-directory> initdb
bin/pg_ctl -D <absolute-data-directory> (start | stop)

Might need to set PGHOST=localhost in bashrc
```

## SQLServer

Follow the [instructions](https://docs.microsoft.com/en-us/sql/database-engine/install-windows/install-sql-server?view=sql-server-ver15)

## Dbgen

For mySQL in `dss.h`, replace

`#define PR_END(fp) fprintf(fp, "\n")`

by

`#define PR_END(fp) {fseek(fp, -1, SEEK_CUR);fprintf(fp, "\n");}`


## Vtune and Platform Profiler

Download the version you want, e.g.:
`wget https://registrationcenter-download.intel.com/akdlm/irc_nas/17977/l_BaseKit_p_2021.3.0.3219_offline.sh`

Run this command to install:
`./l_BaseKit_p_2021.3.0.3219_offline.sh -f $TEMP_DIR -r yes -a --silent --install-dir $INSTALL_DIR --eula accept`

where `$INSTALL_DIR` is your installation directory and `$TEMP_DIR` is a
directory that the installer will extract the data.

Install OS modules. See VTUNE_INSTALLATION/sepdk/src/README.txt for reference

From files directory copy,
1. server.ini  to VTUNE_INSTALLATION/vpp/server/dist/nexus/nexus/server.ini (Server config)
2. disk_info.py to VTUNE_INSTALLATION/vpp/collector/bin/disk_info.py (Removed PMEM from display)
3. start to VTUNE_INSTALLATION/vpp/collector/bin/start (Increases sampling rate for hardware counters)


## TPC-H for MySQL and PostgreSQL

Update the installation paths and configuration settings in `tpch/constants.sh`.
place the config files in respective database directories. before running the
benchmark, start the server for platform profiler. Also, increase your sudo
timeout to infinity to avoid repeated prompts.

1. ./generate.sh
2. ./setup.sh <mysql|postgres>
3. ./benchmark.sh <mysql|postgres> OR ./benchmark_old.sh <mysql|postgres>


UI/Server for Platform Profiler
```
Go to VTUNE-INSTALLATION/vpp/server
./vpp-server start --webserver-port 6543 --database-port 8086 --data-dir <>
Visit http://r740-01:6543/apps/portal/portal.html#/repository
```

Use the `get_results.py` scripts in the respective folders to get the aggregated statistics.

## TPC-H for SQLServer

1. Modify the paths in the tpch/sql-for-sqlserver folder to specify data, log, and the vtune installation directory.
1. Create the database running: `sqlcmd -S localhost -U SA -P <your_password> -i create_db.sql`
2. Import the data with bulk inserts.
3. Run the `run_all.sh` script.

## TPC-H for DuckDB

1. Download [DuckDB](https://github.com/duckdb/duckdb/releases/download/v0.7.1/duckdb_cli-linux-amd64.zip)
2. Modify the relevant paths and create the database running the `create.sh` script. DuckDB doesn't support `ALTER` statements to add foreign keys after the insertion of data, so you need to have your tables in 1 file.
3. Run the benchmark using the `run.sh` script.

## Selectivity queries

We provide the selectivity queries in the folder tpch/selectivity-queries. For each database you need to replace the tpch queries with them and also update the scripts for the respective pattern.

## TPC-C
For TPC-C we create the database only once and we reattach to the initial data
before every experiment run to allow for reproducible experiments.

Download [HammerDB](https://github.com/TPC-Council/HammerDB/releases/download/v4.2/HammerDB-4.2-Linux.tar.gz).
Just uncompress to install. Do the following changes (to make the script accept command line arguments) :
```
diff -r old/ new/
38c38
<     if {$argc != 2 || [lindex $argv 0] != "auto" } {
---
>     if {$argc < 2 || [lindex $argv 0] != "auto" } {
43a44
>         set arg1 [lindex $argv 2]
84a86,87
>   set argv $autostart::arg1
>   set argc 1
```

## MySQL and PostgreSQL
Specify installation paths, scale factor, page size(only for mysql) in tpcc/constants.sh.
Postgres needs a file named pwfile in its top level folder with content postgres.
1. ./<mysql|postgres>.sh build (To be done only once for every scale factor)
2. ./<mysql|postgres>.sh benchmark <pmem_mode> <database_config_filename>

## SQLServer

1. Modify the paths in the tpcc/sqlserver folder to specify data, log, and the vtune installation directory.
2. Create the database: `sqlcmd -S localhost -U SA -P <your_password> -i create_db.sql`
3. Attach the data using the `attach.sql`: `sqlcmd -S localhost -U SA -P <your_password> -i create_db.sql` or build the database using `./hammerdbcli auto build.tcl`
4. Run the `run_all.sh` script.

## VoltDB
1. Clone the code from [here](https://github.com/VoltDB/voltdb)
2. Run the script in server mode in a terminal from [here](https://github.com/VoltDB/voltdb/blob/master/tests/test_apps/tpcc/run.sh)
3. Having the server open run as many clients as you need using the script in client mode from [here](https://github.com/VoltDB/voltdb/blob/master/tests/test_apps/tpcc/run.sh)


## YCSB

1. Clone the repo in version 0.17.0 from [here](https://github.com/brianfrankcooper/YCSB)
2. Follow the instructions for RocksDB in the respective folder
3. Generate the data using the `load.sh` script
4. Run the workloads using the `run.sh` script

## Microbenchmarks

See the separate README in the microbenchmarks folder for instructions.


## Dislaimer
Intel Optane Persistent Memory has to be configured in the correct mode to run
every experiment.
