# Maintenance — turbo-modules

> Used by `/pre-push-update` only — not loaded when the `turbo-modules` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `src/specs/NativeAudioAPIModule.ts` | Spec interface — new methods, changed signatures |
| `common/cpp/audioapi/AudioAPIModuleInstaller.h` | New globals injected, new HostObjects installed |
| `ios/audioapi/ios/AudioAPIModule.mm` | iOS install flow, new platform managers, new RCT_EXPORT_METHOD |
| `android/src/main/java/.../AudioAPIModule.kt` | Android `install()`, new `external fun`, lifecycle changes |
| `android/src/main/cpp/audioapi/android/AudioAPIModule.cpp` | New `registerNatives` entries |

Update the **install flow diagram** when new globals are added to `injectJSIBindings`.
