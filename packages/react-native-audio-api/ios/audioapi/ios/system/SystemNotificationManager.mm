#import <UIKit/UIKit.h>

#import <audioapi/events/AudioEvent.h>
#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/SystemNotificationManager.h>
#define LOG_MODULE SystemNotificationManager
#import <audioapi/ios/utils/Log.h>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/AudioContextRegistry.h>
#include <audioapi/core/inputs/ActiveRecorderHandle.h>

@implementation SystemNotificationManager

static NSString *NotificationManagerContext = @"SystemNotificationManagerContext";

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
  [self observeVolumeChanges:NO];
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
    [[AVAudioSession sharedInstance] removeObserver:self
                                         forKeyPath:@"outputVolume"
                                            context:(void *)&NotificationManagerContext];
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
  [self.notificationCenter addObserver:self
                              selector:@selector(handleWillEnterForeground:)
                                  name:UIApplicationWillEnterForegroundNotification
                                object:nil];
  [self.notificationCenter addObserver:self
                              selector:@selector(handleDidBecomeActive:)
                                  name:UIApplicationDidBecomeActiveNotification
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

- (void)handleWillEnterForeground:(NSNotification *)notification
{
  AUDIO_LOG_TRACE_FUNCTION();
  [self retryInterruptionRecoveryIfNeeded];
}

- (void)handleDidBecomeActive:(NSNotification *)notification
{
  AUDIO_LOG_TRACE_FUNCTION();
  [self retryInterruptionRecoveryIfNeeded];
}

- (void)emitInterruptionBegan
{
  if (!self.audioInterruptionsObserved) {
    return;
  }

  [self.audioAPIModule invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                                          payload:audioapi::InterruptionPayload{
                                                      .type = "began", .shouldResume = false}];
}

- (void)emitInterruptionEnded:(bool)shouldResume
{
  if (!self.audioInterruptionsObserved) {
    return;
  }

  [self.audioAPIModule
      invokeHandlerWithEventName:audioapi::AudioEvent::INTERRUPTION
                         payload:audioapi::InterruptionPayload{
                                     .type = "ended", .shouldResume = shouldResume}];
}

- (void)handleInterruptionBegin
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{
    AUDIO_LOG_TRACE("handleInterruptionBegin");
    bool transitioned = [audioEngine onInterruptionBegin];
    // A recovery still pending from an earlier interruption waits for this one to end too.
    if (transitioned || self.interruptionRecoveryState != InterruptionRecoveryStateNone) {
      self.interruptionRecoveryState = InterruptionRecoveryStateInterrupted;
    }
    if (transitioned) {
      self.interruptionShouldResume = nil;
      [self emitInterruptionBegan];
    }
    AUDIO_LOG_DEBUG(
        "handleInterruptionBegin transitioned=%{public}s emittedToJs=%{public}s recoveryState=%ld engineState=%{public}s",
        logBool(transitioned),
        logBool(transitioned && self.audioInterruptionsObserved),
        (long)self.interruptionRecoveryState,
        audioEngineStateName([audioEngine getState]));
  });
}

- (void)handleInterruptionEnd:(bool)shouldResume
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{
    AUDIO_LOG_TRACE("handleInterruptionEnd shouldResume=%{public}s", logBool(shouldResume));
    [audioEngine onInterruptionEnd];
    if (self.interruptionRecoveryState == InterruptionRecoveryStateNone) {
      AUDIO_LOG_DEBUG(
          "handleInterruptionEnd ignored: recoveryState=none engineState=%{public}s",
          audioEngineStateName([audioEngine getState]));
      return;
    }
    [self restoreAfterInterruption:shouldResume];
  });
}

/// The resume policy applied when an interruption ends.
///
/// `shouldResume` refers to playback only
/// (https://developer.apple.com/documentation/avfaudio/avaudiosession/interruptionoptions/shouldresume),
/// so contexts that were playing follow it, while a recording that was in progress always
/// resumes. Contexts go first, for two reasons: a suspended context detaches its source, so the
/// recording restarts the engine without playback; and a context that starts playback restarts
/// the engine, which would cut into a recording that was already capturing.
/// @return false when a recording or a context meant to continue could not restart.
- (bool)restoreInterruptedParticipants:(bool)shouldResume
{
  bool restored = true;

  for (const auto &context : audioapi::AudioContextRegistry::global().liveContexts()) {
    if (shouldResume) {
      restored = context->resumeAfterInterruption() && restored;
    } else {
      context->suspendAfterInterruption();
    }
  }

  return audioapi::ActiveRecorderHandle::global().resumeIfInterrupted() && restored;
}

