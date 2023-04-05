#!/bin/bash

source constants.sh


function generate() {

  echo "Generating scale factor $SCALE_FACTOR ..."
  pushd $TPCH_INSTALLATION

  limit=$((CHUNKS/10))
  for ((i=0;i<=limit;i++))
  do
    for j in {1..10}
    do
      slice=$((i*10+j))
      if [ $slice -le $CHUNKS ]
      then
        ./dbgen -f -s $SCALE_FACTOR -C $CHUNKS -S $slice &
      fi
    done
  wait

  echo "Slice $slice created ..."
  done
  popd

}

set_chunks
generate
