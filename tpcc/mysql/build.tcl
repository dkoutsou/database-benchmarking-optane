#!/usr/bin/tclsh

dbset db mysql
dbset bm TPROC-C
diset tpcc mysql_count_ware $argv
diset tpcc mysql_num_vu 10
buildschema
waittocomplete
vudestroy
