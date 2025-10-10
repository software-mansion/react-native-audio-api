#!/bin/bash

# FFmpeg Mobile Architecture Build Script
# Builds shared ffmpeg binaries for iOS and Android architectures
set -e

SOURCE_DIR="/path/to/your/ffmpeg"  # Change this to your FFmpeg source directory
if [ ! -d "${SOURCE_DIR}" ]; then
    echo "FFmpeg source directory does not exist: ${SOURCE_DIR}"
    exit 1
fi

BUILD_DIR="$(pwd)/build"
OUTPUT_DIR="$(pwd)/output"
mkdir -p "${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"

AVUTIL_VERSION="60.6.100"
AVCODEC_VERSION="62.8.100"
AVFORMAT_VERSION="62.1.103"
SWRRESAMPLE_VERSION="6.0.100"

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
--disable-static
--enable-shared
--enable-small
--disable-debug
--disable-optimizations
--disable-everything
--disable-audiotoolbox
--disable-videotoolbox
--disable-hwaccels
--enable-protocol=https,tls,tcp,http
--enable-demuxer=hls
--enable-demuxer=mov
--enable-demuxer=mp3
--enable-parser=aac
--enable-decoder=aac
--enable-decoder=mp3
--enable-decoder=flac
--enable-protocol=udp
--enable-protocol=file
--enable-pic
--disable-x86asm
"

build_arch() {
    local ARCH=$1
    local PLATFORM=$2
    local CC=$3
    local CXX=$4
    local CFLAGS=$5
    local LDFLAGS=$6
    local EXTRA_CONFIG=$7
    
    if [[ ${PLATFORM} == "android" ]]; then
        PLATFORM_NAME="android"
        if [[ ${ARCH} == "aarch64" ]]; then
            ARCH_NAME="arm64-v8a"
        elif [[ ${ARCH} == "armv7a" ]]; then
            ARCH_NAME="armeabi-v7a"
        else
            ARCH_NAME=${ARCH}
        fi
    elif [[ ${PLATFORM} == "darwin" ]]; then
        PLATFORM_NAME="ios"
        ARCH_NAME="iphoneos"
    elif [[ ${PLATFORM} == "darwinsim" ]]; then
        PLATFORM_NAME="ios"
        if [[ ${ARCH} == "x86_64" ]]; then
            ARCH_NAME="iphonesimulator_x86_64"
        else
            ARCH_NAME="iphonesimulator_arm64"
        fi
    fi

    echo "Building FFmpeg for ${PLATFORM_NAME} ${ARCH_NAME}..."
    
    SOURCE_PATH="${BUILD_DIR}/ffmpeg-${PLATFORM_NAME}-${ARCH_NAME}"
    OUTPUT_PATH="${OUTPUT_DIR}/${PLATFORM_NAME}/${ARCH_NAME}"
    
    mkdir -p "${OUTPUT_PATH}"
    
    echo "Copying source for ${PLATFORM_NAME} ${ARCH_NAME}..."
    rm -rf "${SOURCE_PATH}"
    cp -r "${SOURCE_DIR}" "${SOURCE_PATH}"
    
    cd "${SOURCE_PATH}"
    
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
    make -j10
    make install
    
    echo "Completed ${PLATFORM} ${ARCH}"
    cd - > /dev/null
}

fix_dynamic_ios_linkage() {
    local LIB_PATH=$1
    
    # Get all dependencies that are not system libraries (including the library itself)
    otool -L "${LIB_PATH}" | grep -v "/usr/lib/" | grep -v "/System/" | awk 'NR>1 {print $1}' | while read -r dep; do
        if [[ -n "$dep" ]]; then
            # Extract library name without any version numbers and extension for framework path
            local lib_name=$(basename "$dep" | sed 's/\.dylib$//' | sed 's/\.[0-9][0-9.]*$//')
            local framework_path="@rpath/${lib_name}.framework/${lib_name}"
            echo "Changing dependency: $dep -> $framework_path"
            install_name_tool -change "$dep" "$framework_path" "${LIB_PATH}"
        fi
    done
    
    # Also update the library's own install name to use @rpath with framework structure
    local lib_name=$(basename "${LIB_PATH}" | sed 's/\.dylib$//' | sed 's/\.[0-9][0-9.]*$//')
    local framework_path="@rpath/${lib_name}.framework/${lib_name}"
    echo "Updating install name for $(basename "${LIB_PATH}") -> $framework_path"
    install_name_tool -id "$framework_path" "${LIB_PATH}"
}

# Check if source exists
if [ ! -d "${SOURCE_DIR}" ]; then
    echo "FFmpeg source not found."
    exit 1
fi

# Clean the source directory of any previous builds
cd "${SOURCE_DIR}"
make distclean 2>/dev/null || true
cd - > /dev/null

# Use NDK_ROOT if ANDROID_NDK_ROOT is not set
NDK_PATH="${ANDROID_NDK_ROOT:-$NDK_ROOT}"

