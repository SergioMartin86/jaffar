#!/bin/bash

export JAFFAR_MAX_FRAME_DATABASE_SIZE_MB=2000
export JAFFAR_HASH_AGE_THRESHOLD=10
export OMP_NUM_THREADS=24
export JAFFAR_SAVE_BEST_PATH=$HOME/jaffar/examples/tas/lvl02/jaffar-bc1.best.sav
export JAFFAR_SAVE_CURRENT_PATH=$HOME/jaffar/examples/tas/lvl02/jaffar-bc1.current.sav

runJaffar() 
{
 seed=$(((RANDOM<<15|RANDOM)))
 echo "Running Seed: $seed" 
 frame=`jaffar-train --RNGSeed $seed --savFile lvl02b.sav lvl02bc1.jaffar | tee /dev/stderr | grep "Winning" | cut -d' ' -f6`
 echo "$seed $frame"
 echo "$seed $frame" >> results-bc1.txt
}

while true
do
 date
  sleep 1
  runJaffar &
 wait
done
