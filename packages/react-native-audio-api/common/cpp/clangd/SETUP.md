`CMakeLists.txt` in this directory generates `compile_commands.json` for clangd (any editor with a clangd integration) and for `pnpm lint:cpp` (clang-tidy). Nothing is built from it.

**Generate compile_commands.json**

```bash
pnpm setup:clangd
```

Generates entries for `common/cpp` (sources + headers), the iOS Objective-C++/Objective-C glue (`ios/audioapi/ios`), the example app's XCTests (`apps/fabric-example/ios/FabricExampleTests`), and merges in Android's real NDK-compiled entries from `android/.cxx` (see `generate-and-copy.sh`), then copies the result to the repo root. It also fetches the FFmpeg headers if they are missing (`download-prebuilt-binaries.sh headers`). `pnpm setup:clangd` runs the worklets project afterwards.

**Prerequisites**

- **Android**: needs a real Gradle build's `android/.cxx` output — this CMakeLists.txt can't safely reproduce NDK cross-compile flags itself.
- **iOS**: needs `apps/fabric-example/ios/Pods` — reuses its generated xcconfig for React/Pod header
  _and framework_ search paths. Since RN 0.87 React-Core ships prebuilt as `React.xcframework`, so
  `#import <React/...>` resolves via `-F`, not `-I`; missing framework paths show up as
  `'React/RCTBridgeModule.h' file not found` in every `.mm` that imports React.

If either is missing or stale (e.g. after a dependency bump), refresh both first:

```bash
pnpm setup:clangd:clean
```

This runs `pod deintegrate && pod install` and a full Android build, then regenerates — slow, only needed when native deps/config actually change.
