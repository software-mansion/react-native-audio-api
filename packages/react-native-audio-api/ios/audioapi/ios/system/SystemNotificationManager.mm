#import <UIKit/UIKit.h>

#import <audioapi/events/AudioEvent.h>
#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/SystemNotificationManager.h>

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
  [self retryInterruptionRecoveryIfNeeded];
}

- (void)handleDidBecomeActive:(NSNotification *)notification
{
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
    bool transitioned = [audioEngine onInterruptionBegin];
    // TODO(IRTMP): remove temporary log.
    NSLog(
        @"[IRTMP] handleInterruptionBegin transitioned=%d recoveryState=%ld",
        transitioned,
        (long)self.interruptionRecoveryState);
    // A recovery still pending from an earlier interruption waits for this one to end too.
    if (transitioned || self.interruptionRecoveryState != InterruptionRecoveryStateNone) {
      self.interruptionRecoveryState = InterruptionRecoveryStateInterrupted;
    }
    if (transitioned) {
      [self emitInterruptionBegan];
    }
  });
}

- (void)handleInterruptionEnd:(bool)shouldResume
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{
    [audioEngine onInterruptionEnd];
    if (self.interruptionRecoveryState == InterruptionRecoveryStateNone) {
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
  // TODO(IRTMP): remove temporary log.
  NSLog(@"[IRTMP] restoreAfterInterruption shouldResume=%d restored=%d", shouldResume, restored);

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
  // TODO(IRTMP): remove temporary log.
  NSLog(
      @"[IRTMP] retryInterruptionRecoveryIfNeeded recoveryState=%ld cachedShouldResume=%@",
      (long)self.interruptionRecoveryState,
      self.interruptionShouldResume);
  if (self.interruptionRecoveryState != InterruptionRecoveryStatePending) {
    return;
  }

  [self restoreAfterInterruption:[self shouldResumeForInterruptionRecovery]];
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSInteger interruptionType =
      [notification.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
  NSInteger interruptionOption =
      [notification.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];

  // TODO(IRTMP): remove temporary log.
  NSLog(
      @"[IRTMP] handleInterruption type=%ld (1 began, 0 ended) option=%ld userInfo=%@",
      (long)interruptionType,
      (long)interruptionOption,
      notification.userInfo);
  if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
    [self handleInterruptionBegin];
    return;
  }

  bool shouldResume = interruptionOption == AVAudioSessionInterruptionOptionShouldResume;

  self.interruptionShouldResume = @(shouldResume);
  [self handleInterruptionEnd:shouldResume];
}

- (void)handleSecondaryAudio:(NSNotification *)notification
{
  NSInteger secondaryAudioType =
      [notification.userInfo[AVAudioSessionSilenceSecondaryAudioHintTypeKey] integerValue];

  // TODO(IRTMP): remove temporary log.
  NSLog(@"[IRTMP] handleSecondaryAudio type=%ld (1 begin, 0 end)", (long)secondaryAudioType);
  if (secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeBegin) {
    [self handleInterruptionBegin];
    return;
  }

  bool shouldResume = secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeEnd;

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
    case AVAudioSessionRouteChangeReasonRouteConfigurationChange:
      [self handleEngineConfigurationChange:nil];
      break;
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

  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onConfigurationChange]; });
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
  // TODO(IRTMP): remove temporary log.
  NSLog(@"[IRTMP] checkSecondaryAudioHint shouldSilence=%d", shouldSilence);

  if (shouldSilence) {
    [self handleInterruptionBegin];
    return;
  }

  [self handleInterruptionEnd:[self shouldResumeForInterruptionRecovery]];
}

@end
