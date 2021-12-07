# Jaffar 

![](jaffar.png)

High-performance solver for Prince of Persia (DOS) tool-assisted speedrunning.  

Docker
===============

To get a ready-to-run Docker container with Jaffar, simply run:

```
docker run sergiom86/jaffar:latest
```

Installation
===============

Follow these steps to manually install in your system

1) Get code

 
```
  git clone --recursive https://github.com/SergioMartin86/jaffar.git
```
  
2) Create build directory


```
  cd jaffar
  mkdir build
  cd build
```

3) Configure and compile

```
  meson ..
  ninja
```
  
Requisites
============

- Meson - For compilation
- Ninja - For compilation
- SDL2: Jaffar has been tested using SDL2 2.0.14. 
- SDL2-image: Jaffar has been tested using SDL2_image 2.0.5.

Usage
=========


```
jaffar-train example.sav example.jaffar
```

Where example.sav is a SDLPop-compatible savestate, and example.jaffar is a Jaffar script with rules to solve the level.

NOTE: If you see the following message:

```
[ERROR] Wrong size of input state lvl06b.sav. Expected: 2714, Read: 2710 bytes.
```

This means you have produced the .sav file with an older version of SDLPop. Yo solve this, add additional zeros at the end of the savestate, until it meets the required size.

For tools to display the current state of training and/or the final results, see: `https://github.com/SergioMartin86/jaffar-play`

Environment Variables:
------------------------

Indicate where the SDLpop root folder is located:

```
export SDLPOP_ROOT=$HOME/jaffar/extern/SDLPoP
```

Indicate number of OMP threads to use. Leave empty if unsure of how many to use.

```
export OMP_NUM_THREADS=16
```

Specify maximum memory size to be used for frame database per worker (in megabytes): 

```
export JAFFAR_MAX_FRAME_DATABASE_SIZE_MB=1000
```

Specify maximum hash table history (last N game frames during which Jaffar will check for duplicates):

```
export JAFFAR_HASH_AGE_THRESHOLD=100
```

Specify how frequently (in seconds) and where the `jaffar-train` command should save the best state of exploration:

```
export JAFFAR_SAVE_BEST_EVERY_SECONDS=5
export JAFFAR_SAVE_BEST_PATH=jaffar.best.sav
```

Specify how frequently (in seconds) and where the `jaffar-train` command should save the current state of exploration:

```
export JAFFAR_SAVE_CURRENT_EVERY_SECONDS=1
export JAFFAR_SAVE_CURRENT_PATH=jaffar.current.sav
```

Authors
=============

- Sergio Martin (eien86)
  + Github: https://github.com/SergioMartin86
  + Twitch: https://www.twitch.tv/eien86
  + Youtube: https://www.youtube.com/channel/UCSXpK3d6vUq58fjgF5jFoKA
   
Credits to Alexander Lyashuk (mooskagh, crem) for the initial idea and development of an initial version of the bot (https://bitbucket.org/mooskagh/sdlpop-tricks).
Credits to David Nagy and the developers of SDLPop (https://github.com/NagyD/SDLPoP) for reverse engineering Prince of Persia and making this possible.
