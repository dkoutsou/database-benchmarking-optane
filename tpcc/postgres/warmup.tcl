#!/usr/bin/tclsh

puts "STARTING WARMUP"
dbset db pg
diset tpcc pg_driver timed
diset tpcc pg_rampup 0
diset tpcc pg_duration $argv
diset tpcc pg_allwarehouse true
diset tpcc pg_storedprocs true
diset tpcc pg_vacuum true

loadscript

vuset vu 24
vucreate
vurun
waittocomplete
vudestroy
clearscript
puts "WARMUP COMPLETE"

