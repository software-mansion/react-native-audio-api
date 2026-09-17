#import <audioapi/ios/system/AudioAPIDiagnostics.h>

#import <os/log.h>

#include <ctype.h>
#include <pthread.h>
#include <atomic>

NSString *const AudioAPIDiagnosticsSubsystem = @"com.swmansion.audioapi";

NSString *const AudioAPIDiagnosticsCategoryEngine = @"engine";
NSString *const AudioAPIDiagnosticsCategoryNotifications = @"notify";
NSString *const AudioAPIDiagnosticsCategoryRecorder = @"record";
NSString *const AudioAPIDiagnosticsCategorySession = @"session";

static NSString *const DiagnosticsScopeStackKey = @"AudioAPIDiagnosticsScopeStack";

#if DEBUG
static std::atomic<bool> diagnosticsEnabled{true};
#else
static std::atomic<bool> diagnosticsEnabled{false};
#endif

static std::atomic<uint64_t> nextSequenceNumber{0};

BOOL AudioAPIDiagnosticsEnabled(void)
{
  return diagnosticsEnabled.load(std::memory_order_relaxed) ? YES : NO;
}

void AudioAPISetDiagnosticsEnabled(BOOL enabled)
{
  diagnosticsEnabled.store(enabled == YES, std::memory_order_relaxed);
}

/// One logger per category, so a trace can be narrowed with
/// `category == "session"` instead of grepping message text.
static os_log_t logForCategory(NSString *category)
{
  static os_log_t engineLog;
  static os_log_t notificationsLog;
  static os_log_t recorderLog;
  static os_log_t sessionLog;
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    const char *subsystem = [AudioAPIDiagnosticsSubsystem UTF8String];

    engineLog = os_log_create(subsystem, "engine");
    notificationsLog = os_log_create(subsystem, "notify");
    recorderLog = os_log_create(subsystem, "record");
    sessionLog = os_log_create(subsystem, "session");
  });

  if ([category isEqualToString:AudioAPIDiagnosticsCategoryNotifications]) {
    return notificationsLog;
  }

  if ([category isEqualToString:AudioAPIDiagnosticsCategoryRecorder]) {
    return recorderLog;
  }

  if ([category isEqualToString:AudioAPIDiagnosticsCategorySession]) {
    return sessionLog;
  }

  return engineLog;
}

static NSTimeInterval secondsSinceFirstEvent(void)
{
  static NSTimeInterval firstEventUptime;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ firstEventUptime = [[NSProcessInfo processInfo] systemUptime]; });

  return [[NSProcessInfo processInfo] systemUptime] - firstEventUptime;
}

static NSString *currentThreadLabel(void)
{
  if ([NSThread isMainThread]) {
    return @"main";
  }

  NSString *threadName = [[NSThread currentThread] name];

  if (threadName.length > 0) {
    return threadName;
  }

  uint64_t threadId = 0;
  pthread_threadid_np(NULL, &threadId);

  return [NSString stringWithFormat:@"t%llu", threadId];
}

static NSMutableArray<NSString *> *currentThreadScopeStack(void)
{
  NSMutableDictionary *threadStorage = [[NSThread currentThread] threadDictionary];
  NSMutableArray<NSString *> *scopeStack = threadStorage[DiagnosticsScopeStackKey];

  if (scopeStack == nil) {
    scopeStack = [[NSMutableArray alloc] init];
    threadStorage[DiagnosticsScopeStackKey] = scopeStack;
  }

  return scopeStack;
}

void AudioAPIPushDiagnosticsScope(NSString *name)
{
  [currentThreadScopeStack() addObject:name];
}

void AudioAPIPopDiagnosticsScope(void)
{
  NSMutableArray<NSString *> *scopeStack = currentThreadScopeStack();

  if (scopeStack.count == 0) {
    return;
  }

  [scopeStack removeLastObject];
}

NSString *AudioAPICurrentDiagnosticsScope(void)
{
  NSMutableArray<NSString *> *scopeStack = currentThreadScopeStack();

  if (scopeStack.count == 0) {
    return @"-";
  }

  return [scopeStack componentsJoinedByString:@">"];
}

