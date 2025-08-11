#!/bin/bash

set -e

cleanup() {
    echo "Cleaning up..."
    rm -rf build/
}

trap cleanup EXIT

cd packages/react-native-audio-api/common/cpp/test

cmake -S . -B build

cd build
make -j4
./tests
cd ..

rm -rf build/
