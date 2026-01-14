#!/bin/bash
# Script to download and unzip prebuilt native binaries for React Native.

MAIN_DOWNLOAD_URL="https://github.com/software-mansion-labs/rn-audio-libs/releases/download"
TAG="v2.0.0"
DOWNLOAD_NAMES=(
    "armeabi-v7a.zip" "arm64-v8a.zip" "x86.zip" "x86_64.zip"
    "ffmpeg_ios.zip" "iphoneos.zip" "iphonesimulator.zip" "jniLibs.zip"
)

# Use a temporary directory for downloads, ensuring it exists
TEMP_DOWNLOAD_DIR="$(pwd)/audioapi-binaries-temp"
mkdir -p "$TEMP_DOWNLOAD_DIR"

if [ "$1" == "android" ]; then
    PROJECT_ROOT="$(pwd)/.."
else
    PROJECT_ROOT="$(pwd)"
fi

if [ "$2" == "skipffmpeg" ]; then
    SKIP_FFMPEG=true
else
    SKIP_FFMPEG=false
fi

JNILIBS_DESTINATION="${PROJECT_ROOT}/android/src/main"
NORMAL_DESTINATION="${PROJECT_ROOT}/common/cpp/audioapi/external"

for name in "${DOWNLOAD_NAMES[@]}"; do
    ARCH_URL="${MAIN_DOWNLOAD_URL}/${TAG}/${name}"
    ZIP_FILE_PATH="${TEMP_DOWNLOAD_DIR}/${name}"

    # Get the directory name from the zip name (e.g., "armeabi-v7a.zip" -> "armeabi-v7a")
    EXTRACTED_DIR_NAME="${name%.zip}"

    if [[ ("$EXTRACTED_DIR_NAME" == "ffmpeg_ios" || "$EXTRACTED_DIR_NAME" == "jniLibs") && "$SKIP_FFMPEG" == true ]]; then
        continue
    fi

    # Determine the final output path
    if [[ "$name" == "jniLibs.zip" ]]; then
        OUTPUT_DIR="${JNILIBS_DESTINATION}"
    else
        OUTPUT_DIR="${NORMAL_DESTINATION}"
    fi
    FINAL_CHECK_PATH="${OUTPUT_DIR}/${EXTRACTED_DIR_NAME}"

    if [ -d "$FINAL_CHECK_PATH" ]; then
        continue
    fi

    # If we are here, the directory does not exist, so we download and unzip.
    echo "Downloading from: $ARCH_URL"
    curl -fsSL "$ARCH_URL" -o "$ZIP_FILE_PATH"

    if [ $? -ne 0 ]; then
        echo "Error: Download failed for ${name}."
        rm -f "$ZIP_FILE_PATH"
        continue
    fi

    echo "Unzipping ${name} to ${OUTPUT_DIR}"
    unzip -o "$ZIP_FILE_PATH" -d "$OUTPUT_DIR"

    # Clean up any __MACOSX directories that may have been created
    rm -rf "${OUTPUT_DIR}/__MACOSX"

done

rm -rf "$TEMP_DOWNLOAD_DIR"
