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
    NSDictionary *body = @{@"value" : [NSNumber numberWithFloat:[change[@"new"] floatValue]]};
    if (self.volumeChangesObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:@"volumeChange" eventBody:body];
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
      NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  bool shouldResume = interruptionOption == AVAudioSessionInterruptionOptionShouldResume;

  if (self.audioInterruptionsObserved) {
    NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @(shouldResume)};
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
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
      NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }
    return;
  }

  bool shouldResume = secondaryAudioType == AVAudioSessionSilenceSecondaryAudioHintTypeEnd;

  if (self.audioInterruptionsObserved) {
    NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @(shouldResume)};
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
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

  NSDictionary *body = @{@"reason" : reasonStr};

  [self.audioAPIModule invokeHandlerWithEventName:@"routeChange" eventBody:body];
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(
      @"[NotificationManager] Media services have been reset, tearing down and rebuilding everything.");
  AudioEngine *audioEngine = self.audioAPIModule.audioEngine;

  dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine restartAudioEngine]; });
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
    NSDictionary *body = @{@"type" : @"began", @"shouldResume" : @false};

    if (self.audioInterruptionsObserved) {
      [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
    }

    return;
  }

  NSDictionary *body = @{@"type" : @"ended", @"shouldResume" : @true};

  if (self.audioInterruptionsObserved) {
    [self.audioAPIModule invokeHandlerWithEventName:@"interruption" eventBody:body];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{ [audioEngine onInterruptionEnd:true]; });
  }
}

@end
