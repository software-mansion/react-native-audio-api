#!/bin/bash

# This script builds and runs the AddressSanitizer-enabled tests in a Linux Docker container from macOS.
# Usage: ./run_graph_tests_in_docker.sh
# Make sure to run from the root of your repo or adjust paths accordingly.

set -e

# Absolute path to the repo root on the host (macOS)
REPO_ROOT=$(cd "$(dirname "$0")/../../../../.." && pwd)

# Name for the Docker image/container
IMAGE_NAME=asan-graph-test
CONTAINER_NAME=asan-graph-test-container

# Build the Docker image (Dockerfile must be in the test dir)
docker build -t $IMAGE_NAME "${REPO_ROOT}/packages/react-native-audio-api/common/cpp/test"

# Run the container, mounting the entire repo for source access
docker run --rm -it \
  --name $CONTAINER_NAME \
  -v "$REPO_ROOT:/workspace" \
  -w /workspace/packages/react-native-audio-api/common/cpp/test \
  -e ASAN_OPTIONS=detect_leaks=1:verbosity=2 \
  $IMAGE_NAME \
  bash RunTestsGraph.sh