/// Main queue only.
- (void)restoreAfterInterruption:(bool)shouldResume
{
  bool restored = [self restoreInterruptedParticipants:shouldResume];
  AUDIO_LOG_DEBUG(
      "restoreAfterInterruption shouldResume=%{public}s restored=%{public}s emittedToJs=%{public}s",
      logBool(shouldResume),
      logBool(restored),
      logBool(restored && self.audioInterruptionsObserved));

  if (!restored) {
    self.interruptionRecoveryState = InterruptionRecoveryStatePending;
    return;
  }

  self.interruptionRecoveryState = InterruptionRecoveryStateNone;
  [self emitInterruptionEnded:shouldResume];
}

- (bool)shouldResumeForInterruptionRecovery
{
  if (self.interruptionShouldResume == nil) {
    return true;
  }

  return self.interruptionShouldResume.boolValue;
}

/// Main queue only. The session often cannot be activated while the app is in the background,
/// so an interruption that ended there is retried once the app is in the foreground again.
- (void)retryInterruptionRecoveryIfNeeded
{
  AUDIO_LOG_TRACE_FUNCTION();
  if (self.interruptionRecoveryState != InterruptionRecoveryStatePending) {
    AUDIO_LOG_TRACE(
        "retryInterruptionRecoveryIfNeeded skipping recoveryState=%ld",
        (long)self.interruptionRecoveryState);
    return;
  }

  bool shouldResume = [self shouldResumeForInterruptionRecovery];
  AUDIO_LOG_DEBUG(
      "retryInterruptionRecoveryIfNeeded retrying shouldResume=%{public}s interruptionShouldResume=%{public}s",
      logBool(shouldResume),
      self.interruptionShouldResume == nil ? "nil"
                                           : logBool(self.interruptionShouldResume.boolValue));
  [self restoreAfterInterruption:shouldResume];
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSInteger interruptionType =
      [notification.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
  NSInteger interruptionOption =
      [notification.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];

  if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
    AUDIO_LOG_TRACE("AVAudioSession interruption began");
    [self handleInterruptionBegin];
    return;
  }

  bool shouldResume = interruptionOption == AVAudioSessionInterruptionOptionShouldResume;
  AUDIO_LOG_TRACE(
      "AVAudioSession interruption ended shouldResume=%{public}s", logBool(shouldResume));
  AUDIO_LOG_DEBUG("AVAudioSession interruption ended option=%ld", (long)interruptionOption);

  dispatch_async(dispatch_get_main_queue(), ^{ self.interruptionShouldResume = @(shouldResume); });
  [self handleInterruptionEnd:shouldResume];
}

- (void)handleSecondaryAudio:(NSNotification *)notification
{
  NSInteger secondaryAudioType =
      [notification.userInfo[AVAudioSessionSilenceSecondaryAudioHintTypeKey] integerValue];

  if (secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeBegin) {
    AUDIO_LOG_TRACE("secondary audio hint began");
    [self handleInterruptionBegin];
    return;
  }

  bool shouldResume = secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeEnd;
  AUDIO_LOG_TRACE("secondary audio hint ended shouldResume=%{public}s", logBool(shouldResume));

  [self handleInterruptionEnd:shouldResume];
}

- (void)handleRouteChange:(NSNotification *)notification
{
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

  switch (routeChangeReason) {
    case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
    case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
    case AVAudioSessionRouteChangeReasonRouteConfigurationChange: {
      // Not a restart: a rebuild posts RouteConfigurationChange itself, so restarting would loop.
      // Real format changes arrive as AVAudioEngineConfigurationChangeNotification.
      AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
      dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onRouteChange]; });
      break;
    }
    default:
      break;
  }
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onMediaServicesReset]; });
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  // Weak, so a queued handler does not keep a replaced engine alive.
  __weak AVAudioEngine *changedEngine = notification.object;

  dispatch_async(dispatch_get_main_queue(), ^{
    AUDIO_LOG_TRACE("engine configuration change");
    // object:nil also delivers other libraries' engines and ones we already replaced;
    // only a change on our current engine needs a restart.
    if (![audioEngine isCurrentEngine:changedEngine]) {
      AUDIO_LOG_DEBUG("engine configuration change ignored: not the current engine");
      return;
    }

    [audioEngine onConfigurationChange];
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

  self.wasOtherAudioPlaying = shouldSilence;

  AUDIO_LOG_TRACE("secondaryAudioShouldBeSilencedHint=%{public}s", logBool(shouldSilence));

  if (shouldSilence) {
    [self handleInterruptionBegin];
    return;
  }

  [self handleInterruptionEnd:[self shouldResumeForInterruptionRecovery]];
}

@end
