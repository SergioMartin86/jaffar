# Jaffar

High-performance fully scalable solver for Prince of Persia (DOS) tool-assisted speedrunning. 

This software is based on the sdlpop-tricks (https://bitbucket.org/mooskagh/sdlpop-tricks) bot.

Installation
-------------

1) Get code 

  git clone --recursive https://github.com/SergioMartin86/jaffar.git
  
2) Create build directory

  cd jaffar
  mkdir build
  cd build
  
3) Configure and compile

  meson ..
  ninja
  
Requisites
---------------

- SDL2: Jaffar has been tested using SDL2 2.0.14 and SDL2_image 2.0.5. If these libraries are not present at the moment of installation, Jaffar will install them automatically. However, these pre-packaged versions have shown some problems on some systems when running on the x11 driver (dummy driver works).

Usage
-------------

TBD


Authors
-------------

- Sergio Martin (eien86)
  + Github: https://github.com/SergioMartin86
  + Twitch: https://www.twitch.tv/eien86
  + Youtube: https://www.youtube.com/channel/UCSXpK3d6vUq58fjgF5jFoKA
  
- Alexander Lyashuk (mooskagh, crem) 
  + Github: https://github.com/mooskagh
  + Twitch: https://www.twitch.tv/mooskagh
