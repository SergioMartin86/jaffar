#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 230 --savFile saves/lvl06.sav scripts/lvl06.jaffar
