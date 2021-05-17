#!/bin/bash

source .set_env.sh

mpirun -n 1 jaffar-train --maxSteps 9000 --savFile saves/lvl01.sav --disableWin scripts/lvl01.jaffar scripts/lvl15.jaffar scripts/lvl02.jaffar scripts/lvl03.jaffar scripts/lvl04.jaffar scripts/lvl05.jaffar scripts/lvl06.jaffar scripts/lvl07.jaffar scripts/lvl08.jaffar scripts/lvl09.jaffar scripts/lvl10.jaffar scripts/lvl11.jaffar scripts/lvl12.jaffar scripts/lvl13.jaffar scripts/lvl14.jaffar