if [ ! -d "$NDK_PATH" ]; then
    echo "Android NDK not found. Please set ANDROID_NDK_ROOT or NDK_ROOT environment variable"
    exit 1
fi

API_LEVEL=21
TOOLCHAIN_PATH="${NDK_PATH}/toolchains/llvm/prebuilt"
export ANDROID_NDK_ROOT=${NDK_PATH}

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
PATH=$TOOLCHAIN/bin:$PATH

OPENSSL_PREBUILT_FOLDER="$(pwd)/openssl-prebuilt"
if [ ! -d "$OPENSSL_PREBUILT_FOLDER" ]; then
    echo "Cloning and building OpenSSL..."
    if [ ! -d "openssl" ]; then
        git clone https://github.com/openssl/openssl.git
    fi
    cd openssl
    mkdir -p "${OPENSSL_PREBUILT_FOLDER}/include"

    # ios-arm
    ./Configure ios64-xcrun no-shared no-asm no-tests
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/iphoneos && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/iphoneos"
    cp -r include/crypto include/openssl "${OPENSSL_PREBUILT_FOLDER}/include"
    make clean

    # arm-simulator
    ./Configure iossimulator-arm64-xcrun no-shared no-asm no-tests
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/iphonesimulator && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/iphonesimulator"
    make clean

    # x86_64-simulator
    ./Configure iossimulator-x86_64-xcrun no-shared no-asm no-tests
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/iphonesimulator-x86_64 && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/iphonesimulator-x86_64"
    make clean

    # arm64-v8a
    ./Configure android-arm64 no-shared no-asm no-tests -D__ANDROID_API__=${API_LEVEL}
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/arm64-v8a && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/arm64-v8a"
    make clean

    # armeabi-v7a
    ./Configure android-arm no-shared no-asm no-tests -D__ANDROID_API__=${API_LEVEL}
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a"
    make clean

    ./Configure android-x86 no-shared no-asm no-tests -D__ANDROID_API__=${API_LEVEL}
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/x86 && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/x86"
    make clean

    ./Configure android-x86_64 no-shared no-asm no-tests -D__ANDROID_API__=${API_LEVEL}
    make build_libs -j10
    mkdir -p ${OPENSSL_PREBUILT_FOLDER}/x86_64 && cp libcrypto.a libssl.a "${OPENSSL_PREBUILT_FOLDER}/x86_64"
    make clean

    cd .. && rm -rf openssl
fi

