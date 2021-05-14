#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 400 --savFile quicksave.sav jaffar.config 
