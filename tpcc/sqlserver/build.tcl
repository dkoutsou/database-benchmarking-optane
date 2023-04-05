#!/usr/bin/tclsh

dbset db mssqls
dbset bm TPROC-C
diset tpcc mssqls_count_ware 1000
diset tpcc mssqls_num_vu 10
buildschema
waittocomplete
vudestroy
