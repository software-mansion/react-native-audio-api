#!/bin/bash
# Script to download and unzip prebuilt native binaries for React Native.

MAIN_DOWNLOAD_URL="https://github.com/software-mansion-labs/rn-audio-libs/releases/download"
TAG="v3.1.0"
ANDROID_DOWNLOAD_NAMES=(
    "android.zip"
    "jniLibs.zip"
)
IOS_DOWNLOAD_NAMES=(
    "ffmpeg_ios.zip"
    "iphoneos.zip"
    "iphonesimulator.zip"
    "macosx.zip"
)
STATIC_LIB_NAMES=(
    "libopusfile.a"
    "libopus.a"
    "libogg.a"
    "libvorbis.a"
    "libvorbisenc.a"
    "libvorbisfile.a"
)
FFMPEG_IOS_FRAMEWORKS=(
    "libavcodec.xcframework"
    "libavformat.xcframework"
    "libavutil.xcframework"
    "libswresample.xcframework"
)
FFMPEG_ANDROID_SO_LIBS=(
    "libavcodec.so"
    "libavformat.so"
    "libavutil.so"
    "libswresample.so"
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

# Use a temporary directory for downloads, ensuring it exists
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Use a temporary directory for downloads, ensuring it exists
TEMP_DOWNLOAD_DIR="${SCRIPT_DIR}/audioapi-binaries-temp"

if [ "$PLATFORM" == "android" ]; then
    DOWNLOAD_NAMES=("${ANDROID_DOWNLOAD_NAMES[@]}")
else
    DOWNLOAD_NAMES=("${IOS_DOWNLOAD_NAMES[@]}")
fi

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

restore_versioned_framework_symlinks() {
    local ffmpeg_ios_dir="${NORMAL_DESTINATION}/ffmpeg_ios"

    if [ ! -d "$ffmpeg_ios_dir" ]; then
        return
    fi

    find "$ffmpeg_ios_dir" -path "*/Versions/A" -type d | while read -r version_dir; do
        local framework_dir
        framework_dir="$(cd "${version_dir}/../.." && pwd)"
        local framework_name
        framework_name="$(basename "$framework_dir" .framework)"

        if [ -f "${version_dir}/${framework_name}" ]; then
            (
                cd "$framework_dir"
                ln -sfn A Versions/Current
                ln -sfn "Versions/Current/${framework_name}" "$framework_name"

                if [ -d "Versions/A/Resources" ]; then
                    ln -sfn "Versions/Current/Resources" Resources
                fi

                if [ -d "Versions/A/Headers" ]; then
                    ln -sfn "Versions/Current/Headers" Headers
                fi
            )
        fi
    done
}

artifacts_present() {
    local extracted_dir_name="$1"
    local final_check_path="$2"
    local abi
    local lib
    local framework
    local so_lib

    case "$extracted_dir_name" in
        ffmpeg_ios)
            for framework in "${FFMPEG_IOS_FRAMEWORKS[@]}"; do
                if [ ! -d "${final_check_path}/${framework}" ]; then
                    return 1
                fi
            done
            ;;
        jniLibs)
            for abi in "${ANDROID_JNI_ABIS[@]}"; do
                for so_lib in "${FFMPEG_ANDROID_SO_LIBS[@]}"; do
                    if [ ! -f "${final_check_path}/${abi}/${so_lib}" ]; then
                        return 1
                    fi
                done
            done
            ;;
        android)
            for abi in "${ANDROID_JNI_ABIS[@]}"; do
                for lib in "${STATIC_LIB_NAMES[@]}"; do
                    if [ ! -f "${final_check_path}/${abi}/${lib}" ]; then
                        return 1
                    fi
                done
            done
            ;;
        iphoneos|iphonesimulator|macosx)
            for lib in "${STATIC_LIB_NAMES[@]}"; do
                if [ ! -f "${final_check_path}/${lib}" ]; then
                    return 1
                fi
            done
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

    # Get the directory name from the zip name (e.g., "armeabi-v7a.zip" -> "armeabi-v7a")
    EXTRACTED_DIR_NAME="${name%.zip}"

    if [[ ("$EXTRACTED_DIR_NAME" == "ffmpeg_ios" || "$EXTRACTED_DIR_NAME" == "jniLibs") && "$SKIP_FFMPEG" == true ]]; then
        continue
    fi

    if [[ ("$EXTRACTED_DIR_NAME" == "android" || "$EXTRACTED_DIR_NAME" == "iphoneos" || "$EXTRACTED_DIR_NAME" == "iphonesimulator" || "$EXTRACTED_DIR_NAME" == "macosx") && "$SKIP_STATIC" == true ]]; then
        continue
    fi

    # Determine the final output path
    if [[ "$name" == "jniLibs.zip" ]]; then
        OUTPUT_DIR="${JNILIBS_DESTINATION}"
    else
        OUTPUT_DIR="${NORMAL_DESTINATION}"
    fi
    FINAL_CHECK_PATH="${OUTPUT_DIR}/${EXTRACTED_DIR_NAME}"

    if artifacts_present "$EXTRACTED_DIR_NAME" "$FINAL_CHECK_PATH"; then
        continue
    fi

    # If we are here, the artifacts are missing, so we download and unzip.
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

if [ "$PLATFORM" == "ios" ]; then
    restore_versioned_framework_symlinks
fi

rm -rf "$TEMP_DOWNLOAD_DIR"
