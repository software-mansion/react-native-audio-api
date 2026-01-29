#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

@implementation AudioEngine

static AudioEngine *_sharedInstance = nil;

+ (instancetype)sharedInstance
{
  return _sharedInstance;
}

- (instancetype)init
{
  if (self = [super init]) {
    self.state = AudioEngineState::AudioEngineStateIdle;
    self.audioEngine = [[AVAudioEngine alloc] init];
    self.inputNode = nil;

    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];

    self.sessionManager = [AudioSessionManager sharedInstance];
  }

  _sharedInstance = self;
  return self;
}

- (void)cleanup
{
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.audioEngine = nil;
  self.sourceNodes = nil;
  self.sourceFormats = nil;
  self.inputNode = nil;

  [self.sessionManager setActive:false error:nil];
  self.sessionManager = nil;
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

  if (sourceNode == nil) {
    NSLog(@"[AudioEngine] No source node found with ID: %@", sourceNodeId);
    return;
  }

  [self.audioEngine detachNode:sourceNode];

  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];
}

- (void)attachInputNode:(AVAudioSinkNode *)inputNode
{
  self.inputNode = inputNode;
  AVAudioFormat *format = [self.audioEngine.inputNode inputFormatForBus:0];

  [self.audioEngine attachNode:inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:inputNode format:format];
}

- (void)detachInputNode
{
  if (self.inputNode == nil) {
    return;
  }

  [self.audioEngine detachNode:self.inputNode];
  self.inputNode = nil;
}

- (void)onInterruptionBegin
{
  if (self.state != AudioEngineState::AudioEngineStateRunning) {
    // If engine was not active, do nothing
    return;
  }

  // If engine was active or paused (or interrupted :)) mark as interrupted
  self.state = AudioEngineState::AudioEngineStateInterrupted;
}

- (void)onInterruptionEnd:(bool)shouldResume
{
  NSError *error = nil;

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    // If engine was not interrupted, do nothing
    // Not a real condition, but better be safe than sorry :shrug:
    return;
  }

  // Stop just in case, reset the engine and build it from scratch
  [self stopIfNecessary];
  [self.audioEngine reset];
  [self rebuildAudioEngine];

  // If shouldResume is false, mark the engine as paused and wait
  // for JS-side resume command
  // TODO: this should be notified to the user f.e. via Event Emitter
  if (!shouldResume) {
    self.state = AudioEngineState::AudioEngineStatePaused;
    return;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(
        @"Error while restarting the audio engine after interruption: %@",
        [error debugDescription]);
    self.state = AudioEngineState::AudioEngineStateIdle;
    return;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
}

- (AudioEngineState)getState
{
  return self.state;
}

/// @brief Rebuilds the audio engine by re-attaching and re-connecting all source nodes and input node.
- (void)rebuildAudioEngine
{
  if (self.audioEngine != nil) {
    for (id sourceNodeId in self.sourceNodes) {
      AVAudioSourceNode *sourceNode = [self.sourceNodes valueForKey:sourceNodeId];

      [self.audioEngine detachNode:sourceNode];
    }

    if (self.inputNode) {
      [self.audioEngine detachNode:self.inputNode];
    }
  }

  self.audioEngine = [[AVAudioEngine alloc] init];

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

// @brief Starts the audio engine if not already running.
- (bool)startEngine
{
  NSError *error = nil;

  if ([self.audioEngine isRunning] && self.state == AudioEngineState::AudioEngineStateRunning) {
    return true;
  }

  if (![self.sessionManager setActive:true error:&error]) {
    // TODO: return user facing error
    NSLog(@"Error while activating audio session: %@", [error debugDescription]);
    return false;
  }

  if (self.state == AudioEngineState::AudioEngineStateInterrupted) {
    [self.audioEngine stop];
    [self.audioEngine reset];
    [self rebuildAudioEngine];
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"Error while starting the audio engine: %@", [error debugDescription]);
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  return true;
}

- (void)stopEngine
{
  if (self.state == AudioEngineState::AudioEngineStateIdle) {
    return;
  }

  [self.audioEngine stop];
  self.state = AudioEngineState::AudioEngineStateIdle;
}

- (bool)startIfNecessary
{
  if (self.state == AudioEngineState::AudioEngineStateRunning) {
    return true;
  }

  if (([self.sourceNodes count] > 0) || self.inputNode != nil) {
    return [self startEngine];
  }

  return false;
}

- (void)pauseIfNecessary
{
  if (self.state == AudioEngineState::AudioEngineStatePaused) {
    return;
  }

  [self.audioEngine pause];
  self.state = AudioEngineState::AudioEngineStatePaused;
}

- (void)stopIfNecessary
{
  if (self.state == AudioEngineState::AudioEngineStateIdle) {
    return;
  }

  [self stopEngine];
}

- (void)stopIfPossible
{
  if (self.state == AudioEngineState::AudioEngineStateIdle) {
    return;
  }

  bool hasInput = self.inputNode != nil;
  bool hasSources = [self.sourceNodes count] > 0;

  if (hasInput || hasSources) {
    return;
  }

  [self stopEngine];
}

- (void)restartAudioEngine
{
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  [self rebuildAudioEngine];
  if (self.state == AudioEngineState::AudioEngineStateRunning) {
    [self startEngine];
  }
}

- (void)logAudioEngineState
{
  AVAudioSession *session = [AVAudioSession sharedInstance];

  NSLog(@"================ 🎧 AVAudioEngine STATE ================");

  // AVAudioEngine state
  NSLog(@"➡️ engine.isRunning: %@", self.audioEngine.isRunning ? @"true" : @"false");
  NSLog(
      @"➡️ engine.isInManualRenderingMode: %@",
      self.audioEngine.isInManualRenderingMode ? @"true" : @"false");

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

@end
