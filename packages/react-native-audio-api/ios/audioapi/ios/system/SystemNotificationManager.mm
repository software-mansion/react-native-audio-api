#import <audioapi/events/AudioEvent.h>
#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioAPIDiagnostics.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/SystemNotificationManager.h>

#include <atomic>

@implementation SystemNotificationManager

static NSString *NotificationManagerContext = @"SystemNotificationManagerContext";

/// Restart requests handed to the main queue against restart requests that have
/// actually run. The gap between them is the queue depth, which is the one thing
/// a per-event log line cannot show: a burst of notifications each scheduling a
/// restart looks identical to a single restart until the two counts diverge.
static std::atomic<uint64_t> restartsScheduled{0};
static std::atomic<uint64_t> restartsDrained{0};

- (instancetype)initWithAudioAPIModule:(AudioAPIModule *)audioAPIModule
{
  if (self = [super init]) {
    self.audioAPIModule = audioAPIModule;
    self.notificationCenter = [NSNotificationCenter defaultCenter];
    self.audioInterruptionsObserved = false;

    [self configureNotifications];
  }

  return self;
}

- (void)cleanup
{
  self.notificationCenter = nil;
}

- (void)observeAudioInterruptions:(BOOL)enabled
{
  self.audioInterruptionsObserved = enabled;
}

- (void)activelyReclaimSession:(BOOL)enabled
{
  if (!enabled) {
    [self stopPollingSecondaryAudioHint];

    [self.notificationCenter removeObserver:self
                                       name:AVAudioSessionSilenceSecondaryAudioHintNotification
                                     object:nil];
    return;
  }

  [self.notificationCenter addObserver:self
                              selector:@selector(handleSecondaryAudio:)
                                  name:AVAudioSessionSilenceSecondaryAudioHintNotification
                                object:nil];

  dispatch_async(dispatch_get_main_queue(), ^{ [self startPollingSecondaryAudioHint]; });
}

// WARNING: this does not work in a simulator environment, test it on a real
// device
- (void)observeVolumeChanges:(BOOL)enabled
{
  if (self.volumeChangesObserved == enabled) {
    return;
  }

  if (enabled) {
    [[AVAudioSession sharedInstance] addObserver:self
                                      forKeyPath:@"outputVolume"
                                         options:NSKeyValueObservingOptionNew
                                         context:(void *)&NotificationManagerContext];
  } else {
    [[AVAudioSession sharedInstance] removeObserver:self forKeyPath:@"outputVolume" context:nil];
  }

  self.volumeChangesObserved = enabled;
}

- (void)configureNotifications
{
  [self.notificationCenter addObserver:self
                              selector:@selector(handleRouteChange:)
                                  name:AVAudioSessionRouteChangeNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleMediaServicesReset:)
                                  name:AVAudioSessionMediaServicesWereResetNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleEngineConfigurationChange:)
                                  name:AVAudioEngineConfigurationChangeNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleInterruption:)
                                  name:AVAudioSessionInterruptionNotification
                                object:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context != &NotificationManagerContext) {
    return;
  }

  if ([keyPath isEqualToString:@"outputVolume"]) {
    if (self.volumeChangesObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::VOLUME_CHANGE
                                              payload:audioapi::DoubleValuePayload{
                                                          .value = [change[@"new"] floatValue]}];
    }
  }
}

- (void)handleInterruption:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  NSInteger interruptionType =
      [notification.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
  NSInteger interruptionOption =
      [notification.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];

  if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [audioEngine onInterruptionBegin];
      [sessionManager markInactive];
    });

    if (self.audioInterruptionsObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                                              payload:audioapi::InterruptionPayload{
                                                          .type = "began", .shouldResume = false}];
    }

    return;
  }

  bool shouldResume = interruptionOption == AVAudioSessionInterruptionOptionShouldResume;

  if (self.audioInterruptionsObserved) {
    [self.audioAPIModule
        invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                           payload:audioapi::InterruptionPayload{
                                       .type = "ended", .shouldResume = shouldResume}];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onInterruptionEnd:shouldResume]; });
  }
}

- (void)handleSecondaryAudio:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;
  NSInteger secondaryAudioType =
      [notification.userInfo[AVAudioSessionSilenceSecondaryAudioHintTypeKey] integerValue];

  if (secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeBegin) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [sessionManager markInactive];
      [audioEngine onInterruptionBegin];
    });

    if (self.audioInterruptionsObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                                              payload:audioapi::InterruptionPayload{
                                                          .type = "began", .shouldResume = false}];
    }
    return;
  }

  bool shouldResume = secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeEnd;

  if (self.audioInterruptionsObserved) {
    [self.audioAPIModule
        invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                           payload:audioapi::InterruptionPayload{
                                       .type = "ended", .shouldResume = shouldResume}];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onInterruptionEnd:shouldResume]; });
  }
}

