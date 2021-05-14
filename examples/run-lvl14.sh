#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 600 --savFile saves/lvl14.sav scripts/lvl14.jaffar
