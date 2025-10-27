## If you would like to relink ffmpeg to your implementation there are two options:

### Option A)

you can modify script in scripts/download-prebuilt-binaries.sh

- ios dynamic frameworks are in `ffmpeg_ios.zip` directory
- android shared libraries are in `jniLibs.zip`

just replace way of downloading them that links to your binaries

### Option B)

directly modify libraries in source code

- ios dynamic frameworks are placed in `common/cpp/audioapi/external`
- android shared libraries are placed in `android/src/main/jniLibs`

---
`USED_LIBRARIES`:
- libavcodec
- libavformat
- libavutil
- libswresample
