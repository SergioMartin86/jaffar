#!/bin/bash

source .set_env.sh

# Number of MPI Ranks to use
mpiRanks=1
if [ ! -z "$1" ]; then
   mpiRanks=$1
fi

# Creating results directory
rm -rf fullRunResults
mkdir -p fullRunResults

export JAFFAR_SAVE_BEST_PATH=jaffar.best.sav
export JAFFAR_SAVE_CURRENT_PATH=jaffar.current.sav
export JAFFAR_SOLUTION_BEST_PATH=jaffar.best.sol
export JAFFAR_SOLUTION_CURRENT_PATH=jaffar.current.sol

mpirun -n 1 jaffar-train --maxSteps 250  --savFile saves/lvl01.sav scripts/lvl01.jaffar; cat jaffar.best.sol >> fullRunResults/lvl01.sol 
mpirun -n 1 jaffar-train --maxSteps 120  --savFile jaffar.best.sav scripts/lvl15.jaffar; cat jaffar.best.sol >> fullRunResults/lvl15.sol 
mpirun -n 1 jaffar-train --maxSteps 980  --savFile jaffar.best.sav scripts/lvl02.jaffar; cat jaffar.best.sol >> fullRunResults/lvl02.sol 
mpirun -n 1 jaffar-train --maxSteps 1150 --savFile jaffar.best.sav scripts/lvl03.jaffar; cat jaffar.best.sol >> fullRunResults/lvl03.sol 
mpirun -n 1 jaffar-train --maxSteps 730  --savFile jaffar.best.sav scripts/lvl04.jaffar; cat jaffar.best.sol >> fullRunResults/lvl04.sol 
mpirun -n 1 jaffar-train --maxSteps 550  --savFile jaffar.best.sav scripts/lvl05.jaffar; cat jaffar.best.sol >> fullRunResults/lvl05.sol 
mpirun -n 1 jaffar-train --maxSteps 230  --savFile jaffar.best.sav scripts/lvl06.jaffar; cat jaffar.best.sol >> fullRunResults/lvl06.sol 
mpirun -n 1 jaffar-train --maxSteps 530  --savFile jaffar.best.sav scripts/lvl07.jaffar; cat jaffar.best.sol >> fullRunResults/lvl07.sol 
mpirun -n 1 jaffar-train --maxSteps 1000 --savFile jaffar.best.sav scripts/lvl08.jaffar; cat jaffar.best.sol >> fullRunResults/lvl08.sol 
mpirun -n 1 jaffar-train --maxSteps 1500 --savFile jaffar.best.sav scripts/lvl09.jaffar; cat jaffar.best.sol >> fullRunResults/lvl09.sol 
mpirun -n 1 jaffar-train --maxSteps 950  --savFile jaffar.best.sav scripts/lvl10.jaffar; cat jaffar.best.sol >> fullRunResults/lvl10.sol 
mpirun -n 1 jaffar-train --maxSteps 900  --savFile jaffar.best.sav scripts/lvl11.jaffar; cat jaffar.best.sol >> fullRunResults/lvl11.sol 
mpirun -n 1 jaffar-train --maxSteps 900  --savFile jaffar.best.sav scripts/lvl12.jaffar; cat jaffar.best.sol >> fullRunResults/lvl12.sol 
mpirun -n 1 jaffar-train --maxSteps 400  --savFile jaffar.best.sav scripts/lvl13.jaffar; cat jaffar.best.sol >> fullRunResults/lvl13.sol 
mpirun -n 1 jaffar-train --maxSteps 600  --savFile jaffar.best.sav scripts/lvl14.jaffar; cat jaffar.best.sol >> fullRunResults/lvl14.sol 

