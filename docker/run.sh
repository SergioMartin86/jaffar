set -e
source docker.cfg
podman run  -it --rm ${REPOSITORY}:latest
