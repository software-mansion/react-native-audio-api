#!/bin/bash

# Step 1: Configure CMake project
cmake -S . -B build

# Step 2: Build the project
cmake --build build

# Step 3: Run the test binary
cd build
./tests
cd ..

# Step 4: Clean up build directory
rm -rf build/
