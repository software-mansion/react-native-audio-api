#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation AudioEngine

static AudioEngine *_sharedInstance = nil;

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
  @throw [NSException exceptionWithName:@"Singleton" reason:@"Use +[AudioEngine sharedInstance]" userInfo:nil];
  return nil;
}

- (instancetype)initPrivate
{
  if (self = [super init]) {
    self.tapId = nil;
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.audioEngine.mainMixerNode.outputVolume = 1;
    self.audioEngine.inputNode.volume = 1;
    self.inputMixer = [[AVAudioMixerNode alloc] init];

    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[AudioEngine] cleanup");
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.tapId = nil;
  self.inputMixer = nil;
  self.audioEngine = nil;
  self.sourceNodes = nil;
  self.sourceFormats = nil;
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
    NSLog(@"Error while rebuilding audio engine: %@", [error debugDescription]);
    return false;
  }

  return true;
}

- (void)startEngine
{
  NSLog(@"[AudioEngine] startEngine");
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
  NSLog(@"[AudioEngine] stopEngine");
  if (![self.audioEngine isRunning]) {
    return;
  }

  [self.audioEngine pause];
}

- (bool)isRunning
{
  return [self.audioEngine isRunning];
}

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format
{
  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  NSLog(@"[AudioEngine] attaching new source node with ID: %@", sourceNodeId);

  [self.sourceNodes setValue:sourceNode forKey:sourceNodeId];
  [self.sourceFormats setValue:format forKey:sourceNodeId];

  [self.audioEngine attachNode:sourceNode];
  [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];

  [self startIfNecessary];
  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  NSLog(@"[AudioEngine] detaching source node with ID: %@", sourceNodeId);

  AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
  [self.audioEngine detachNode:sourceNode];

  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];

  [self stopIfNecessary];
}

- (NSString *)installInputTap:(AVAudioNodeTapBlock)tapBlock
{
  if (self.tapId != nil) {
    return nil;
  }

  NSString *tapId = [[NSUUID UUID] UUIDString];
  self.tapId = tapId;

  [self startIfNecessary];

  AVAudioInputNode *input = [self.audioEngine inputNode];
  AVAudioFormat *format = [input inputFormatForBus:0];

  [self.audioEngine attachNode:self.inputMixer];
  [self.audioEngine connect:input to:self.inputMixer format:format];
  [self.inputMixer installTapOnBus:0 bufferSize:2048 format:format block:tapBlock];
  return tapId;
}

- (void)removeInputTap:(NSString *)tapId
{
  if (self.tapId == nil || self.tapId != tapId) {
    return;
  }

  [self.inputMixer removeTapOnBus:0];
  [self.audioEngine detachNode:self.inputMixer];
  self.tapId = nil;
}

- (void)startIfNecessary
{
  if ([self isRunning]) {
    return;
  }

  if ([self.sourceNodes count] > 0 || self.tapId != nil) {
    [[AudioSessionManager sharedInstance] setActive:true];
    [self startEngine];
  }
}

- (void)stopIfNecessary
{
  if (![self isRunning]) {
    return;
  }

  if ([self.sourceNodes count] == 0 && self.tapId == nil) {
    [self stopEngine];
    [[AudioSessionManager sharedInstance] setActive:false];
  }
}

@end
