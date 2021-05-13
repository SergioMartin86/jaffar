#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 600 --savFile quicksave.sav jaffar.config 
