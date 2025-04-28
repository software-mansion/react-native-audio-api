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
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.inputNode = nil;

    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];

    [[AudioSessionManager sharedInstance] setActive:true];
  }

  return self;
}

- (void)cleanup
{
  NSLog(@"[AudioEngine] cleanup");
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
  self.sourceNodes = nil;
  self.sourceFormats = nil;
  self.inputNode = nil;
  
  [[AudioSessionManager sharedInstance] setActive:false];
}

- (bool)rebuildAudioEngine
{
  if ([self.audioEngine isRunning]) {
    return true;
  }

  for (id sourceNodeId in self.sourceNodes) {
    AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
    AVAudioFormat *format = [self.sourceFormats valueForKey:sourceNodeId];

    [self.audioEngine attachNode:sourceNode];
    [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];
  }

  [self.audioEngine attachNode:self.inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:self.inputNode format:nil];

  [self startEngine];

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

  [self.audioEngine stop];
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

- (void)attachInputNode:(AVAudioSinkNode *)inputNode
{
  self.inputNode = inputNode;

  [self.audioEngine attachNode:inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:inputNode format:nil];

  [self startIfNecessary];
}

- (void)detachInputNode
{
  [self.audioEngine detachNode:self.inputNode];
  self.inputNode = nil;

  [self stopIfNecessary];
}

- (void)startIfNecessary
{
  if ([self isRunning]) {
    return;
  }

  if ([self.sourceNodes count] > 0 || self.inputNode != nil) {
    [self startEngine];
  }
}

- (void)stopIfNecessary
{
  if (![self isRunning]) {
    return;
  }

  if ([self.sourceNodes count] == 0 && self.inputNode == nil) {
    [self stopEngine];
  }
}

@end
