#import <IOSAudioManager.h>

@implementation IOSAudioManager

- (instancetype)init
{
  if (self == [super init]) {
    printf("Initializing IOSAudioManager\n");

    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];

    [self configureAudioSession];
    [self configureNotifications];
  }

  return self;
}

- (void)cleanup
{
  printf("Cleaning up IOSAudioManager\n");

  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
  self.sourceNodes = nil;
  self.audioSession = nil;
  self.sourceFormats = nil;
  self.notificationCenter = nil;
}

- (bool)configureAudioSession
{
  NSError *error = nil;

  if (!self.audioSession) {
    self.audioSession = [AVAudioSession sharedInstance];
  }

  // TODO: take from dict
  [self.audioSession setCategory:AVAudioSessionCategoryPlayback
                            mode:AVAudioSessionModeDefault
                         options:AVAudioSessionCategoryOptionDuckOthers | AVAudioSessionCategoryOptionAllowBluetooth |
                         AVAudioSessionCategoryOptionAllowAirPlay
                           error:&error];

  if (error != nil) {
    NSLog(@"Error while configuring audio session: %@", [error localizedDescription]);
    return false;
  }

  [self.audioSession setActive:true error:&error];

  if (error != nil) {
    NSLog(@"Error while activating audio session: %@", [error localizedDescription]);
    return false;
  }

  return true;
}

- (bool)configureNotifications
{
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
    NSLog(@"Error while rebuilding audio engine: %@", [error localizedDescription]);
    return false;
  }

  return true;
}

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format
{
  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];

  [self.sourceNodes setValue:sourceNode forKey:sourceNodeId];
  [self.sourceFormats setValue:format forKey:sourceNodeId];

  [self.audioEngine attachNode:sourceNode];
  [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];

  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
  [self.audioEngine detachNode:sourceNode];

  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];
}

- (void)handleInterruption:(NSNotification *)notification
{
  // TODO: parse notification user info and pause/restart the engine
}

- (void)handleRouteChange:(NSNotification *)notification
{
  // TODO: pause the engine if notification is device unavailable and there is no new device
}

- (void)handleMediaServicesReset:(NSNotification *)notification
{
  // TODO: pause and panic or just rebuild?
}

- (void)handleEngineConfigurationChange:(NSNotification *)notification
{
  // TODO: add variable isRunning?

  [self rebuildAudioEngine];
}

@end
