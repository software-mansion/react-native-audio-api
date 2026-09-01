#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Lifecycle tracing for the iOS audio session and engine.
///
/// Route changes, interruptions and media-server resets are driven by the OS and
/// cannot be reproduced from a test, so the only way to explain a restart that
/// went wrong is a trace of what the library asked for and what AVFoundation
/// answered. Every line carries a sequence number, a monotonic timestamp, the
/// calling thread and the enclosing operation path, which is what turns a
/// self-inflicted feedback loop - our own `setCategory:` posting the
/// configuration change that triggers the next restart - into something visible
/// rather than merely suspected.
///
/// Tracing is on in debug builds and off otherwise; call
/// `AudioAPISetDiagnosticsEnabled(YES)` to force it on while chasing a report
/// from a release build.
///
/// Lines go to the unified log under subsystem `com.swmansion.audioapi`, with
/// one category per area below. Unlike `NSLog` this survives a locked and
/// backgrounded device, which is the only state some session failures reproduce
/// in, and it can be read back after the fact:
///
///     log stream --predicate 'subsystem == "com.swmansion.audioapi"' --style compact
///     log show --last 5m --predicate 'subsystem == "com.swmansion.audioapi"'
///
/// Failures are logged at the error level, so `--predicate '... AND
/// messageType == error'` narrows a long trace to what actually broke. Messages
/// are declared public: an audio route is not user data, and redacted lines
/// would defeat the point.

FOUNDATION_EXPORT NSString *const AudioAPIDiagnosticsSubsystem;

FOUNDATION_EXPORT NSString *const AudioAPIDiagnosticsCategoryEngine;
FOUNDATION_EXPORT NSString *const AudioAPIDiagnosticsCategoryNotifications;
FOUNDATION_EXPORT NSString *const AudioAPIDiagnosticsCategoryRecorder;
FOUNDATION_EXPORT NSString *const AudioAPIDiagnosticsCategorySession;

FOUNDATION_EXPORT BOOL AudioAPIDiagnosticsEnabled(void);
FOUNDATION_EXPORT void AudioAPISetDiagnosticsEnabled(BOOL enabled);

FOUNDATION_EXPORT void AudioAPILogEvent(NSString *category, NSString *format, ...)
    NS_FORMAT_FUNCTION(2, 3);
/// Counterpart of `AudioAPILogEvent` for things that actually went wrong. Marked
/// with `!`, logged at the error level and emitted even when tracing is
/// disabled, so a release build still reports why the engine gave up.
FOUNDATION_EXPORT void AudioAPILogFailure(NSString *category, NSString *format, ...)
    NS_FORMAT_FUNCTION(2, 3);

/// Names the operation the current thread is inside, so nested events - and
/// events the OS delivers re-entrantly from within an AVFoundation call - report
/// where they came from. Prefer the `AUDIOAPI_TRACE_SCOPE` macro over these.
FOUNDATION_EXPORT void AudioAPIPushDiagnosticsScope(NSString *name);
FOUNDATION_EXPORT void AudioAPIPopDiagnosticsScope(void);
/// The enclosing scopes joined innermost-last, e.g. `startEngine>setActive`, or
/// `-` when the current thread is not inside a traced operation.
FOUNDATION_EXPORT NSString *AudioAPICurrentDiagnosticsScope(void);

/// These read `AVAudioSession`, and every property read is an XPC round trip to
/// mediaserverd. Keep them off paths that can repeat within one operation - a
/// trace that stalls the thread it is observing reports its own overhead.
FOUNDATION_EXPORT NSString *AudioAPIDescribeRoute(AVAudioSessionRouteDescription *_Nullable route);
FOUNDATION_EXPORT NSString *AudioAPIDescribeSession(AVAudioSession *_Nullable session);
/// Reports the raw sample rate and channel count even when they are unusable,
/// which is how a refused input shows itself: a non-nil format reading 0 Hz.
FOUNDATION_EXPORT NSString *AudioAPIDescribeFormat(AVAudioFormat *_Nullable format);
/// Renders an `NSError` as `domain/code` plus its description, and spells out
/// the four-character code AVFoundation packs into `code` - a session error
/// reads as `560557684`, which is the integer form of '!int'.
FOUNDATION_EXPORT NSString *AudioAPIDescribeError(NSError *_Nullable error);

#define AUDIOAPI_LOG(category, format, ...) \
  do { \
    if (AudioAPIDiagnosticsEnabled()) { \
      AudioAPILogEvent((category), (format), ##__VA_ARGS__); \
    } \
  } while (0)

#define AUDIOAPI_LOG_FAILURE(category, format, ...) \
  AudioAPILogFailure((category), (format), ##__VA_ARGS__)

#ifdef __cplusplus

namespace audioapi {

/// Scope-based counterpart of `AudioAPIPushDiagnosticsScope`.
struct DiagnosticsScope {
  explicit DiagnosticsScope(NSString *name)
  {
    AudioAPIPushDiagnosticsScope(name);
  }

  ~DiagnosticsScope()
  {
    AudioAPIPopDiagnosticsScope();
  }

  DiagnosticsScope(const DiagnosticsScope &) = delete;
  DiagnosticsScope &operator=(const DiagnosticsScope &) = delete;
};

} // namespace audioapi

#define AUDIOAPI_TRACE_SCOPE(name) audioapi::DiagnosticsScope _audioAPIDiagnosticsScope(name)

#endif // __cplusplus

NS_ASSUME_NONNULL_END
