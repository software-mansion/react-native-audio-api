---
name: native-android
description: >
  Android native layer for react-native-audio-api — Kotlin, JNI/fbjni, Oboe audio I/O,
  MediaSession, and Android-specific patterns. Covers AudioAPIModule (fbjni HybridClass),
  MediaSessionManager (audio focus, permissions, notifications, device enumeration),
  Oboe AudioPlayer (stream setup, callbacks), JNI event invocation path, version-gated API calls
  (API 26, API 33), ForegroundServiceManager, and Kotlin permission patterns.
  Use when modifying android/src/main/ files, implementing Android-only platform features,
  debugging Android-specific audio behaviour, or working with Oboe streams and Android audio focus.
  Trigger phrases: "Android native", "Kotlin audio", "Oboe", "MediaSession", "audio focus Android",
  "fbjni", "HybridClass", "JNI audio", "Android permission", "Android interruption",
  "android/src/main", "ForegroundService".
---

# Skill: Android Native Layer

## Directory Structure

```
android/src/main/
├── java/com/swmansion/audioapi/
│   ├── AudioAPIModule.kt           # TurboModule entry point (fbjni HybridObject)
│   ├── AudioAPIPackage.kt          # React Native package registration
│   └── system/
│       ├── MediaSessionManager.kt  # Audio focus, permissions, notifications, devices
│       ├── AudioFocusListener.kt   # Audio focus callbacks
│       ├── ForegroundServiceManager.kt
│       ├── VolumeChangeListener.kt
│       ├── PermissionRequestListener.kt
│       ├── NativeFileInfo.kt
│       ├── CentralizedForegroundService.kt
│       └── notification/           # Playback + recording notifications
└── cpp/audioapi/android/
    ├── AudioAPIModule.h/.cpp       # fbjni HybridClass C++ peer
    ├── OnLoad.cpp                  # JNI_OnLoad — calls registerNatives()
    └── core/
        ├── AudioPlayer.h/.cpp      # Oboe stream setup + callbacks
        ├── AndroidAudioRecorder.h/.cpp
        └── utils/                  # File writing, FFmpeg backend, MiniAudio backend
```

---

## fbjni HybridClass Pattern

`AudioAPIModule` uses fbjni's `HybridClass` pattern — a Kotlin class holds a C++ peer via `HybridData`.

### Registration (C++ side)

```cpp
// OnLoad.cpp — called at .so load time
void AudioAPIModule::registerNatives() {
  registerHybrid({
    makeNativeMethod("initHybrid",        AudioAPIModule::initHybrid),
    makeNativeMethod("injectJSIBindings", AudioAPIModule::injectJSIBindings),
    makeNativeMethod("invokeHandlerWithEventNameAndEventBody",
                     AudioAPIModule::invokeHandlerWithEventNameAndEventBody),
  });
}
```

### Initialization flow

```
AudioAPIModule.kt (init block)
  ├── System.loadLibrary("react-native-audio-api")
  ├── get CallInvokerHolderImpl from reactContext
  ├── get WorkletsModule if worklets enabled
  └── mHybridData = initHybrid(workletsModule, jsContext, callInvokerHolder)
              ↓ JNI call
      AudioAPIModule.cpp::initHybrid()
        ├── reinterpret_cast<jsi::Runtime *>(jsContext)
        ├── unwrap jsCallInvoker from holder
        ├── [if worklets] get WorkletsModuleProxy
        └── makeCxxInstance(...)   → C++ AudioAPIModule peer
```

### install() flow

```
AudioAPIModule.kt::install()
  ├── MediaSessionManager.initialize(activity, reactContext, ...)
  ├── NativeFileInfo.initialize(reactContext)
  └── injectJSIBindings()   // external fun → JNI → AudioAPIModuleInstaller
```

**`external fun`** is Kotlin's keyword for JNI-implemented methods.

---

## JNI Event Invocation

Kotlin calls back into C++ for audio events (e.g. audio focus changes, volume, interruption):

```kotlin
// Kotlin side (MediaSessionManager, AudioFocusListener)
module.invokeHandlerWithEventNameAndEventBody(
    AudioEvent.INTERRUPTION.ordinal,
    mapOf("type" to "began", "shouldResume" to false)
)
```

```cpp
// C++ side — converts Java Map → C++ unordered_map
void AudioAPIModule::invokeHandlerWithEventNameAndEventBody(
    jint eventOrdinal, alias_ref<JMap<JString, JObject>> jBody) {
  auto audioEvent = static_cast<AudioEvent>(eventOrdinal);
  std::unordered_map<std::string, EventValue> body;
  // jni::JString, JInteger, JDouble, JFloat, JBoolean casting...
  audioEventHandlerRegistry_->invokeHandlerWithEventBody(audioEvent, body);
}
```

---

## MediaSessionManager

Singleton. Manages audio focus, permissions, notifications, and device enumeration.

### Audio Focus

```kotlin
// Request audio focus
val result = mediaSessionManager.requestAudioFocus(focusType)
// focusType: "gain" | "gainTransient" | "gainTransientMayDuck" | "gainTransientExclusive"

// Abandon focus
mediaSessionManager.abandonAudioFocus()
```

