#pragma once

#import <dispatch/dispatch.h>
#import <os/log.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Native logging for the iOS layer, written to the unified log under the subsystem
 * `com.swmansion.audioapi`. The category is the module name.
 *
 * Define `LOG_MODULE` as that name before including this header.
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

__unused static inline const char *_Nonnull logBool(bool value)
{
  return value ? "true" : "false";
}

/** From least to most verbose; a line is printed when its level is at most the minimum level. */
typedef enum {
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE,
} LogLevel;

__unused static inline LogLevel logLevelFromName(const char *_Nullable name)
{
  if (name == NULL) {
    return LOG_LEVEL_ERROR;
  }
  if (strcmp(name, "warning") == 0) {
    return LOG_LEVEL_WARNING;
  }
  if (strcmp(name, "debug") == 0) {
    return LOG_LEVEL_DEBUG;
  }
  if (strcmp(name, "trace") == 0) {
    return LOG_LEVEL_TRACE;
  }
  return LOG_LEVEL_ERROR;
}

__unused static inline LogLevel logMinimumLevel(void)
{
  static LogLevel minimumLevel = LOG_LEVEL_ERROR;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ minimumLevel = logLevelFromName(getenv("AUDIO_LOG_LEVEL")); });
  return minimumLevel;
}

#ifndef LOG_MODULE
#error "Define LOG_MODULE to the module name before including Log.h"
#endif

#define LOG_STRINGIFY_IMPL(name) #name
#define LOG_STRINGIFY(name) LOG_STRINGIFY_IMPL(name)

__unused static inline os_log_t _Nonnull logModule(void)
{
  static os_log_t log;
  static dispatch_once_t onceToken;
  dispatch_once(
      &onceToken, ^{ log = os_log_create(AUDIO_LOG_SUBSYSTEM, LOG_STRINGIFY(LOG_MODULE)); });
  return log;
}

#if defined(DEBUG) && DEBUG
#define AUDIO_LOG_ERROR(fmt, ...) \
  do { \
    if (logMinimumLevel() >= LOG_LEVEL_ERROR) \
      os_log_error(logModule(), "[error] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_WARNING(fmt, ...) \
  do { \
    if (logMinimumLevel() >= LOG_LEVEL_WARNING) \
      os_log(logModule(), "[warning] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_DEBUG(fmt, ...) \
  do { \
    if (logMinimumLevel() >= LOG_LEVEL_DEBUG) \
      os_log_info(logModule(), "[debug] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_TRACE(fmt, ...) \
  do { \
    if (logMinimumLevel() >= LOG_LEVEL_TRACE) \
      os_log_debug(logModule(), "[trace] " fmt, ##__VA_ARGS__); \
  } while (0)
#else
#define AUDIO_LOG_ERROR(fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_WARNING(fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_DEBUG(fmt, ...) \
  do { \
  } while (0)
#define AUDIO_LOG_TRACE(fmt, ...) \
  do { \
  } while (0)
#endif
