# Maintenance — build-compilation-dependencies

> Used by `/pre-push-update` only — not loaded when the `build-compilation-dependencies` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `android/CMakeLists.txt` | SIMD detection, RN version flag workaround |
| `android/src/main/cpp/audioapi/CMakeLists.txt` | Source glob patterns, FFmpeg exclusion, DSP `-O3` targets, IMPORTED lib list, include paths |
| `android/build.gradle` | Feature flag detection, CMake arg forwarding, BuildConfig fields, worklets task dependency, packaging options |
| `RNAudioAPI.podspec` | Subspecs table, `miniaudio_impl` workaround, `-force_load` list, xcframeworks list, `rnaa_utils.rb` dynamic paths |
| `apps/fabric-example/ios/Podfile` | New Architecture enablement, minimum iOS version helper |
| `common/cpp/test/CMakeLists.txt` | Excluded sources list, compile definitions, GoogleTest fetch URL, include paths |
| `common/cpp/test/src/MockAudioEventHandlerRegistry.h` | Mock interface — update fixture boilerplate in `build-details.md` if signature changes |
| `scripts/download-prebuilt-binaries.sh` | New download artifacts, new TAG version |
