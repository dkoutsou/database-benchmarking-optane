#!/usr/bin/tclsh

puts "SETTING CONFIGURATION"
dbset db mysql
diset tpcc mysql_driver timed
diset tpcc mysql_rampup 0
diset tpcc mysql_duration 3
diset tpcc mysql_allwarehouse true
diset tpcc mysql_timeprofile true
loadscript

puts "$argv VU TEST"
vuset vu $argv
vucreate
vurun
waittocomplete
vudestroy
puts "TEST COMPLETE"

