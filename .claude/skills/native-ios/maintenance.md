# Maintenance — native-ios

> Used by `/pre-push-update` only — not loaded when the `native-ios` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `ios/audioapi/ios/**/*.mm` | New ObjC++ patterns, new system API usage, new version guards |
| `ios/audioapi/ios/**/*.h` | New interfaces, new platform types |
| `ios/audioapi/ios/system/AudioSessionManager.*` | Session configuration options, permission flow |
| `ios/audioapi/ios/system/AudioEngine.*` | Interruption recovery pattern, node attach/detach |
| `ios/audioapi/ios/AudioAPIModule.mm` | `install()` flow, CallInvoker source, new platform managers |
| `RNAudioAPI.podspec` | Build config changes (also see `build-compilation-dependencies` skill) |
