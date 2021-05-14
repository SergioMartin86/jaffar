#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 600 --savFile saves/lvl15.sav scripts/lvl15.jaffar
