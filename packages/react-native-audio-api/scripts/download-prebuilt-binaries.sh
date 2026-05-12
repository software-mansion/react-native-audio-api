#!/bin/bash
# Script to download and unzip prebuilt native binaries for React Native.

MAIN_DOWNLOAD_URL="https://github.com/software-mansion-labs/rn-audio-libs/releases/download"
TAG="v3.1.0"
DOWNLOAD_NAMES=(
    "android.zip"
    "ffmpeg_ios.zip"
    "iphoneos.zip"
    "iphonesimulator.zip"
    "jniLibs.zip"
    "macosx.zip"
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

restore_versioned_framework_symlinks

rm -rf "$TEMP_DOWNLOAD_DIR"
