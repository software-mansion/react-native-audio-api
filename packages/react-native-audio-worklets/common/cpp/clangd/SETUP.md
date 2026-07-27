`CMakeLists.txt` in this directory can be used to build the C++ side and generate `compile_commands.json` for the VS Code / clangd C++ extension.

**Prerequisites**

1. Install dependencies from the monorepo root (`yarn install`).
2. Generate TurboModule codegen headers once (either is enough):
   - Android: `cd packages/react-native-audio-worklets/android && ./gradlew generateCodegenArtifactsFromSchema`
   - iOS: build `apps/fabric-example` once, or run `pod install` in that app
3. For full IntelliSense on TurboModule files (`NativeAudioWorkletsModule.cpp`), run `pod install` in `apps/fabric-example/ios` so Folly/React-Core headers are available under `Pods/Headers/Public`.

**Generate compile_commands.json**

From this directory (`common/cpp/clangd`):

```bash
./generate-and-copy.sh
```

This writes `compile_commands.json` to the package root and merges it into the
repo-root `compile_commands.json`.
