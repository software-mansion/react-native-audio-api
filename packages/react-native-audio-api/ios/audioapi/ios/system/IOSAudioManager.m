#import <IOSAudioManager.h>

@implementation IOSAudioManager

- (instancetype)init
{
  if (self == [super init]) {
    NSLog(@"[IOSAudioManager] init");

    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];

    self.sessionCategory = AVAudioSessionCategoryPlayback;
    self.sessionMode = AVAudioSessionModeDefault;
    self.sessionOptions = AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionAllowBluetooth |
        AVAudioSessionCategoryOptionAllowAirPlay;

    [self configureAudioSession];
    [self configureNotifications];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[IOSAudioManager] cleanup");
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
  self.sourceNodes = nil;
  self.audioSession = nil;
  self.sourceFormats = nil;
  self.notificationCenter = nil;
}

- (float)getSampleRate
{
  return [self.audioSession sampleRate];
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

- (bool)rebuildAudioEngine
{
  NSLog(@"[IOSAudioManager] rebuildAudioEngine");
  NSError *error = nil;

  if ([self.audioEngine isRunning]) {
    return true;
  }

  for (id sourceNodeId in self.sourceNodes) {
    AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
    AVAudioFormat *format = [self.sourceFormats valueForKey:sourceNodeId];

    [self.audioEngine attachNode:sourceNode];
    [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];
  }

  if ([self.audioEngine isRunning]) {
    return true;
  }

  if (![self.audioEngine startAndReturnError:&error]) {
    NSLog(@"Error while rebuilding audio engine: %@", [error debugDescription]);
    return false;
  }

  return true;
}

- (void)startEngine
{
  NSLog(@"[IOSAudioManager] startEngine");
  NSError *error = nil;

  if ([self.audioEngine isRunning]) {
    return;
  }

  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"Error while starting the audio engine: %@", [error debugDescription]);
  }
}

- (void)stopEngine
{
  NSLog(@"[IOSAudioManager] stopEngine");
  if (![self.audioEngine isRunning]) {
    return;
  }

  [self.audioEngine pause];
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

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format
{
  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  NSLog(@"[IOSAudioManager] attaching new source node with ID: %@", sourceNodeId);

  [self.sourceNodes setValue:sourceNode forKey:sourceNodeId];
  [self.sourceFormats setValue:format forKey:sourceNodeId];

  [self.audioEngine attachNode:sourceNode];
  [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];

  if ([self.sourceNodes count] == 1) {
    [self startEngine];
  }

  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  NSLog(@"[IOSAudioManager] detaching source nde with ID: %@", sourceNodeId);

  AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
  [self.audioEngine detachNode:sourceNode];

  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];

  if ([self.sourceNodes count] == 0) {
    [self stopEngine];
  }
}

- (void)handleInterruption:(NSNotification *)notification
{
  NSError *error;
  UInt8 type = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
  UInt8 option = [[notification.userInfo valueForKey:AVAudioSessionInterruptionOptionKey] intValue];

  if (type == AVAudioSessionInterruptionTypeBegan) {
    self.isInterrupted = true;
    NSLog(@"[IOSAudioManager] Detected interruption, stopping the engine");

    if (self.isRunning) {
      [self.audioEngine pause];
    }

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
    [self rebuildAudioEngine];
  }

  if (!self.isRunning) {
    return;
  }

  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"[IOSAudioManager] Error while restarting the engine, reason: %@", [error debugDescription]);
  }
}

- (void)handleRouteChange:(NSNotification *)notification
{
  // TODO: pause the engine if notification is "device unavailable" and there is no new device
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  // TODO: teardown and rebuild everything from scratch 🫠
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  if (!self.isRunning) {
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
    [self rebuildAudioEngine];
  });
}

@end
