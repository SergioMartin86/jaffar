#!/usr/bin/env bash
set -e
source docker.cfg
podman rmi -f ${REPOSITORY}:latest
podman rmi -f ${REPOSITORY}:${TAG}
