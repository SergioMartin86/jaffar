#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 900 --savFile saves/lvl11.sav scripts/lvl11.jaffar