# iOS Architectures
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Building for iOS architectures..."
    
    # iOS SDK paths
    IOS_SDK_PATH=$(xcrun --sdk iphoneos --show-sdk-path)
    IOS_SIM_SDK_PATH=$(xcrun --sdk iphonesimulator --show-sdk-path)
    
    # iOS Device architecture
    build_arch "arm64" "darwin" \
        "$(xcrun --sdk iphoneos --find clang)" \
        "$(xcrun --sdk iphoneos --find clang++)" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -arch arm64 -mios-version-min=11.0 -isysroot ${IOS_SDK_PATH}" \
        "-L${OPENSSL_PREBUILT_FOLDER}/iphoneos -arch arm64 -mios-version-min=11.0 -isysroot ${IOS_SDK_PATH}" \
        "--disable-iconv --disable-zlib --enable-openssl --disable-securetransport"

    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphoneos/lib/libavcodec.${AVCODEC_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphoneos/lib/libavformat.${AVFORMAT_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphoneos/lib/libavutil.${AVUTIL_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphoneos/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib"
    
    rm -rf "${OUTPUT_DIR}/ios/iphoneos/share"

    # iOS Simulator arm (Silicon Macs)
    build_arch "arm64" "darwinsim" \
        "$(xcrun --sdk iphonesimulator --find clang)" \
        "$(xcrun --sdk iphonesimulator --find clang++)" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -arch arm64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "-L${OPENSSL_PREBUILT_FOLDER}/iphonesimulator -arch arm64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "--disable-iconv --disable-zlib --enable-openssl --disable-securetransport"

    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavcodec.${AVCODEC_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavformat.${AVFORMAT_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavutil.${AVUTIL_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib"

    rm -rf "${OUTPUT_DIR}/ios/iphonesimulator_arm64/share"

    # iOS Simulator x86_64 (Intel Macs)
    build_arch "x86_64" "darwinsim" \
        "$(xcrun --sdk iphonesimulator --find clang)" \
        "$(xcrun --sdk iphonesimulator --find clang++)" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -arch x86_64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "-L${OPENSSL_PREBUILT_FOLDER}/iphonesimulator-x86_64 -arch x86_64 -mios-simulator-version-min=11.0 -isysroot ${IOS_SIM_SDK_PATH}" \
        "--disable-iconv --disable-zlib --enable-openssl --disable-securetransport"

    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavcodec.${AVCODEC_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavformat.${AVFORMAT_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavutil.${AVUTIL_VERSION}.dylib"
    fix_dynamic_ios_linkage "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib"

    rm -rf "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/share"

    mkdir -p "${OUTPUT_DIR}/ios/iphonesimulator/lib"
    # Create universal simulator libraries
    lipo -create \
        "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavcodec.${AVCODEC_VERSION}.dylib" \
        "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavcodec.${AVCODEC_VERSION}.dylib" \
        -output "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavcodec.${AVCODEC_VERSION}.dylib"
    lipo -create \
        "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavformat.${AVFORMAT_VERSION}.dylib" \
        "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavformat.${AVFORMAT_VERSION}.dylib" \
        -output "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavformat.${AVFORMAT_VERSION}.dylib"
    
    lipo -create \
        "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libavutil.${AVUTIL_VERSION}.dylib" \
        "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libavutil.${AVUTIL_VERSION}.dylib" \
        -output "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavutil.${AVUTIL_VERSION}.dylib" 
       
    lipo -create \
        "${OUTPUT_DIR}/ios/iphonesimulator_arm64/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib" \
        "${OUTPUT_DIR}/ios/iphonesimulator_x86_64/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib" \
        -output "${OUTPUT_DIR}/ios/iphonesimulator/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib"

    mv "${OUTPUT_DIR}/ios/iphonesimulator/lib/libswresample.${SWRRESAMPLE_VERSION}.dylib" "${OUTPUT_DIR}/ios/iphonesimulator/lib/libswresample.dylib"
    mv "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavutil.${AVUTIL_VERSION}.dylib" "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavutil.dylib"
    mv "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavformat.${AVFORMAT_VERSION}.dylib" "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavformat.dylib"
    mv "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavcodec.${AVCODEC_VERSION}.dylib" "${OUTPUT_DIR}/ios/iphonesimulator/lib/libavcodec.dylib"

    # create frameworks from binaries
    bash ./create_xcframework.sh
    
    echo "iOS builds completed!"
else
    echo "Skipping iOS builds (requires macOS)"
fi

# Android Architectures
if [ -n "$ANDROID_NDK_ROOT" ] || [ -n "$NDK_ROOT" ]; then
    echo "Building for Android architectures..."
        
    # ARM64-v8a
    build_arch "aarch64" "android" \
        "${TOOLCHAIN}/bin/aarch64-linux-android${API_LEVEL}-clang" \
        "${TOOLCHAIN}/bin/aarch64-linux-android${API_LEVEL}-clang++" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/sysroot/usr/include -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "-L${OPENSSL_PREBUILT_FOLDER}/arm64-v8a -L${TOOLCHAIN}/sysroot/usr/lib/aarch64-linux-android/${API_LEVEL} -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "--enable-openssl --extra-libs=-lz"

    rm -rf ${OUTPUT_DIR}/android/arm64-v8a/share
    
    # ARMv7a
    build_arch "armv7a" "android" \
        "${TOOLCHAIN}/bin/armv7a-linux-androideabi${API_LEVEL}-clang" \
        "${TOOLCHAIN}/bin/armv7a-linux-androideabi${API_LEVEL}-clang++" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/sysroot/usr/include -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "-L${OPENSSL_PREBUILT_FOLDER}/armeabi-v7a -L${TOOLCHAIN}/sysroot/usr/lib/arm-linux-android/${API_LEVEL} -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "--enable-openssl --extra-libs=-lz"

    rm -rf ${OUTPUT_DIR}/android/armeabi-v7a/share

    # x86
    build_arch "x86" "android" \
        "${TOOLCHAIN}/bin/i686-linux-android${API_LEVEL}-clang" \
        "${TOOLCHAIN}/bin/i686-linux-android${API_LEVEL}-clang++" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/darwin-x86_64/sysroot/usr/include -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "-L${OPENSSL_PREBUILT_FOLDER}/x86 -L${TOOLCHAIN}/darwin-x86_64/sysroot/usr/lib/i686-linux-android/${API_LEVEL} -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "--enable-openssl --extra-libs=-lz"

    rm -rf ${OUTPUT_DIR}/android/x86/share


    # x86_64
    build_arch "x86_64" "android" \
        "${TOOLCHAIN}/bin/x86_64-linux-android${API_LEVEL}-clang" \
        "${TOOLCHAIN}/bin/x86_64-linux-android${API_LEVEL}-clang++" \
        "-I${OPENSSL_PREBUILT_FOLDER}/include -I${TOOLCHAIN}/darwin-x86_64/sysroot/usr/include -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "-L${OPENSSL_PREBUILT_FOLDER}/x86_64 -L${TOOLCHAIN}/darwin-x86_64/sysroot/usr/lib/x86_64-linux-android/${API_LEVEL} -fPIC -Wl,-Bsymbolic -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384" \
        "--enable-openssl --extra-libs=-lz"

    rm -rf ${OUTPUT_DIR}/android/x86_64/share

        
    echo "Android builds completed!"
else
    echo "Skipping Android builds (ANDROID_NDK_ROOT or NDK_ROOT not set)"
fi

rm -rf "${BUILD_DIR}"

echo "All FFmpeg builds completed!"
