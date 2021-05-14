#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 9000 --savFile saves/lvl01.sav --disableHistory --disableWin scripts/*.jaffar 
