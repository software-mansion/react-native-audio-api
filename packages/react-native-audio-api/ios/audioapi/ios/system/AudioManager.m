#import <audioapi/ios/system/AudioManager.h>

@import MediaPlayer;

#define MEDIA_DICT                                                \
  @{                                                              \
    @"album" : MPMediaItemPropertyAlbumTitle,                     \
    @"artist" : MPMediaItemPropertyArtist,                        \
    @"genre" : MPMediaItemPropertyGenre,                          \
    @"duration" : MPMediaItemPropertyPlaybackDuration,            \
    @"elapsedTime" : MPNowPlayingInfoPropertyElapsedPlaybackTime, \
    @"title" : MPMediaItemPropertyTitle,                          \
    @"isLiveStream" : MPNowPlayingInfoPropertyIsLiveStream        \
  }

@implementation AudioManager

- (instancetype)init
{
  if (self == [super init]) {
    NSLog(@"[IOSAudioManager] init");

    self.audioEngine = [[AudioEngine alloc] init];

    self.sessionCategory = AVAudioSessionCategoryPlayback;
    self.sessionMode = AVAudioSessionModeDefault;
    self.sessionOptions = AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionAllowBluetooth |
        AVAudioSessionCategoryOptionAllowAirPlay;

    [self configureAudioSession];
    [self configureNotifications];
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[IOSAudioManager] cleanup");
  [AudioEngine cleanup];

  self.audioSession = nil;
  self.notificationCenter = nil;
}

- (float)getSampleRate
{
  return [self.audioSession sampleRate];
}

- (void)setNowPlaying:(NSDictionary *)info
{
  MPNowPlayingInfoCenter *playingInfoCenter = [MPNowPlayingInfoCenter defaultCenter];
  NSMutableDictionary *mediaDict = [NSMutableDictionary dictionary];

  for (NSString *key in MEDIA_DICT) {
    if ([info objectForKey:key] != nil) {
      [mediaDict setValue:[info objectForKey:key] forKey:[MEDIA_DICT objectForKey:key]];
    }
  }

  playingInfoCenter.nowPlayingInfo = mediaDict;
}

- (void)setSessionOptionsWithCategory:(AVAudioSessionCategory)category
                                 mode:(AVAudioSessionMode)mode
                              options:(AVAudioSessionCategoryOptions)options
{
  bool hasDirtySettings = false;

  if (self.sessionCategory != category) {
    hasDirtySettings = true;
    self.sessionCategory = category;
  }

  if (self.sessionMode != mode) {
    hasDirtySettings = true;
    self.sessionMode = mode;
  }

  if (self.sessionOptions != options) {
    hasDirtySettings = true;
    self.sessionOptions = options;
  }

  if (hasDirtySettings) {
    [self configureAudioSession];
  }
}

- (bool)configureAudioSession
{
  NSLog(
      @"[IOSAudioManager] configureAudioSession, category: %@, mode: %@, options: %lu",
      self.sessionCategory,
      self.sessionMode,
      (unsigned long)self.sessionOptions);

  NSError *error = nil;

  if (!self.audioSession) {
    self.audioSession = [AVAudioSession sharedInstance];
  }

  [self.audioSession setCategory:self.sessionCategory mode:self.sessionMode options:self.sessionOptions error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error debugDescription]);
    return false;
  }

  [self.audioSession setActive:true error:&error];

  if (error != nil) {
    NSLog(@"Error while activating audio session: %@", [error debugDescription]);
    return false;
  }

  return true;
}

- (bool)configureNotifications
{
  NSLog(@"[IOSAudioManager] configureNotifications");
  if (!self.notificationCenter) {
    self.notificationCenter = [NSNotificationCenter defaultCenter];
  }

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

  return true;
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSError *error;
  UInt8 type = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  UInt8 option = [[notification.userInfo valueForKey:AVAudioSessionInterruptionOptionKey] intValue];

  if (type == AVAudioSessionInterruptionTypeBegan) {
    self.isInterrupted = true;
    NSLog(@"[IOSAudioManager] Detected interruption, stopping the engine");

    [AudioEngine stopEngine];

    return;
  }

  if (type != AVAudioSessionInterruptionTypeEnded) {
    NSLog(@"[IOSAudioManager] Unexpected interruption state, chilling");
    return;
  }

  self.isInterrupted = false;

  if (option != AVAudioSessionInterruptionOptionShouldResume) {
    NSLog(@"[IOSAudioManager] Interruption ended, but engine is not allowed to resume");
    return;
  }

  NSLog(@"[IOSAudioManager] Interruption ended, resuming the engine");
  bool success = [self.audioSession setActive:true error:&error];

  if (!success) {
    NSLog(@"[IOSAudioManager] Unable to activate the audio session, reason: %@", [error debugDescription]);
    return;
  }

  if (self.hadConfigurationChange) {
    [AudioEngine rebuildAudioEngine];
  }

  [AudioEngine startEngine];
}

- (void)handleRouteChange:(NSNotification *)notification
{
  UInt8 reason = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  NSLog(@"[IOSAudioManager] Route change detected, reason: %u", reason);

  if (reason == AVAudioSessionRouteChangeReasonOldDeviceUnavailable) {
    NSLog(@"[IOSAudioManager] The previously used audio device became unavailable. Audio engine paused");
    [AudioEngine stopEngine];
  }
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  NSLog(@"[IOSAudioManager] Media services have been reset, tearing down and rebuilding everything.");
  [self cleanup];
  [self configureAudioSession];
  [self configureNotifications];
  [AudioEngine rebuildAudioEngine];
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  if (![AudioEngine isRunning]) {
    NSLog(@"[IOSAudioManager] detected engine configuration change when engine is not running, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  if (self.isInterrupted) {
    NSLog(@"[IOSAudioManager] detected engine configuration change during interruption, marking for rebuild");
    self.hadConfigurationChange = true;
    return;
  }

  NSLog(@"[IOSAudioManager] detected engine configuration change, rebuilding the graph");
  dispatch_async(dispatch_get_main_queue(), ^{
    [AudioEngine rebuildAudioEngine];
  });
}

@end
