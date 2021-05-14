#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 1150 --savFile saves/lvl03.sav scripts/lvl03.jaffar
