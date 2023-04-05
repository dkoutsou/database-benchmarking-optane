#!/usr/bin/tclsh

puts "SETTING CONFIGURATION"
dbset db pg
diset tpcc pg_driver timed
diset tpcc pg_rampup 0
diset tpcc pg_duration 3
diset tpcc pg_allwarehouse true
diset tpcc pg_timeprofile true
diset tpcc pg_storedprocs true

loadscript

puts "$argv VU TEST"
vuset vu $argv
vucreate
vurun
waittocomplete
vudestroy
puts "TEST COMPLETE"
