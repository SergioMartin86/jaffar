#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 5000 --savFile lvl01/quicksave.sav --disableHistory --disableWin lvl*/jaffar.config 
