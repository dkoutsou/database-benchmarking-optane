#!/usr/bin/tclsh

puts "SETTING CONFIGURATION"
dbset db mssqls
diset tpcc mssqls_driver timed
diset tpcc mssqls_rampup 0
diset tpcc mssqls_duration 3
diset tpcc mssqls_allwarehouse true
diset tpcc mssqls_timeprofile true
loadscript

puts "$argv VU TEST"
vuset vu $argv
vucreate
vurun
waittocomplete
vudestroy
puts "TEST COMPLETE"
