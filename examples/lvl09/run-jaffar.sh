#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 1500 --savFile quicksave.sav jaffar.config 
