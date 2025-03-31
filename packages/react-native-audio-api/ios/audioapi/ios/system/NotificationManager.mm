#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#import <audioapi/ios/system/NotificationManager.h>

@implementation NotificationManager

static NotificationManager *_sharedInstance = nil;

+ (instancetype)sharedInstance
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _sharedInstance = [[self alloc] initPrivate];
  });
  return _sharedInstance;
}

- (instancetype)init
{
  @throw [NSException exceptionWithName:@"Singleton" reason:@"Use +[NotificationManager sharedInstance]" userInfo:nil];
  return nil;
}

- (instancetype)initPrivate
{
  if (self = [super init]) {
    self.notificationCenter = [NSNotificationCenter defaultCenter];

    [self configureNotifications];
  }
  return self;
}

- (void)cleanup
{
  NSLog(@"[NotificationManager] cleanup");

  self.notificationCenter = nil;
}

- (void)configureNotifications
{
  [self.notificationCenter addObserver:self
                              selector:@selector(handleInterruption:)
                                  name:AVAudioSessionInterruptionNotification
                                object:nil];
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
  NSError *error;
  UInt8 type = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  UInt8 option = [[notification.userInfo valueForKey:AVAudioSessionInterruptionOptionKey] intValue];
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *audioSessionManager = [AudioSessionManager sharedInstance];

  if (type == AVAudioSessionInterruptionTypeBegan) {
    self.isInterrupted = true;
    NSLog(@"[NotificationManager] Detected interruption, stopping the engine");

    [audioEngine stopEngine];

    return;
  }

  if (type != AVAudioSessionInterruptionTypeEnded) {
    NSLog(@"[NotificationManager] Unexpected interruption state, chilling");
    return;
  }

  self.isInterrupted = false;

  if (option != AVAudioSessionInterruptionOptionShouldResume) {
    NSLog(@"[NotificationManager] Interruption ended, but engine is not allowed to resume");
    return;
  }

  NSLog(@"[NotificationManager] Interruption ended, resuming the engine");
  bool success = [audioSessionManager setActive:true error:&error];

  if (!success) {
    NSLog(@"[NotificationManager] Unable to activate the audio session, reason: %@", [error debugDescription]);
    return;
  }

  if (self.hadConfigurationChange) {
    [audioEngine rebuildAudioEngine];
  }

  [audioEngine startEngine];
}

- (void)handleRouteChange:(NSNotification *)notification
{
  UInt8 reason = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  NSLog(@"[NotificationManager] Route change detected, reason: %u", reason);
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  if (reason == AVAudioSessionRouteChangeReasonOldDeviceUnavailable) {
    NSLog(@"[NotificationManager] The previously used audio device became unavailable. Audio engine paused");
    [audioEngine stopEngine];
  }
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
