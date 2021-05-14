#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 530 --savFile saves/lvl07.sav scripts/lvl07.jaffar