- (void)handleRouteChange:(NSNotification *)notification
{
  AUDIOAPI_TRACE_SCOPE(@"routeChange");

  NSInteger routeChangeReason =
      [notification.userInfo[AVAudioSessionRouteChangeReasonKey] integerValue];
  NSString *reasonStr;

  switch (routeChangeReason) {
    case AVAudioSessionRouteChangeReasonUnknown:
      reasonStr = @"Unknown";
      break;
    case AVAudioSessionRouteChangeReasonOverride:
      reasonStr = @"Override";
      break;
    case AVAudioSessionRouteChangeReasonCategoryChange:
      reasonStr = @"CategoryChange";
      break;
    case AVAudioSessionRouteChangeReasonWakeFromSleep:
      reasonStr = @"WakeFromSleep";
      break;
    case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
      reasonStr = @"NewDeviceAvailable";
      break;
    case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
      reasonStr = @"OldDeviceUnavailable";
      break;
    case AVAudioSessionRouteChangeReasonRouteConfigurationChange:
      reasonStr = @"ConfigurationChange";
      break;
    case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
      reasonStr = @"NoSuitableRouteForCategory";
      break;
    default:
      reasonStr = @"Unknown";
      break;
  }

  [self.audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::ROUTE_CHANGE
                         payload:audioapi::StringPayload{
                                     .name = "reason", .reason = [reasonStr UTF8String]}];

  AVAudioSession *session = [AVAudioSession sharedInstance];

  AUDIOAPI_LOG(
      AudioAPIDiagnosticsCategoryNotifications,
      @"routeChange reason=%@, route is {%@}, session is {%@}",
      reasonStr,
      AudioAPIDescribeRoute(session.currentRoute),
      AudioAPIDescribeSession(session));

  switch (routeChangeReason) {
    case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
    case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
    case AVAudioSessionRouteChangeReasonRouteConfigurationChange:
      // Note the reason a ConfigurationChange is not taken at face value: our
      // own reconfiguration reports itself here, so this is the door a restart
      // walks back through after it has already run.
      AUDIOAPI_LOG(
          AudioAPIDiagnosticsCategoryNotifications,
          @"routeChange reason=%@ is treated as a configuration change, restarting the engine",
          reasonStr);
      [self handleEngineConfigurationChange:nil];
      break;
    default:
      AUDIOAPI_LOG(
          AudioAPIDiagnosticsCategoryNotifications,
          @"routeChange reason=%@ needs no engine restart",
          reasonStr);
      break;
  }
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  if (![audioEngine isInUse] && !sessionManager.isActive) {
    return;
  }

  AUDIOAPI_LOG_FAILURE(
      AudioAPIDiagnosticsCategoryNotifications,
      @"media services were reset, tearing down and rebuilding everything");

  dispatch_async(dispatch_get_main_queue(), ^{
    AUDIOAPI_TRACE_SCOPE(@"mediaServicesReset");

    bool wasSessionActive = sessionManager.isActive;
    [sessionManager markInactive];

    if (wasSessionActive) {
      [sessionManager ensureActive:true error:nil];
    }

    [audioEngine restartAudioEngine];
  });
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  AUDIOAPI_TRACE_SCOPE(@"engineNotification");

  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  // This notification is registered with object:nil, so it also fires for
  // AVAudioEngine instances owned by other libraries in the host app. Without
  // an engine of our own there is nothing to restart, and marking the session
  // inactive would corrupt bookkeeping for apps that only manage the session.
  if (![audioEngine isInUse]) {
    AUDIOAPI_LOG(
        AudioAPIDiagnosticsCategoryNotifications,
        @"configuration change ignored, no engine of ours is in use");
    return;
  }

  uint64_t scheduled = restartsScheduled.fetch_add(1, std::memory_order_relaxed) + 1;
  uint64_t drained = restartsDrained.load(std::memory_order_relaxed);

  AUDIOAPI_LOG(
      AudioAPIDiagnosticsCategoryNotifications,
      @"scheduling a restart on the main queue, %llu scheduled and %llu run so far (%llu in "
      @"flight)",
      scheduled,
      drained,
      scheduled - drained);

  dispatch_async(dispatch_get_main_queue(), ^{
    AUDIOAPI_TRACE_SCOPE(@"restartDispatch");

    uint64_t run = restartsDrained.fetch_add(1, std::memory_order_relaxed) + 1;
    uint64_t outstanding = restartsScheduled.load(std::memory_order_relaxed) - run;

    // Reported before the work, not after: a restart that never returns because
    // it re-entered the notification that scheduled it leaves no trailing line.
    AUDIOAPI_LOG(
        AudioAPIDiagnosticsCategoryNotifications,
        @"running scheduled restart %llu, %llu still queued behind it",
        run,
        outstanding);

    [sessionManager markInactive];
    [audioEngine restartAudioEngine];
  });
}

- (void)startPollingSecondaryAudioHint
{
  if (self.hintPollingTimer) {
    [self.hintPollingTimer invalidate];
    self.hintPollingTimer = nil;
  }

  self.wasOtherAudioPlaying = false;
  self.hintPollingTimer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                                           target:self
                                                         selector:@selector(checkSecondaryAudioHint)
                                                         userInfo:nil
                                                          repeats:true];

  [[NSRunLoop mainRunLoop] addTimer:self.hintPollingTimer forMode:NSRunLoopCommonModes];
}

- (void)stopPollingSecondaryAudioHint
{
  [self.hintPollingTimer invalidate];
  self.hintPollingTimer = nil;
}

- (void)checkSecondaryAudioHint
{
  BOOL shouldSilence = [AVAudioSession sharedInstance].secondaryAudioShouldBeSilencedHint;

  if (shouldSilence == self.wasOtherAudioPlaying) {
    return;
  }

  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  self.wasOtherAudioPlaying = shouldSilence;

  if (shouldSilence) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [sessionManager markInactive];
      [audioEngine onInterruptionBegin];
    });
    if (self.audioInterruptionsObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                                              payload:audioapi::InterruptionPayload{
                                                          .type = "began", .shouldResume = false}];
    }

    return;
  }

  if (self.audioInterruptionsObserved) {
    [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                                            payload:audioapi::InterruptionPayload{
                                                        .type = "ended", .shouldResume = true}];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onInterruptionEnd:true]; });
  }
}

@end
