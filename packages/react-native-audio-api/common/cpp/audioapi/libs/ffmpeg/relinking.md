## If you would like to relink ffmpeg to your implementation there are two options:

### Option A)

you can modify script in scripts/download-prebuilt-binaries.sh

- ios dynamic frameworks are one zip per library (`libavcodec.xcframework.zip`, `libavformat.xcframework.zip`, `libavutil.xcframework.zip`, `libswresample.xcframework.zip`), each unzipped straight into `common/cpp/audioapi/external/xcframeworks/`
- android shared libraries are in `jniLibs.zip`

just replace way of downloading them that links to your binaries

SwiftPM builds skip the script: edit the `binaryTarget` URLs and checksums in `Package.swift` instead.

### Option B)

replace the binaries in place

- ios: `common/cpp/audioapi/external/xcframeworks/*.xcframework`
- android: `android/src/main/jniLibs/<abi>/*.so`

---
`USED_LIBRARIES`:
- libavcodec
- libavformat
- libavutil
- libswresample
