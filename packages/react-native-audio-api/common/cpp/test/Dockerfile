FROM ubuntu:22.04

# Install build tools and dependencies, including GCC 13 for full C++20 support
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-13 g++-13 clang cmake git wget unzip python3 build-essential && \
    rm -rf /var/lib/apt/lists/*

# Set GCC 13 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Set working directory
WORKDIR /workspace/packages/react-native-audio-api/common/cpp/test

# Entrypoint is left empty so the script can specify the command
