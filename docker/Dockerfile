FROM docker.io/ubuntu:hirsute
MAINTAINER Sergio Martin <eienburuu@gmail.com>

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
WORKDIR /home/jaffar
RUN echo "export JAFFAR_MAX_FRAME_DATABASE_SIZE_MB=500" >> .bashrc; \
echo "export JAFFAR_HASH_AGE_THRESHOLD=10" >> .bashrc; \
echo "export JAFFAR_DISABLE_MOVE_HISTORY=FALSE" >> .bashrc; \
echo "export JAFFAR_SAVE_BEST_EVERY_SECONDS=1" >> .bashrc; \
echo "export JAFFAR_SAVE_CURRENT_EVERY_SECONDS=0" >> .bashrc; \
echo "export JAFFAR_SHOW_UPDATE_EVERY_SECONDS=0" >> .bashrc; \
echo "export JAFFAR_SAVE_BEST_PATH=/home/jaffar/jaffar.best.sav" >> .bashrc; \
echo "export JAFFAR_SAVE_CURRENT_PATH=/home/jaffar/jaffar.current.sav" >> .bashrc; \
echo "export JAFFAR_SOLUTION_BEST_PATH=/home/jaffar/jaffar.best.sol" >> .bashrc; \
echo "export JAFFAR_SOLUTION_CURRENT_PATH=/home/jaffar/jaffar.current.sol" >> .bashrc; \
echo "export PATH=/home/jaffar/jaffar/build:/home/jaffar/jaffar-play/build:\$PATH" >> .bashrc; \
echo "export LD_LIBRARY_PATH=/home/jaffar/jaffar/build:/home/jaffar/jaffar-play/build:\$LD_LIBRARY_PATH" >> .bashrc; \
echo "export SDLPOP_ROOT=/home/jaffar/jaffar-play/extern/SDLPoP" >> .bashrc

# Print welcome message
RUN echo '[ ! -z "$TERM" -a -r /home/jaffar/.motd ] && cat /home/jaffar/.motd' >> .bashrc
RUN echo "Welcome to Jaffar. For instructions, see: https://github.com/SergioMartin86/jaffar/" > .motd

# Changing dir to the examples folder
WORKDIR /home/jaffar/jaffar/examples

