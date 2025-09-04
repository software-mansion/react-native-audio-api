#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation AudioEngine

static AudioEngine *_sharedInstance = nil;

+ (instancetype)sharedInstance
{
  return _sharedInstance;
}

- (instancetype)initWithAudioSessionManager:(AudioSessionManager *)sessionManager
{
  if (self = [super init]) {
    self.isInterrupted = false;
    self.isSupposedToBeRunning = true;
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.inputNode = nil;

    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];
    self.sourceStates = [[NSMutableDictionary alloc] init];

    self.sessionManager = sessionManager;
  }

  _sharedInstance = self;

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
  self.sourceStates = nil;
  self.inputNode = nil;

  [self.sessionManager setActive:false];
  self.sessionManager = nil;
}

- (void)rebuildAudioEngine
{
  for (id sourceNodeId in self.sourceNodes) {
    AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];
    AVAudioFormat *format = [self.sourceFormats valueForKey:sourceNodeId];

    [self.audioEngine attachNode:sourceNode];
    [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];
  }

  if (self.inputNode) {
    [self.audioEngine attachNode:self.inputNode];
    [self.audioEngine connect:self.audioEngine.inputNode to:self.inputNode format:nil];
  }
}

- (bool)rebuildAudioEngineAndStartIfNecessary
{
  [self rebuildAudioEngine];
  return [self startIfNecessary];
}

- (bool)restartAudioEngine
{
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.audioEngine = [[AVAudioEngine alloc] init];
  return [self rebuildAudioEngineAndStartIfNecessary];
}

- (bool)startEngine
{
  NSLog(@"[AudioEngine] startEngine");
  NSError *error = nil;
  self.isSupposedToBeRunning = true;

  if ([self.audioEngine isRunning] && ![self isInterrupted]) {
    NSLog(@"[AudioEngine] Engine is already running");
    return true;
  }

  if (![self.sessionManager setActive:true]) {
    return false;
  }

  if ([self isInterrupted]) {
    NSLog(@"[AudioEngine] rebuilding after interruption");
    [self.audioEngine stop];
    [self.audioEngine reset];

    [self rebuildAudioEngine];

    self.isInterrupted = false;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"Error while starting the audio engine: %@", [error debugDescription]);
    return false;
  }

  return true;
}

- (void)stopEngine
{
  NSLog(@"[AudioEngine] stopEngine");
  if (![self.audioEngine isRunning]) {
    return;
  }

  self.isSupposedToBeRunning = false;
  [self.audioEngine stop];
}

- (void)pauseEngine:(NSString *)sourceNodeId
{
  NSLog(@"[AudioEngine] pauseEngine");
  self.isSupposedToBeRunning = false;

  if (![self.audioEngine isRunning]) {
    return;
  }

  [self.sourceStates setValue:@false forKey:sourceNodeId];
  [self pauseIfNecessary];
}

- (bool)isRunning
{
  return [self.audioEngine isRunning] && !self.isInterrupted;
}

- (void)markAsInterrupted
{
  self.isInterrupted = true;
  self.isSupposedToBeRunning = false;
}

- (void)unmarkAsInterrupted
{
  self.isInterrupted = false;
}

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format
{
  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  NSLog(@"[AudioEngine] attaching new source node with ID: %@", sourceNodeId);

  [self.sourceNodes setValue:sourceNode forKey:sourceNodeId];
  [self.sourceFormats setValue:format forKey:sourceNodeId];
  [self.sourceStates setValue:@true forKey:sourceNodeId];

  [self.audioEngine attachNode:sourceNode];
  [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];

  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  NSLog(@"[AudioEngine] detaching source node with ID: %@", sourceNodeId);

  AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];

  if (sourceNode == nil) {
    NSLog(@"[AudioEngine] No source node found with ID: %@", sourceNodeId);
    return;
  }

  [self.audioEngine detachNode:sourceNode];

  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];
  [self.sourceStates removeObjectForKey:sourceNodeId];
}

- (void)attachInputNode:(AVAudioSinkNode *)inputNode
{
  self.inputNode = inputNode;

  [self.audioEngine attachNode:inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:inputNode format:nil];
}

- (void)detachInputNode
{
  if (self.inputNode == nil) {
    return;
  }

  [self.audioEngine detachNode:self.inputNode];
  self.inputNode = nil;
  [self restartAudioEngine];
}

- (bool)startIfNecessary
{
  if ([self isRunning]) {
    return true;
  }

  if ([self.sourceNodes count] > 0 || self.inputNode != nil) {
    return [self startEngine];
  }

  return false;
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

- (void)pauseIfNecessary
{
  if (![self isRunning]) {
    return;
  }

  for (NSString *sourceId in self.sourceStates) {
    if ([self.sourceStates[sourceId] boolValue]) {
      NSLog(@"state %@", self.sourceStates[sourceId]);
      return;
    }
  }

  NSLog(@"[AudioEngine] pauseEngine");
  [self.audioEngine pause];
}

- (void)logAudioEngineState
{
  AVAudioSession *session = [AVAudioSession sharedInstance];

  NSLog(@"================ 🎧 AVAudioEngine STATE ================");

  // AVAudioEngine state
  NSLog(@"➡️ engine.isRunning: %@", self.audioEngine.isRunning ? @"YES" : @"NO");
  NSLog(@"➡️ engine.isInManualRenderingMode: %@", self.audioEngine.isInManualRenderingMode ? @"YES" : @"NO");

  // Session state
  NSLog(@"🎚️ Session category: %@", session.category);
  NSLog(@"🎚️ Session mode: %@", session.mode);
  NSLog(@"🎚️ Session sampleRate: %f Hz", session.sampleRate);
  NSLog(@"🎚️ Session IO buffer duration: %f s", session.IOBufferDuration);

  // Current route
  AVAudioSessionRouteDescription *route = session.currentRoute;

  NSLog(@"🔊 Current audio route outputs:");
  for (AVAudioSessionPortDescription *output in route.outputs) {
    NSLog(@"  Output: %@ (%@)", output.portType, output.portName);
  }

  NSLog(@"🎤 Current audio route inputs:");
  for (AVAudioSessionPortDescription *input in route.inputs) {
    NSLog(@"  Input: %@ (%@)", input.portType, input.portName);
  }

  // Output node format
  AVAudioFormat *format = [self.audioEngine.outputNode inputFormatForBus:0];
  NSLog(@"📐 Engine output format: %.0f Hz, %u channels", format.sampleRate, format.channelCount);

  NSLog(@"=======================================================");
}

- (bool)isSupposedToRun
{
  return self.isSupposedToBeRunning;
}

@end
