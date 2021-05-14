#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 950 --savFile quicksave.sav jaffar.config 
