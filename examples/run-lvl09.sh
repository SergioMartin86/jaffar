#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 1500 --savFile saves/lvl09.sav scripts/lvl09.jaffar
