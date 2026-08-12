`CMakeLists.txt` in this directory can be used to build the C++ side and generate `compile_commands.json` for clangd (VS Code, Cursor, etc.).

**Generate compile_commands.json**

From this directory (`common/cpp/clangd`):

```bash
./generate-and-copy.sh
```

This generates entries for `common/cpp`, the Android JNI glue (`android/src/main/cpp/audioapi`), and the iOS Objective-C++ glue (`ios/audioapi/ios`), then copies the result to the repo root.

**Prerequisites for full header resolution**

- **Android**: `fbjni`/`oboe`/`react-android` headers are resolved from your local Gradle dependency cache (`~/.gradle/caches`, or `$GRADLE_USER_HOME`), and the NDK sysroot from `$ANDROID_HOME/ndk`. Build the Android app (or open it in Android Studio) at least once to populate the cache — a missing cache just means fewer include paths, not a hard failure.
- **iOS**: React/Pod headers are reused from whatever `apps/fabric-example/ios/Pods` already has, via its generated xcconfig. Run `pod install` there at least once (see the [build skill](../../../../.claude/skills/build-compilation-dependencies/SKILL.md)).
