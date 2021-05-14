if [[ -z "${JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB}" ]]; then
  export JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB=500
fi

if [[ -z "${JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB}" ]]; then
  export JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB=2200
fi

if [[ -z "${JAFFAR_SAVE_BEST_EVERY_SECONDS}" ]]; then
  export JAFFAR_SAVE_BEST_EVERY_SECONDS=0
fi

if [[ -z "${JAFFAR_SAVE_CURRENT_EVERY_SECONDS}" ]]; then
  export JAFFAR_SAVE_CURRENT_EVERY_SECONDS=0
fi

if [[ -z "${JAFFAR_SHOW_UPDATE_EVERY_SECONDS}" ]]; then
  export JAFFAR_SHOW_UPDATE_EVERY_SECONDS=1
fi

if [[ -z "${JAFFAR_SAVE_BEST_PATH}" ]]; then
  export JAFFAR_SAVE_BEST_PATH=$HOME/jaffar.best.sav
fi

if [[ -z "${JAFFAR_SAVE_CURRENT_PATH}" ]]; then
  export JAFFAR_SAVE_CURRENT_PATH=$HOME/jaffar.current.sav
fi

if [[ -z "${SDL_AUDIODRIVER}" ]]; then
  export SDL_AUDIODRIVER=dummy
fi

if [[ -z "${OMP_NUM_THREADS}" ]]; then
  export OMP_NUM_THREADS=4
fi
