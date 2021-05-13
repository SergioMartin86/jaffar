#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 250 --savFile quicksave.sav jaffar.config 
