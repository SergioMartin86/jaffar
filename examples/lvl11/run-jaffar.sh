#!/bin/bash

mpirun -n 1 jaffar-train --maxSteps 900 --savFile quicksave.sav jaffar.config 
