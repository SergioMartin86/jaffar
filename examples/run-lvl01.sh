#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 250 --savFile saves/lvl01.sav scripts/lvl01.jaffar 