/// The sequence number is what makes a restart storm countable: the unified log
/// reorders lines emitted from different threads within the same millisecond, so
/// wall-clock order alone cannot tell 40 dispatches from 4 retried 10 times.
static void logLine(NSString *category, NSString *marker, NSString *message, bool isFailure)
{
  NSString *line =
      [NSString stringWithFormat:@"[AudioAPI]%@#%04llu %8.3fs %-6s %@ | %@",
                                 marker,
                                 nextSequenceNumber.fetch_add(1, std::memory_order_relaxed),
                                 secondsSinceFirstEvent(),
                                 [currentThreadLabel() UTF8String],
                                 AudioAPICurrentDiagnosticsScope(),
                                 message];

  os_log_t log = logForCategory(category);

  if (isFailure) {
    os_log_error(log, "%{public}@", line);
    return;
  }

  // Default rather than info: info-level lines live only in the in-memory ring
  // buffer and are dropped unless someone is already streaming, which is never
  // true for a failure that only reproduces on a locked device.
  os_log(log, "%{public}@", line);
}

void AudioAPILogEvent(NSString *category, NSString *format, ...)
{
  if (!AudioAPIDiagnosticsEnabled()) {
    return;
  }

  va_list arguments;
  va_start(arguments, format);
  NSString *message = [[NSString alloc] initWithFormat:format arguments:arguments];
  va_end(arguments);

  logLine(category, @" ", message, false);
}

void AudioAPILogFailure(NSString *category, NSString *format, ...)
{
  va_list arguments;
  va_start(arguments, format);
  NSString *message = [[NSString alloc] initWithFormat:format arguments:arguments];
  va_end(arguments);

  logLine(category, @"!", message, true);
}

NSString *AudioAPIDescribeRoute(AVAudioSessionRouteDescription *route)
{
  if (route == nil) {
    return @"(none)";
  }

  NSMutableArray<NSString *> *describedPorts = [[NSMutableArray alloc] init];

  for (AVAudioSessionPortDescription *input in route.inputs) {
    [describedPorts
        addObject:[NSString stringWithFormat:@"in:%@(%@)", input.portName, input.portType]];
  }

  for (AVAudioSessionPortDescription *output in route.outputs) {
    [describedPorts
        addObject:[NSString stringWithFormat:@"out:%@(%@)", output.portName, output.portType]];
  }

  if (describedPorts.count == 0) {
    return @"(empty)";
  }

  return [describedPorts componentsJoinedByString:@", "];
}

NSString *AudioAPIDescribeSession(AVAudioSession *session)
{
  if (session == nil) {
    return @"(none)";
  }

  return [NSString stringWithFormat:
                       @"category=%@, mode=%@, options=%lu, sampleRate=%.0f, inputChannels=%lu, "
                       @"outputChannels=%lu, ioBuffer=%.4fs",
                       session.category ?: @"(null)",
                       session.mode ?: @"(null)",
                       (unsigned long)session.categoryOptions,
                       session.sampleRate,
                       (unsigned long)session.inputNumberOfChannels,
                       (unsigned long)session.outputNumberOfChannels,
                       session.IOBufferDuration];
}

NSString *AudioAPIDescribeFormat(AVAudioFormat *format)
{
  if (format == nil) {
    return @"nil";
  }

  return [NSString stringWithFormat:@"%.0fHz/%uch/%@",
                                    format.sampleRate,
                                    format.channelCount,
                                    format.interleaved ? @"interleaved" : @"deinterleaved"];
}

/// AVFoundation reports session failures as an `OSStatus` packed from four
/// printable characters, and the decimal form is unreadable. Spelling it out is
/// what separates the failure modes: '!int' means the session was refused
/// because another app owns it, '!act' that activation itself was rejected.
static NSString *describeFourCharacterCode(NSInteger code)
{
  uint32_t rawCode = (uint32_t)code;
  char characters[5] = {
      (char)((rawCode >> 24) & 0xFF),
      (char)((rawCode >> 16) & 0xFF),
      (char)((rawCode >> 8) & 0xFF),
      (char)(rawCode & 0xFF),
      '\0'};

  for (size_t index = 0; index < 4; index += 1) {
    if (!isprint((unsigned char)characters[index])) {
      return @"";
    }
  }

  return [NSString stringWithFormat:@" ('%s')", characters];
}

NSString *AudioAPIDescribeError(NSError *error)
{
  if (error == nil) {
    return @"nil";
  }

  return [NSString stringWithFormat:@"%@/%ld%@: %@",
                                    error.domain,
                                    (long)error.code,
                                    describeFourCharacterCode(error.code),
                                    error.localizedDescription];
}
