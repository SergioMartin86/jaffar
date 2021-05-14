#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 550 --savFile saves/lvl05.sav scripts/lvl05.jaffar
