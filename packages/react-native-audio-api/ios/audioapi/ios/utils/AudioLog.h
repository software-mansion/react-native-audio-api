#pragma once

#import <dispatch/dispatch.h>
#import <os/log.h>

#include <stdlib.h>
#include <string.h>

/**
 * Native logging for the iOS layer, written to the unified log under the subsystem
 * `com.swmansion.audioapi`, with one category per module.
 *
 * Each line starts with its level in brackets (`[error]`, `[warning]`, `[debug]`, `[trace]`),
 * because the unified log has no warning or trace type: filter on that tag, not on the log type.
 * Debug lines are `os_log_info` and trace lines are `os_log_debug`, which the system drops unless
 * a debugger is attached or `log config --mode "level:debug"` is set for the subsystem.
 *
 * Debug builds print the levels up to the one named by the `AUDIO_LOG_LEVEL` environment variable
 * (`error`, `warning`, `debug`, `trace`), read once per launch; absent or unrecognized means
 * `error`. Release builds compile the calls out, arguments included.
 *
 * Interpolated strings and objects need `%{public}`, otherwise the log shows `<private>`.
 * The unified log is not real-time safe, so the calls must not run on the render callback.
 */

#define AUDIO_LOG_SUBSYSTEM "com.swmansion.audioapi"

/** From least to most verbose; a line is printed when its level is at most the minimum level. */
typedef enum {
  AudioLogLevelError,
  AudioLogLevelWarning,
  AudioLogLevelDebug,
  AudioLogLevelTrace,
} AudioLogLevel;

static inline AudioLogLevel AudioLogLevelFromName(const char *name)
{
  if (name == NULL) {
    return AudioLogLevelError;
  }
  if (strcmp(name, "warning") == 0) {
    return AudioLogLevelWarning;
  }
  if (strcmp(name, "debug") == 0) {
    return AudioLogLevelDebug;
  }
  if (strcmp(name, "trace") == 0) {
    return AudioLogLevelTrace;
  }
  return AudioLogLevelError;
}

static inline AudioLogLevel AudioLogMinimumLevel(void)
{
  static AudioLogLevel minimumLevel = AudioLogLevelError;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ minimumLevel = AudioLogLevelFromName(getenv("AUDIO_LOG_LEVEL")); });
  return minimumLevel;
}

#define AUDIO_LOG_DEFINE_MODULE(functionName, category) \
  static inline os_log_t functionName(void) \
  { \
    static os_log_t log; \
    static dispatch_once_t onceToken; \
    dispatch_once(&onceToken, ^{ log = os_log_create(AUDIO_LOG_SUBSYSTEM, category); }); \
    return log; \
  }

AUDIO_LOG_DEFINE_MODULE(AudioLogEngine, "engine")
AUDIO_LOG_DEFINE_MODULE(AudioLogSession, "session")
AUDIO_LOG_DEFINE_MODULE(AudioLogRecorder, "recorder")
AUDIO_LOG_DEFINE_MODULE(AudioLogPlayer, "player")
AUDIO_LOG_DEFINE_MODULE(AudioLogNotifications, "notifications")

#undef AUDIO_LOG_DEFINE_MODULE

#if defined(DEBUG) && DEBUG
#define AUDIO_LOG_ERROR(log, fmt, ...) \
  do { \
    if (AudioLogMinimumLevel() >= AudioLogLevelError) \
      os_log_error((log), "[error] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_WARNING(log, fmt, ...) \
  do { \
    if (AudioLogMinimumLevel() >= AudioLogLevelWarning) \
      os_log((log), "[warning] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_DEBUG(log, fmt, ...) \
  do { \
    if (AudioLogMinimumLevel() >= AudioLogLevelDebug) \
      os_log_info((log), "[debug] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_TRACE(log, fmt, ...) \
  do { \
    if (AudioLogMinimumLevel() >= AudioLogLevelTrace) \
      os_log_debug((log), "[trace] " fmt, ##__VA_ARGS__); \
  } while (0)
#else
#define AUDIO_LOG_ERROR(log, fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_WARNING(log, fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_DEBUG(log, fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_TRACE(log, fmt, ...) \
  do { \
  } while (0)
#endif
