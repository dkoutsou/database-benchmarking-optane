#!/usr/bin/tclsh

dbset db pg
dbset bm TPROC-C
diset tpcc pg_count_ware $argv
diset tpcc pg_num_vu 10
diset tpcc pg_storedprocs true
diset tpcc pg_vacuum true
buildschema
waittocomplete
vudestroy
