#pragma once

#import <dispatch/dispatch.h>
#import <os/log.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Logs to the unified log, with `LOG_MODULE` as the category.
 *
 * The unified log has no warning or trace type, so each line starts with a `[level]` tag: filter
 * on that tag, not on the log type. Errors use `os_log_fault` and warnings `os_log_error` because
 * Xcode's console colors those red and yellow. Debug and trace lines are not persisted by default,
 * so they reach only live readers: an attached debugger, Console.app or `log stream --level debug`.
 *
 * Debug builds print the levels up to the `AUDIO_LOG_LEVEL` environment variable, read once per
 * launch. Release builds compile the calls out, arguments included.
 *
 * Interpolated strings and objects need `%{public}`, otherwise the log shows `<private>`.
 * The unified log is not real-time safe, so the calls must not run on the render callback.
 */

#define AUDIO_LOG_SUBSYSTEM "com.swmansion.audioapi"

__unused static inline const char *_Nonnull logBool(bool value)
{
  return value ? "true" : "false";
}

/** From least to most verbose. */
typedef enum {
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE,
} LogLevel;

#define LOG_LEVEL_DEFAULT LOG_LEVEL_WARNING

__unused static inline LogLevel logLevelFromName(const char *_Nullable name)
{
  if (name == NULL) {
    return LOG_LEVEL_DEFAULT;
  }
  if (strcmp(name, "error") == 0) {
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
  return LOG_LEVEL_DEFAULT;
}

__unused static inline LogLevel logVerbosityThreshold(void)
{
  static LogLevel threshold = LOG_LEVEL_DEFAULT;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ threshold = logLevelFromName(getenv("AUDIO_LOG_LEVEL")); });
  return threshold;
}

/** `__INCLUDE_LEVEL__` is 0 when editor tooling parses this header without an includer. */
#if !defined(LOG_MODULE) && __INCLUDE_LEVEL__ > 0
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
    if (logVerbosityThreshold() >= LOG_LEVEL_ERROR) \
      os_log_fault(logModule(), "[error] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_WARNING(fmt, ...) \
  do { \
    if (logVerbosityThreshold() >= LOG_LEVEL_WARNING) \
      os_log_error(logModule(), "[warning] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_DEBUG(fmt, ...) \
  do { \
    if (logVerbosityThreshold() >= LOG_LEVEL_DEBUG) \
      os_log_info(logModule(), "[debug] " fmt, ##__VA_ARGS__); \
  } while (0)
#define AUDIO_LOG_TRACE(fmt, ...) \
  do { \
    if (logVerbosityThreshold() >= LOG_LEVEL_TRACE) \
      os_log_debug(logModule(), "[trace] " fmt, ##__VA_ARGS__); \
  } while (0)
/** Inside a block, `__func__` names the block, not the enclosing method. */
#define AUDIO_LOG_TRACE_FUNCTION() \
  do { \
    if (logVerbosityThreshold() >= LOG_LEVEL_TRACE) \
      os_log_debug(logModule(), "[trace] %{public}s", __func__); \
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
#define AUDIO_LOG_TRACE_FUNCTION() \
  do { \
  } while (0)
#endif
