FROM docker.io/ubuntu:hirsute

# update aptitude
ENV TZ=Europe/Zurich
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y --fix-missing upgrade

# install aptitude essentials
RUN apt-get -y install build-essential
RUN apt-get -y install git
RUN apt-get -y install vim
RUN apt-get -y install pkg-config
RUN apt-get -y install python3-dev
RUN apt-get -y install python3-pip

RUN apt-get update
RUN apt-get -y --fix-missing upgrade
RUN apt-get -y install libpulse-dev
RUN apt-get -y install libsdl2-dev
RUN apt-get -y install libsdl2-image-2.0-0
RUN apt-get -y install libboost-dev
RUN apt-get -y install libpng-dev
RUN apt-get -y install libjpeg-dev
RUN apt-get -y install tmux
RUN apt-get -y install libncurses-dev

RUN python3 -m pip install meson
RUN python3 -m pip install ninja
RUN python3 -m pip install cmake

CMD ["/bin/bash"]

# add non-root user
RUN useradd -m jaffar
WORKDIR /home/jaffar
USER jaffar

# Getting source codes
RUN git clone https://github.com/SergioMartin86/jaffar-play.git --recursive
RUN git clone https://github.com/SergioMartin86/jaffar.git --recursive

# Installing Jaffar-Play
WORKDIR /home/jaffar/jaffar-play
RUN meson --buildtype=release build 
RUN meson compile -C build 

# Installing Jaffar
WORKDIR /home/jaffar/jaffar
RUN meson --buildtype=release build 
RUN meson compile -C build 

# Adding required environment variables
ENV JAFFAR_MAX_FRAME_DATABASE_SIZE_MB=500
ENV JAFFAR_HASH_AGE_THRESHOLD=10
ENV JAFFAR_DISABLE_MOVE_HISTORY=FALSE
ENV JAFFAR_SAVE_BEST_EVERY_SECONDS=1
ENV JAFFAR_SAVE_CURRENT_EVERY_SECONDS=0
ENV JAFFAR_SHOW_UPDATE_EVERY_SECONDS=0
ENV SDLPOP_ROOT=$HOME/jaffar-play/extern/SDLPoP
ENV JAFFAR_SAVE_BEST_PATH=$HOME/jaffar.best.sav
ENV JAFFAR_SAVE_CURRENT_PATH=$HOME/jaffar.current.sav
ENV JAFFAR_SOLUTION_BEST_PATH=$HOME/jaffar.best.sol
ENV JAFFAR_SOLUTION_CURRENT_PATH=$HOME/jaffar.current.sol

# Changing dir to the examples folder
WORKDIR /home/jaffar/jaffar/examples

