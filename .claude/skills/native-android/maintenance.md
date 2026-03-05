# Maintenance — native-android

> Used by `/pre-push-update` only — not loaded when the `native-android` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `android/src/main/java/**/*.kt` | New Kotlin patterns, new Android API usage, new version guards |
| `android/src/main/cpp/audioapi/android/AudioAPIModule.*` | JNI event path, `registerNatives` entries, `initHybrid` flow |
| `android/src/main/cpp/audioapi/android/core/AudioPlayer.*` | Oboe stream setup, `DataCallback` pattern |
| `android/src/main/java/.../system/MediaSessionManager.kt` | Audio focus, permissions, notification lifecycle |
| `android/src/main/java/.../system/AudioFocusListener.kt` | Focus→event mapping table |
| `android/build.gradle` | New BuildConfig flags, new Gradle deps (also see `build-compilation-dependencies` skill) |
