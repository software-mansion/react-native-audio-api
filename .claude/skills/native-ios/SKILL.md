---
name: native-ios
description: >
  iOS native layer for react-native-audio-api — Objective-C++, AVFoundation, CoreAudio, audio
  session management, and iOS-specific patterns. Covers AudioSessionManager (AVAudioSession
  configuration, permissions, device routing), AudioEngine (AVAudioEngine lifecycle, interruption
  recovery, source/sink node attach/detach), IOSAudioPlayer/Recorder patterns, SystemNotificationManager,
  and the New Architecture TurboModule path. Use when modifying ios/audioapi/ files, implementing
  iOS-only platform features, debugging iOS-specific audio behaviour, or working with AVAudioSession
  configuration.
  Trigger phrases: "iOS native", "AVAudioSession", "AVAudioEngine", "AudioSessionManager",
  "AudioEngine iOS", "pod install", "ObjC++", "iOS audio session", "iOS permission",
  "interruption iOS", "CoreAudio", "AVFoundation", "ios/audioapi".
---

# Skill: iOS Native Layer

## Directory Structure

```
ios/audioapi/ios/
├── AudioAPIModule.h/.mm          # TurboModule entry point
├── core/
│   ├── IOSAudioPlayer.h/.mm      # AVAudioEngine-backed playback
│   ├── IOSAudioRecorder.h/.mm    # AVAudioSinkNode-based recording
│   ├── NativeAudioPlayer.h       # ObjC wrapper for IOSAudioPlayer
│   ├── NativeAudioRecorder.h     # ObjC wrapper for IOSAudioRecorder
│   └── utils/
│       ├── FileOptions.h/.mm
│       ├── IOSFileWriter.h/.mm
│       └── IOSRecorderCallback.h/.mm
└── system/
    ├── AudioSessionManager.h/.mm       # AVAudioSession lifecycle
    ├── AudioEngine.h/.mm               # AVAudioEngine lifecycle
    ├── SystemNotificationManager.h/.mm # Interruption + route change listeners
    └── notification/
        ├── NotificationRegistry.h/.mm
        ├── PlaybackNotification.h/.mm
        └── PlaybackNotificationReceiver.h/.mm
```

---

## AudioSessionManager

Manages the `AVAudioSession` singleton for the whole app.

### Configuration

```objc
// Set category + mode + options
[AudioSessionManager setAudioSessionOptions:category mode:mode options:options
                              allowHaptics:allowHaptics notifyOnDeactivation:true];
```

Key categories: `playback`, `record`, `playAndRecord`, `multiRoute`, `ambient`, `soloAmbient`.
Key modes: `default`, `videoChat`, `voiceChat`, `measurement`, `moviePlayback`, `spokenAudio`.

Options bitmask (note: Bluetooth HFP is hardcoded as `0x4` due to Xcode version variance):
`duckOthers`, `mixWithOthers`, `allowAirPlay`, `allowBluetoothA2DP`, `defaultToSpeaker`, `overrideMutedMicrophoneInterruption`.

Haptics flag applied separately via `setAllowHapticsAndSystemSoundsDuringRecording:` (iOS 13+ only).

### Permissions

```objc
// Async, blocks with dispatch_semaphore
[AudioSessionManager requestRecordingPermissions:^(PermissionStatus status) { ... }];

// Check current state
PermissionStatus status = [AudioSessionManager checkRecordingPermissions];
// → Granted / Denied / Undetermined
```

iOS 17+ uses `AVAudioApplication.requestRecordPermission()` on device; earlier API on simulator.
`NSMicrophoneUsageDescription` must be in Info.plist or the request crashes.

### Device Routing

```objc
NSArray<NSDictionary *> *devices = [AudioSessionManager getDevicesInfo];
[AudioSessionManager setInputDevice:portUID];  // route to specific mic/input
```

### Interruption Recovery

`markInactive()` is called by `SystemNotificationManager` when an interruption begins — forces session state to inactive so it is properly reactivated when the interruption ends.

---

## AudioEngine

Wraps `AVAudioEngine` lifecycle. Tracks four states: `Idle`, `Running`, `Paused`, `Interrupted`.

### Attaching Nodes

```objc
// Playback — returns UUID used to detach later
NSString *nodeId = [AudioEngine attachSourceNode:avAudioSourceNode format:format];
[AudioEngine detachSourceNodeWithId:nodeId];

// Recording input
[AudioEngine attachInputNode:avAudioSinkNode];
[AudioEngine detachInputNode];
```

`attachSourceNode` connects the source to `mainMixerNode → outputNode` automatically.

