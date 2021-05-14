#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 980 --savFile saves/lvl02.sav scripts/lvl02.jaffar
