#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 400 --savFile saves/lvl13.sav scripts/lvl13.jaffar
