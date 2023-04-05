#!/usr/bin/tclsh

puts "STARTING WARMUP"
dbset db mysql
diset tpcc mysql_driver timed
diset tpcc mysql_rampup 0
diset tpcc mysql_duration $argv
diset tpcc mysql_allwarehouse true
loadscript

vuset vu 11
vucreate
vurun
waittocomplete
vudestroy
clearscript
puts "WARMUP COMPLETE"

