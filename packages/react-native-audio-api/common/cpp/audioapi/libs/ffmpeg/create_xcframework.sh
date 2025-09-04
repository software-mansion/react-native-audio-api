#!/usr/bin/env bash

# Creates an Info.plist file for a given framework:
build_info_plist() {
    local file_path="$1"
    local framework_name="$2"
    local framework_id="$3"

    # Minimum version must be the same we used when building FFmpeg.
    local minimum_version_key="MinimumOSVersion"
    local minimum_os_version="16.0"

    local supported_platforms="iPhoneOS"

    info_plist="<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>${framework_name}</string>
    <key>CFBundleIdentifier</key>
    <string>${framework_id}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${framework_name}</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleShortVersionString</key>
    <string>7.0.2</string>
    <key>CFBundleVersion</key>
    <string>7.0.2</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>${minimum_version_key}</key>
    <string>${minimum_os_version}</string>
    <key>CFBundleSupportedPlatforms</key>
    <array>
        <string>${supported_platforms}</string>
    </array>
    <key>NSPrincipalClass</key>
    <string></string>
</dict>
</plist>"
    echo $info_plist | tee ${file_path} 1>/dev/null
}

dylib_regex="^@rpath/.*\.dylib$"

# Creates framework from a dylib file:
create_framework() {
    local framework_name="$1"
    local ffmpeg_library_path="$2"
    local framework_complete_path="${ffmpeg_library_path}/framework/${framework_name}.framework/${framework_name}"

    # Create framework directory and copy dylib file to this directory:
    mkdir -p "${ffmpeg_library_path}/framework/${framework_name}.framework"
    cp "${ffmpeg_library_path}/lib/${framework_name}.dylib" "${ffmpeg_library_path}/framework/${framework_name}.framework/${framework_name}"

    # Change the shared library identification name, removing version number and 'dylib' extension;
    # \c Frameworks part of the name is needed since this is where frameworks will be installed in
    # an application bundle:
    install_name_tool -id @rpath/Frameworks/${framework_name}.framework/${framework_name} "${framework_complete_path}"

    # Add Info.plist file into the framework directory:
    build_info_plist "${ffmpeg_library_path}/framework/${framework_name}.framework/Info.plist" "${framework_name}" "io.qt.ffmpegkit."${framework_name}
    otool -L "$framework_complete_path" | awk '/\t/ {print $1}' | egrep "$dylib_regex" | while read -r dependency_path; do
        found_name=$(tmp=${dependency_path/*\/}; echo ${tmp/\.*})
        if [ "$found_name" != "$framework_name" ]
        then
            # Change the dependent shared library install name to remove version number and 'dylib' extension:
            install_name_tool -change "$dependency_path" @rpath/Frameworks/${found_name}.framework/${found_name} "${framework_complete_path}"
        fi
    done
}

ffmpeg_libs="libavcodec libavformat libavutil libswresample"

for name in $ffmpeg_libs; do
    create_framework $name output/ios/iphoneos
    create_framework $name output/ios/iphonesimulator
done

# Create XCFramework:
OUTPUT_DIR="output/ios"

xcodebuild -create-xcframework \
    -framework "${OUTPUT_DIR}/iphoneos/framework/libavcodec.framework" \
    -framework "${OUTPUT_DIR}/iphonesimulator/framework/libavcodec.framework" \
    -output "${OUTPUT_DIR}/libavcodec.xcframework"

xcodebuild -create-xcframework \
    -framework "${OUTPUT_DIR}/iphoneos/framework/libavformat.framework" \
    -framework "${OUTPUT_DIR}/iphonesimulator/framework/libavformat.framework" \
    -output "${OUTPUT_DIR}/libavformat.xcframework"

xcodebuild -create-xcframework \
    -framework "${OUTPUT_DIR}/iphoneos/framework/libavutil.framework" \
    -framework "${OUTPUT_DIR}/iphonesimulator/framework/libavutil.framework" \
    -output "${OUTPUT_DIR}/libavutil.xcframework"

xcodebuild -create-xcframework \
    -framework "${OUTPUT_DIR}/iphoneos/framework/libswresample.framework" \
    -framework "${OUTPUT_DIR}/iphonesimulator/framework/libswresample.framework" \
    -output "${OUTPUT_DIR}/libswresample.xcframework"