### Lifecycle

```objc
[AudioEngine startIfNecessary];    // activates AVAudioSession, then starts engine
[AudioEngine pauseIfNecessary];    // pause (keeps session active)
[AudioEngine stopIfPossible];      // stop only if no sources and no input
```

### Interruption Recovery

When `onInterruptionBegin()` is called, state transitions to `Interrupted`.
`onInterruptionEnd()` rebuilds the engine (detaches all nodes, creates new `AVAudioEngine`, re-attaches), then restarts if there were active sources.

**Rebuilding is necessary** because `AVAudioEngine` can enter an unrecoverable state after an interruption. Do not just `start` a paused engine after interruption — always rebuild.

---

## IOSAudioPlayer

C++ class that wraps `NativeAudioPlayer` (ObjC). Key pattern:

```mm
// Playback uses AVAudioSourceNode with a C++ functor callback
AVAudioSourceNode *sourceNode = [[AVAudioSourceNode alloc]
    initWithFormat:format
    renderBlock:^OSStatus(BOOL *isSilence, const AudioTimeStamp *ts, AVAudioFrameCount count,
                          AudioBufferList *outputData) {
      renderAudio_(outputData, count);  // C++ functor fills the buffer
      return noErr;
    }];
```

State tracking uses `std::atomic<bool>` for thread-safe `isRunning()` checks.

---

## IOSAudioRecorder

Uses `AVAudioSinkNode` (iOS 13+) to capture microphone input:

```mm
AVAudioSinkNode *sinkNode = [[AVAudioSinkNode alloc]
    initWithReceiverBlock:^(const AudioTimeStamp *ts, AVAudioFrameCount count,
                             const AudioBufferList *inputData) {
      callback_(inputData, count);   // C++ callback processes PCM frames
      return noErr;
    }];
```

File writing offloaded to `IOSFileWriter` which uses `TaskOffloader` to avoid blocking the audio thread.

---

## SystemNotificationManager

Listens to AVAudioSession notifications and routes them into the event system:

| Notification | Action |
|---|---|
| `AVAudioSessionInterruptionNotification` | `onInterruptionBegin()` / `onInterruptionEnd()` on AudioEngine |
| `AVAudioEngineConfigurationChange` | Engine rebuild + restart |
| Route change | `invokeHandlerWithEventBody(INTERRUPTION, ...)` → JS callback |

---

## AudioAPIModule.mm — TurboModule

### New Architecture path

```objc
#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<TurboModule>)getTurboModule:(const ObjCTurboModule::InitParams &)params {
  return std::make_shared<NativeAudioAPIModuleSpecJSI>(params);
}
#endif
```

### `install()` flow

1. Allocate `AudioSessionManager`, `AudioEngine`, `SystemNotificationManager`, `NotificationRegistry`
2. Get `jsi::Runtime *` from bridge: `reinterpret_cast<jsi::Runtime *>(self.bridge.runtime)`
3. Get `CallInvoker` — old Bridge vs New Arch:
   ```objc
   #if defined(RCT_NEW_ARCH_ENABLED)
     auto jsCallInvoker = _callInvoker.callInvoker;
   #else
     auto jsCallInvoker = self.bridge.jsCallInvoker;
   #endif
   ```
4. Create `AudioEventHandlerRegistry`
5. Call `AudioAPIModuleInstaller::injectJSIBindings(...)`

`install()` is exported as `RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD` — must complete before any JS audio API call.

Module runs on a dedicated serial queue: `com.swmansion.audioapi.MainModuleQueue`.

---

## Version Guards

```objc
if (@available(iOS 13.0, *)) { /* AVAudioSinkNode, haptics flag */ }
if (@available(iOS 17.0, *)) { /* AVAudioApplication.requestRecordPermission */ }
```

Always use `@available` guards. Do not assume availability without checking.

---

## Common Pitfalls

- **`AVAudioEngine` after interruption** — must rebuild, not just restart. `onInterruptionEnd()` already handles this, but don't bypass it.
- **`NSMicrophoneUsageDescription` missing** — causes a crash (not a graceful denial) when requesting permissions.
- **Forgetting `startIfNecessary`** before attaching source nodes — the engine must be running for nodes to produce audio.
- **`dispatch_semaphore` on main thread** — permission requests use a semaphore; ensure the callback doesn't need the main thread or deadlock results.
- **Hardcoded Bluetooth HFP option value** — `0x4` is intentional due to Xcode version variance in `AVAudioSessionOptions` constants.

---

*Maintenance: see [maintenance.md](maintenance.md).*
