#!/bin/bash

# FFmpeg Mobile Architecture Build Script
# Builds static libraries (.a files) for iOS and Android architectures

set -e

# Configuration
SOURCE_DIR="/path/to/your/ffmpeg/source"  # Change this to your FFmpeg source directory
if [ ! -d "${SOURCE_DIR}" ]; then
    echo "FFmpeg source directory does not exist: ${SOURCE_DIR}"
    exit 1
fi
BUILD_DIR="$(pwd)/build"
OUTPUT_DIR="$(pwd)/output"

# Create directories
mkdir -p "${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"

# Common configure options
COMMON_CONFIG="
--disable-programs
--disable-doc
--disable-htmlpages
--disable-manpages
--disable-podpages
--disable-txtpages
--disable-avdevice
--disable-swscale
--disable-avfilter
--disable-protocols
--disable-devices
--disable-filters
--enable-static
--disable-shared
--enable-small
--disable-debug
--disable-optimizations
--disable-everything
--disable-audiotoolbox
--disable-videotoolbox
--disable-hwaccels
--enable-protocol=https,tls,tcp
--enable-demuxer=hls
--enable-demuxer=mov
--enable-demuxer=mp3
--enable-parser=aac
--enable-decoder=aac
--enable-decoder=mp3
--enable-decoder=flac
"

# Function to build for a specific architecture
build_arch() {
    local ARCH=$1
    local PLATFORM=$2
    local CC=$3
    local CXX=$4
    local CFLAGS=$5
    local LDFLAGS=$6
    local EXTRA_CONFIG=$7
    
    if [[ ${PLATFORM} == "linux" ]]; then
        PLATFORM_NAME="android"
        ARCH_NAME=${ARCH}
    elif [[ ${PLATFORM} == "darwin" ]]; then
        PLATFORM_NAME="ios"
        ARCH_NAME=${ARCH}
    elif [[ ${PLATFORM} == "darwinsim" ]]; then
        PLATFORM_NAME="ios"
        ARCH_NAME=${ARCH}-sim
    fi

    echo "Building FFmpeg for ${PLATFORM_NAME} ${ARCH_NAME}..."
    
    SOURCE_PATH="${BUILD_DIR}/ffmpeg-${PLATFORM_NAME}-${ARCH_NAME}"
    OUTPUT_PATH="${OUTPUT_DIR}/${PLATFORM_NAME}/${ARCH_NAME}"
    
    mkdir -p "${OUTPUT_PATH}"
    
    # Clean copy of source for this architecture
    echo "Copying source for ${PLATFORM_NAME} ${ARCH_NAME}..."
    rm -rf "${SOURCE_PATH}"
    cp -r "${SOURCE_DIR}" "${SOURCE_PATH}"
    
    cd "${SOURCE_PATH}"
    
    # Clean any previous build artifacts
    make distclean 2>/dev/null || true
    
    if [[ ${PLATFORM} == "darwinsim" ]]; then
        PLATFORM="darwin"
    fi

    echo --enable-cross-compile --arch=${ARCH} --target-os=${PLATFORM} --cc="${CC}" --cxx="${CXX}" --extra-cflags="${CFLAGS}" --extra-ldflags="${LDFLAGS}" --prefix="${OUTPUT_PATH}" ${COMMON_CONFIG} ${EXTRA_CONFIG}

    # Configure
    ./configure \
        --enable-cross-compile \
        --arch=${ARCH} \
        --target-os=${PLATFORM} \
        --cc="${CC}" \
        --cxx="${CXX}" \
        --extra-cflags="${CFLAGS}" \
        --extra-ldflags="${LDFLAGS}" \
        --prefix="${OUTPUT_PATH}" \
        ${COMMON_CONFIG} \
        ${EXTRA_CONFIG}
    
    # Build
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    make install
    
    echo "Completed ${PLATFORM} ${ARCH}"
    cd - > /dev/null
}

# Check if source exists
if [ ! -d "${SOURCE_DIR}" ]; then
    echo "FFmpeg source not found."
    exit 1
fi

# Clean the source directory of any previous builds
echo "Cleaning source directory..."
cd "${SOURCE_DIR}"
make distclean 2>/dev/null || true
cd - > /dev/null

