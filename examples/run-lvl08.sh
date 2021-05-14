#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 1000 --savFile saves/lvl08.sav scripts/lvl08.jaffar
