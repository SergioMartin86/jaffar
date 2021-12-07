#!/usr/bin/env bash
set -e
source docker.cfg
podman build -t ${REPOSITORY}:latest .