# iOS Architectures
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Building for iOS architectures..."
    
    # iOS SDK paths
    IOS_SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
    IOS_SIM_SDK_PATH=$(xcrun --sdk iphonesimulator --show-sdk-path)
    
    # iOS Device architectures
    build_arch "arm64" "darwin" \
        "$(xcrun --sdk iphoneos --find clang)" \
        "$(xcrun --sdk iphoneos --find clang++)" \
        "-arch arm64 -mios-version-min=11.0 -isysroot ${IOS_SDK_PATH}" \
        "-arch arm64 -mios-version-min=11.0 -isysroot ${IOS_SDK_PATH}" \
        "--disable-iconv --disable-zlib"
    
    rm -rf "${OUTPUT_DIR}/ios/arm64/share"
    
    # # iOS Simulator architectures
    # build_arch "x86_64" "darwinsim" \
    #     "$(xcrun --sdk iphonesimulator --find clang)" \
    #     "$(xcrun --sdk iphonesimulator --find clang++)" \
    #     "-arch x86_64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
    #     "-arch x86_64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
    #     "--extra-libs=-lz -liconv"

    # rm -rf "${OUTPUT_DIR}/ios/x86_64-sim/share"

    build_arch "arm64" "darwinsim" \
        "$(xcrun --sdk iphonesimulator --find clang)" \
        "$(xcrun --sdk iphonesimulator --find clang++)" \
        "-arch arm64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "-arch arm64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "--disable-iconv --disable-zlib"

    rm -rf "${OUTPUT_DIR}/ios/arm64-sim/share"
    
    echo "iOS builds completed!"
else
    echo "Skipping iOS builds (requires macOS)"
fi

# Android Architectures
if [ -n "$ANDROID_NDK_ROOT" ] || [ -n "$NDK_ROOT" ]; then
    echo "Building for Android architectures..."
    
    # Use NDK_ROOT if ANDROID_NDK_ROOT is not set
    NDK_PATH="${ANDROID_NDK_ROOT:-$NDK_ROOT}"
    
    if [ ! -d "$NDK_PATH" ]; then
        echo "Android NDK not found. Please set ANDROID_NDK_ROOT or NDK_ROOT environment variable"
    else
        # Android API level
        API_LEVEL=21
        
        # Android toolchain
        TOOLCHAIN_PATH="${NDK_PATH}/toolchains/llvm/prebuilt"
        
        # Detect host OS for toolchain
        if [[ "$OSTYPE" == "darwin"* ]]; then
            HOST_TAG="darwin-x86_64"
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            HOST_TAG="linux-x86_64"
        else
            echo "Unsupported host OS for Android NDK"
            exit 1
        fi
        
        TOOLCHAIN="${TOOLCHAIN_PATH}/${HOST_TAG}"

        OPENSSL_PREBUILT_FOLDER="$(pwd)/openssl-prebuilt"
        if [ ! -d "$OPENSSL_PREBUILT_FOLDER" ]; then
            echo "Cloning and building OpenSSL for Android..."
            git clone https://github.com/openssl/openssl.git
            cd openssl
            export ANDROID_NDK_ROOT=${NDK_PATH}
            PATH=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/darwin-x86_64/bin:$PATH
            mkdir -p "${OPENSSL_PREBUILT_FOLDER}/include"
            cp -r include/crypto include/openssl "${OPENSSL_PREBUILT_FOLDER}/include"

            # arm64-v8a
            ./Configure android-arm64 no-shared no-asm -D__ANDROID_API__=${API_LEVEL}
            make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
            mkdir -p ${OPENSSL_PREBUILT_FOLDER}/arm64-v8a && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/arm64-v8a"
            make clean

            # armeabi-v7a
            ./Configure android-arm no-shared no-asm -D__ANDROID_API__=${API_LEVEL}
            make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
            mkdir -p ${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a"
            make clean && cd .. && rm -rf openssl
        fi
        
        # ARM64-v8a
        build_arch "aarch64" "linux" \
            "${TOOLCHAIN}/bin/aarch64-linux-android${API_LEVEL}-clang" \
            "${TOOLCHAIN}/bin/aarch64-linux-android${API_LEVEL}-clang++" \
            "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/darwin-x86_64/sysroot/usr/include" \
            "-L${OPENSSL_PREBUILT_FOLDER}/arm64-v8a -L${TOOLCHAIN}/darwin-x86_64/sysroot/usr/lib/armv7a-linux-android/${API_LEVEL}" \
            "--enable-openssl --extra-libs=-lz"

        rm -rf ${OUTPUT_DIR}/android/aarch64/share
        
        # ARMv7a
        build_arch "armv7a" "linux" \
            "${TOOLCHAIN}/bin/armv7a-linux-androideabi${API_LEVEL}-clang" \
            "${TOOLCHAIN}/bin/armv7a-linux-androideabi${API_LEVEL}-clang++" \
            "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/darwin-x86_64/sysroot/usr/include" \
            "-L${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a -L${TOOLCHAIN}/darwin-x86_64/sysroot/usr/lib/armv7a-linux-android/${API_LEVEL}" \
            "--enable-openssl --extra-libs=-lz"

        rm -rf ${OUTPUT_DIR}/android/armv7a/share
        
        echo "Android builds completed!"
    fi
else
    echo "Skipping Android builds (ANDROID_NDK_ROOT or NDK_ROOT not set)"
fi

echo ""
echo "Build Summary:"
echo "=============="
find "${OUTPUT_DIR}" -name "*.a" | while read -r lib; do
    echo "Built: $lib"
done

echo ""
echo "All builds completed!"
echo "Static libraries (.a files) are located in: ${OUTPUT_DIR}"