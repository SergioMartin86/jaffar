#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 1000 --savFile quicksave.sav jaffar.config 
