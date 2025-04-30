#import <audioapi/ios/AudioManagerModule.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/NotificationManager.h>

@implementation NotificationManager

static NotificationManager *_sharedInstance = nil;
static NSString *VolumeObservationContext = @"VolumeObservationContext";

+ (instancetype)sharedInstanceWithAudioManagerModule:(AudioManagerModule *)audioManagerModule
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _sharedInstance = [[self alloc] initPrivateWithAudioManagerModule:audioManagerModule];
  });
  return _sharedInstance;
}

- (instancetype)init
{
  @throw [NSException exceptionWithName:@"Singleton" reason:@"Use +[NotificationManager sharedInstance]" userInfo:nil];
  return nil;
}

- (instancetype)initPrivateWithAudioManagerModule:(AudioManagerModule *)audioManagerModule
{
  if (self = [super init]) {
    self.audioManagerModule = audioManagerModule;
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
  if (self.audioInterruptionsObserved == enabled) {
    return;
  }

  if (enabled) {
    [self.notificationCenter addObserver:self
                                selector:@selector(handleInterruption:)
                                    name:AVAudioSessionInterruptionNotification
                                  object:nil];
  } else {
    [self.notificationCenter removeObserver:self name:AVAudioSessionInterruptionNotification object:nil];
  }

  self.audioInterruptionsObserved = enabled;
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
                                         context:(void *)&VolumeObservationContext];
  } else {
    [[AVAudioSession sharedInstance] removeObserver:self forKeyPath:@"outputVolume" context:nil];
  }
  self.volumeChangesObserved = enabled;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if ([keyPath isEqualToString:@"outputVolume"] && context == &VolumeObservationContext) {
    NSNumber *volume = [NSNumber numberWithFloat:[change[@"new"] floatValue]];
    [self.audioManagerModule sendEventWithName:@"onVolumeChange" body:@{@"value" : volume}];
  }
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
}

- (void)handleInterruption:(NSNotification *)notification
{
  if (!self.audioInterruptionsObserved) {
    return;
  }

  NSInteger interruptionType = [notification.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
  NSInteger interruptionOption = [notification.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];

  if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
    [self.audioManagerModule sendEventWithName:@"onInterruption"
                                          body:@{@"type" : @"began", @"shouldResume" : @"false"}];
    return;
  }

  if (interruptionOption == AVAudioSessionInterruptionOptionShouldResume) {
    [self.audioManagerModule sendEventWithName:@"onInterruption" body:@{@"type" : @"ended", @"shouldResume" : @"true"}];
    return;
  }

  [self.audioManagerModule sendEventWithName:@"onInterruption" body:@{@"type" : @"ended", @"shouldResume" : @"false"}];
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
      reasonStr = @"DoubleUnknown";
      break;
  }

  [self.audioManagerModule sendEventWithName:@"onRouteChange" body:@{@"reason" : @"reasonStr"}];
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(@"[NotificationManager] Media services have been reset, tearing down and rebuilding everything.");
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *audioSessionManager = [AudioSessionManager sharedInstance];

  [self cleanup];
  [audioSessionManager configureAudioSession];
  [self configureNotifications];
  [audioEngine rebuildAudioEngine];
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  if (![audioEngine isRunning]) {
    NSLog(
        @"[NotificationManager] detected engine configuration change when engine is not running, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  if (self.isInterrupted) {
    NSLog(@"[NotificationManager] detected engine configuration change during interruption, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    [audioEngine rebuildAudioEngine];
  });
}

@end
