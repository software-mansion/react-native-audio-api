#import <audioapi/ios/AudioAPIModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/NotificationManager.h>

@implementation NotificationManager

static NSString *NotificationManagerContext = @"NotificationManagerContext";

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
  NSLog(@"[NotificationManager] cleanup");

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

    [self.notificationCenter removeObserver:self name:AVAudioSessionSilenceSecondaryAudioHintNotification object:nil];
    return;
  }

  [self.notificationCenter addObserver:self
                              selector:@selector(handleSecondaryAudio:)
                                  name:AVAudioSessionSilenceSecondaryAudioHintNotification
                                object:nil];

  dispatch_async(dispatch_get_main_queue(), ^{
    [self startPollingSecondaryAudioHint];
  });
}

// WARNING: this does not work in a simulator environment, test it on a real device
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
    NSDictionary *body = @{@"value" : [NSNumber numberWithFloat:[change[@"new"] floatValue]]};
    if (self.volumeChangesObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:@"volumeChange" eventBody:body];
    }
  }
}

- (void)handleInterruption:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  NSInteger interruptionType = [notification.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
  NSInteger interruptionOption = [notification.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];

  if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
    [audioEngine markAsInterrupted];

    if (self.audioInterruptionsObserved) {
      NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  if (interruptionOption == AVAudioSessionInterruptionOptionShouldResume) {
    [audioEngine unmarkAsInterrupted];

    if (self.audioInterruptionsObserved) {
      NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @true};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  if (self.audioInterruptionsObserved) {
    NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @false};
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
  }
}

- (void)handleSecondaryAudio:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  NSInteger secondaryAudioType = [notification.userInfo[AVAudioSessionSilenceSecondaryAudioHintTypeKey] integerValue];

  NSLog(@"handleSecondaryAudio");

  if (secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeBegin) {
    [audioEngine markAsInterrupted];

    if (self.audioInterruptionsObserved) {
      NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  if (secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeEnd) {
    [audioEngine unmarkAsInterrupted];

    if (self.audioInterruptionsObserved) {
      NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @true};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  if (self.audioInterruptionsObserved) {
    NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @false};
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
  }
}

- (void)handleRouteChange:(NSNotification *)notification
{
  NSInteger routeChangeReason = [notification.userInfo[AVAudioSessionRouteChangeReasonKey] integerValue];
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

  NSDictionary *body = @{@"reason" : reasonStr};

  [self.audioAPIModule invokeHandlerWithEventName:@"routeChange" eventBody:body];
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(@"[NotificationManager] Media services have been reset, tearing down and rebuilding everything.");
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *audioSessionManager = self.audioAPIModule.audioSessionManager;

  [audioSessionManager reconfigureAudioSession];
  [audioEngine restartAudioEngine];
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;
  AudioSessionManager *sessionManager = self.audioAPIModule.audioSessionManager;

  if (![audioEngine isSupposedToBeRunning]) {
    NSLog(@"[NotificationManager] detected engine configuration change when engine is not running");
    [sessionManager markSettingsAsDirty];
    return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    [sessionManager markSettingsAsDirty];
    [audioEngine rebuildAudioEngineAndStartIfNecessary];
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
                                                          repeats:YES];

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
  self.wasOtherAudioPlaying = shouldSilence;

  if (shouldSilence) {
    [audioEngine markAsInterrupted];
    NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};

    if (self.audioInterruptionsObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  [audioEngine unmarkAsInterrupted];
  NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @true};

  if (self.audioInterruptionsObserved) {
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
  }
}

@end