Version-gated internally:
- API 26+: `AudioFocusRequest.Builder` pattern
- API <26: deprecated `AudioManager.requestAudioFocus(listener, ...)`

### AudioFocusListener → Event Mapping

| Platform event | AudioEvent fired | shouldResume |
|---|---|---|
| `AUDIOFOCUS_GAIN` | `INTERRUPTION` (type: "ended") | from saved state |
| `AUDIOFOCUS_LOSS` | `INTERRUPTION` (type: "began") | false |
| `AUDIOFOCUS_LOSS_TRANSIENT` | `INTERRUPTION` (type: "began") | true |
| `AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK` | `DUCK` | — |

### Permissions

```kotlin
// Recording
mediaSessionManager.requestRecordingPermissions(activity)
val status = mediaSessionManager.checkRecordingPermissions()
// → "Granted" | "Denied" | "Undetermined"

// Notifications (API 33+ requires explicit permission)
mediaSessionManager.requestNotificationPermissions(activity)
```

Uses `PermissionAwareActivity` with request codes (`RECORDING_REQUEST_CODE`, `NOTIFICATION_REQUEST_CODE`).
`shouldShowRequestPermissionRationale()` distinguishes Denied vs Undetermined.
API <33: notification permission is auto-granted.

### Device Enumeration (API 26+)

```kotlin
val devices = mediaSessionManager.getDevicesInfo()
// Returns input/output AudioDeviceInfo list with type mapping:
// TYPE_BUILTIN_MIC, TYPE_WIRED_HEADPHONES, TYPE_BLUETOOTH_A2DP, etc.
```

---

## Oboe AudioPlayer

`AudioPlayer` implements `AudioStreamDataCallback` and `AudioStreamErrorCallback`.

### Stream Setup

```cpp
AudioStreamBuilder()
  .setSharingMode(SharingMode::Exclusive)
  .setFormat(AudioFormat::Float)
  .setFormatConversionAllowed(true)
  .setPerformanceMode(PerformanceMode::None)  // not HighPerformance — avoids latency issues
  .setChannelCount(channelCount_)
  .setSampleRate(sampleRate_)
  .setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
  .setDataCallback(this)
  .setErrorCallback(this)
  .openStream(&mStream_);
```

`PerformanceMode::None` is intentional — `HighPerformance` (FAST path) can cause latency issues with some Android devices.

### Callbacks

```cpp
// Audio thread — fills audioData from the C++ audio engine
DataCallbackResult onAudioReady(AudioStream *, void *audioData, int32_t numFrames) {
  renderAudio_(audioBuffer_, numFrames);       // C++ functor
  audioBuffer_->interleaveTo((float *)audioData, numFrames);
  return DataCallbackResult::Continue;
}

// Error thread — after stream close
void onErrorAfterClose(AudioStream *, Result error) {
  __android_log_print(ANDROID_LOG_ERROR, "AudioPlayer", "Stream error: %s",
                      convertToText(error));
}
```

### Lifecycle

```cpp
mStream_->requestStart();   // start()
mStream_->requestStop();    // stop()
mStream_->requestPause();   // suspend()
```

State tracking uses `std::atomic<bool> isRunning_` for lock-free cross-thread checks.

---

## ForegroundServiceManager

Manages a `CentralizedForegroundService` that keeps the app alive during background audio:

- Tracks subscribers (active `BaseNotification` instances)
- Starts `CentralizedForegroundService` when first subscriber added (API 26+: `startForegroundService`)
- Stops when last subscriber removed
- `PlaybackNotification` wraps `MediaSessionCompat` for lock-screen media controls
- `PlaybackNotificationReceiver` handles dismiss + media button intents → fires JS events

---

## Build Configuration Flags

Read from `BuildConfig` (set in `build.gradle` from `gradle.properties`):

```kotlin
if (BuildConfig.RN_AUDIO_API_ENABLE_WORKLETS) { /* worklets integration */ }
if (BuildConfig.IS_NEW_ARCHITECTURE_ENABLED) { /* TurboModule vs old Bridge */ }
if (BuildConfig.RN_AUDIO_API_FFMPEG_DISABLED) { /* no FFmpeg decoding */ }
```

---

## Common Pitfalls

- **Audio focus not abandoned** — always call `abandonAudioFocus()` on pause/stop. Failure to do so blocks other apps from taking focus.
- **Forgetting API version guards** — `AudioFocusRequest.Builder` is API 26+; `MediaSessionCompat` for API <26; `POST_NOTIFICATIONS` permission is API 33+ only.
- **`PerformanceMode::HighPerformance`** — do not change to this; it causes latency/glitch issues on some devices. Current `PerformanceMode::None` is intentional.
- **JNI Map type casting** — converting Java `Map<String, Object>` to C++ requires explicit type checks (`JString`, `JInteger`, `JDouble`, etc.); missing a type causes a silent null.
- **ForegroundService not started** — `startForegroundService` must be called before posting a foreground notification (API 26+), or the OS kills the service immediately.

---

*Maintenance: see [maintenance.md](maintenance.md).*
