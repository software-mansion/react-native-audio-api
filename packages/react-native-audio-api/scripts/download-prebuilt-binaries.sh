#!/bin/bash
# Script to download and unzip prebuilt native binaries for React Native.

MAIN_DOWNLOAD_URL="https://github.com/software-mansion-labs/rn-audio-libs/releases/download"
TAG="v1.0.0"
DOWNLOAD_NAMES=(
    "armeabi-v7a.zip" "arm64-v8a.zip" "x86.zip" "x86_64.zip"
    "ffmpeg_ios.zip" "iphoneos.zip" "iphonesimulator.zip" "jniLibs.zip"
)

# Use a temporary directory for downloads, ensuring it exists
TEMP_DOWNLOAD_DIR="$(pwd)/audioapi-binaries-temp"
mkdir -p "$TEMP_DOWNLOAD_DIR"
if [ $1 == "android" ]
then
    PROJECT_ROOT="$(pwd)/.."
else
    PROJECT_ROOT="$(pwd)"
fi

for name in "${DOWNLOAD_NAMES[@]}"; do
    ARCH_URL="${MAIN_DOWNLOAD_URL}/${TAG}/${name}"
    ZIP_FILE_PATH="${TEMP_DOWNLOAD_DIR}/${name}"

    if [[ "$name" == "jniLibs.zip" ]]; then
        OUTPUT_DIR="${PROJECT_ROOT}/android/src/main"
    else
        OUTPUT_DIR="${PROJECT_ROOT}/common/cpp/audioapi/external"
    fi

    mkdir -p "$OUTPUT_DIR"

    if [ -f "$ZIP_FILE_PATH" ]; then
        echo "Zip file already exists locally. Skipping download."
        continue
    fi
    echo "Downloading from: $ARCH_URL"
    curl -fsSL "$ARCH_URL" -o "$ZIP_FILE_PATH"

    if [ $? -ne 0 ]; then
        rm -f "$ZIP_FILE_PATH"
        continue
    fi

    unzip -o "$ZIP_FILE_PATH" -d "$OUTPUT_DIR"

    # Clean up any __MACOSX directories that may have been created
    rm -rf "${OUTPUT_DIR}/__MACOSX"

done

rm -rf "$TEMP_DOWNLOAD_DIR"
