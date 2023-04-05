#!/usr/bin/tclsh

puts "STARTING WARMUP"
dbset db mssqls
diset tpcc mssqls_driver timed
diset tpcc mssqls_rampup 0
diset tpcc mssqls_duration 3
diset tpcc mssqls_allwarehouse true
loadscript

vuset vu 11
vucreate
vurun
waittocomplete
vudestroy
clearscript
puts "WARMUP COMPLETE"
