#!/bin/bash
# Script to download and unzip prebuilt native binaries for React Native.

MAIN_DOWNLOAD_URL="https://github.com/software-mansion-labs/rn-audio-libs/releases/download"
# Package.swift pins the same tag for its SwiftPM binary targets; bump both together.
TAG="v4.0.0"
FFMPEG_HEADERS_ARCHIVE="include_ffmpeg.zip"
STATIC_LIB_NAMES=(
    "libopusfile"
    "libopus"
    "libogg"
    "libvorbis"
    "libvorbisenc"
    "libvorbisfile"
)
FFMPEG_LIB_NAMES=(
    "libavcodec"
    "libavformat"
    "libavutil"
    "libswresample"
)
# iOS binaries ship as one zipped <lib>.xcframework per library.
# One umbrella header per library proves the header set was extracted.
FFMPEG_HEADER_MARKERS=(
    "libavcodec/avcodec.h"
    "libavformat/avformat.h"
    "libavutil/avutil.h"
    "libswresample/swresample.h"
)
ANDROID_JNI_ABIS=(
    "armeabi-v7a"
    "arm64-v8a"
    "x86_64"
    "x86"
)

PLATFORM="$1"
if [ "$PLATFORM" != "android" ] && [ "$PLATFORM" != "ios" ]; then
    echo "Error: platform argument required. Usage: $0 <android|ios> [skipffmpeg]"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Use a temporary directory for downloads, ensuring it exists
TEMP_DOWNLOAD_DIR="${SCRIPT_DIR}/audioapi-binaries-temp"
mkdir -p "$TEMP_DOWNLOAD_DIR"

if [ "$2" == "skipffmpeg" ]; then
    SKIP_FFMPEG=true
else
    SKIP_FFMPEG=false
fi

if [ "${DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS}" == "1" ]; then
    SKIP_STATIC=true
else
    SKIP_STATIC=false
fi

JNILIBS_DESTINATION="${PROJECT_ROOT}/android/src/main"
NORMAL_DESTINATION="${PROJECT_ROOT}/common/cpp/audioapi/external"
XCFRAMEWORKS_DESTINATION="${NORMAL_DESTINATION}/xcframeworks"

DOWNLOAD_NAMES=()
if [ "$PLATFORM" == "android" ]; then
    if [ "$SKIP_STATIC" == false ]; then
        DOWNLOAD_NAMES+=("android.zip")
    fi
    if [ "$SKIP_FFMPEG" == false ]; then
        DOWNLOAD_NAMES+=("jniLibs.zip" "$FFMPEG_HEADERS_ARCHIVE")
    fi
else
    if [ "$SKIP_STATIC" == false ]; then
        for lib in "${STATIC_LIB_NAMES[@]}"; do
            DOWNLOAD_NAMES+=("${lib}.xcframework.zip")
        done
    fi
    if [ "$SKIP_FFMPEG" == false ]; then
        for lib in "${FFMPEG_LIB_NAMES[@]}"; do
            DOWNLOAD_NAMES+=("${lib}.xcframework.zip")
        done
        # The FFmpeg frameworks carry no headers; CocoaPods adds this directory to the search path.
        DOWNLOAD_NAMES+=("$FFMPEG_HEADERS_ARCHIVE")
    fi
fi

artifacts_present() {
    local extracted_name="$1"
    local final_check_path="$2"
    local abi
    local lib
    local so_lib

    case "$extracted_name" in
        include_ffmpeg)
            for header in "${FFMPEG_HEADER_MARKERS[@]}"; do
                if [ ! -f "${final_check_path}/${header}" ]; then
                    return 1
                fi
            done
            ;;
        jniLibs)
            for abi in "${ANDROID_JNI_ABIS[@]}"; do
                for so_lib in "${FFMPEG_LIB_NAMES[@]}"; do
                    if [ ! -f "${final_check_path}/${abi}/${so_lib}.so" ]; then
                        return 1
                    fi
                done
            done
            ;;
        android)
            for abi in "${ANDROID_JNI_ABIS[@]}"; do
                for lib in "${STATIC_LIB_NAMES[@]}"; do
                    if [ ! -f "${final_check_path}/${abi}/${lib}.a" ]; then
                        return 1
                    fi
                done
            done
            ;;
        *.xcframework)
            [ -f "${final_check_path}/Info.plist" ]
            return
            ;;
        *)
            [ -d "$final_check_path" ]
            return
            ;;
    esac

    return 0
}

for name in "${DOWNLOAD_NAMES[@]}"; do
    ARCH_URL="${MAIN_DOWNLOAD_URL}/${TAG}/${name}"
    ZIP_FILE_PATH="${TEMP_DOWNLOAD_DIR}/${name}"

    # Each archive unpacks to one top-level entry named after it (e.g. "android.zip" -> "android",
    # "libopus.xcframework.zip" -> "libopus.xcframework").
    EXTRACTED_NAME="${name%.zip}"

    if [[ "$name" == "jniLibs.zip" ]]; then
        OUTPUT_DIR="${JNILIBS_DESTINATION}"
    elif [[ "$EXTRACTED_NAME" == *.xcframework ]]; then
        OUTPUT_DIR="${XCFRAMEWORKS_DESTINATION}"
    else
        OUTPUT_DIR="${NORMAL_DESTINATION}"
    fi
    FINAL_CHECK_PATH="${OUTPUT_DIR}/${EXTRACTED_NAME}"

    if artifacts_present "$EXTRACTED_NAME" "$FINAL_CHECK_PATH"; then
        continue
    fi

    # If we are here, the artifacts are missing, so we download and unzip.
    echo "Downloading from: $ARCH_URL"
    curl -fsSL "$ARCH_URL" -o "$ZIP_FILE_PATH"

    if [ $? -ne 0 ]; then
        # The build cannot succeed without these files, so fail here where the cause is
        # visible instead of at compile/link time with a missing-header or symbol error.
        echo "Error: Download failed for ${ARCH_URL}." >&2
        rm -f "$ZIP_FILE_PATH"
        rm -rf "$TEMP_DOWNLOAD_DIR"
        exit 1
    fi

    echo "Unzipping ${name} to ${OUTPUT_DIR}"
    mkdir -p "$OUTPUT_DIR"
    # A stale bundle from an interrupted run would otherwise mix with the fresh one.
    if [[ "$EXTRACTED_NAME" == *.xcframework ]]; then
        rm -rf "${OUTPUT_DIR:?}/${EXTRACTED_NAME}"
    fi
    unzip -o "$ZIP_FILE_PATH" -d "$OUTPUT_DIR"

    # Clean up any __MACOSX directories that may have been created
    rm -rf "${OUTPUT_DIR}/__MACOSX"

done

rm -rf "$TEMP_DOWNLOAD_DIR"
