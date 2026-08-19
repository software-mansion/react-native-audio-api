`CMakeLists.txt` in this directory can be used to build the C++ side and generate `compile_commands.json` for clangd (VS Code, Cursor, etc.).

**Generate compile_commands.json**

```bash
yarn setup:clangd
```

Generates entries for `common/cpp` (sources + headers), the iOS Objective-C++/Objective-C glue (`ios/audioapi/ios`), and merges in Android's real NDK-compiled entries from `android/.cxx` (see `generate-and-copy.sh`), then copies the result to the repo root.

**Prerequisites**

- **Android**: needs a real Gradle build's `android/.cxx` output — this CMakeLists.txt can't safely reproduce NDK cross-compile flags itself.
- **iOS**: needs `apps/fabric-example/ios/Pods` — reuses its generated xcconfig for React/Pod header paths.

If either is missing or stale (e.g. after a dependency bump), refresh both first:

```bash
yarn setup:clangd:clean
```

This runs `pod deintegrate && pod install` and a full Android build, then regenerates — slow, only needed when native deps/config actually change.
