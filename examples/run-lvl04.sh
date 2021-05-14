#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 730 --savFile saves/lvl04.sav scripts/lvl04.jaffar
