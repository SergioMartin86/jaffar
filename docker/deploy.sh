#!/usr/bin/env bash
set -e
source docker.cfg

podman build \
    -t "${REPOSITORY}:${TAG}" \
    -t "${REPOSITORY}:latest" .

podman login docker.io
podman push "${REPOSITORY}:${TAG}"
podman push "${REPOSITORY}:latest"

podman rmi -f ${REPOSITORY}:${TAG}
