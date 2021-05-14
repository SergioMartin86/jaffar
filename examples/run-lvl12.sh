#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 900 --savFile saves/lvl12.sav scripts/lvl12.jaffar
