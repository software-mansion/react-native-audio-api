#import <UIKit/UIKit.h>

#import <audioapi/events/AudioEvent.h>
#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/SystemNotificationManager.h>

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
  // Returning to the foreground is the most reliable moment at which the system stops
  // refusing to start the engine, since the refusals happen while the device is locked.
  [self.notificationCenter addObserver:self
                              selector:@selector(handleApplicationDidBecomeActive:)
                                  name:UIApplicationDidBecomeActiveNotification
                                object:nil];
}

- (void)handleApplicationDidBecomeActive:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine retryPendingRestartIfNeeded]; });
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

  // Whoever owns interruption handling, an ended interruption is an opportunity to finish a
  // restart that was refused or deferred earlier. `onInterruptionEnd` above only recovers an
  // engine still marked interrupted, and a configuration change arriving during the
  // interruption can have moved it out of that state; this covers what it leaves behind.
  // Queued after it, so a successful resume clears the pending restart and this does nothing.
  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine retryPendingRestartIfNeeded]; });
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

  // A new route can lift whatever made the system refuse an earlier restart, and it
  // arrives without an engine configuration change whenever the engine is not running.
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine retryPendingRestartIfNeeded]; });
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(
      @"[NotificationManager] Media services have been reset, tearing down and rebuilding everything.");
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  dispatch_async(dispatch_get_main_queue(), ^{
    [sessionManager markInactive];
    [sessionManager ensureActive:true error:nil];
    [audioEngine restartAudioEngine];
  });
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  dispatch_async(dispatch_get_main_queue(), ^{
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
