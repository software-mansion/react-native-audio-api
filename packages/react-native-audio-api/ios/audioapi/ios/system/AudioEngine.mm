#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

#include <mutex>

@interface AudioEngineSourceRegistration : NSObject

@property (nonatomic, copy) AVAudioSourceNodeRenderBlock renderBlock;
@property (nonatomic, assign) float sampleRate;
@property (nonatomic, assign) AVAudioChannelCount channelCount;

@end

@implementation AudioEngineSourceRegistration
@end

@interface AudioEngineInputRegistration : NSObject

@property (nonatomic, copy) AVAudioSinkNodeReceiverBlock receiverBlock;
@property (nonatomic, assign) BOOL voiceProcessingEnabled;
@property (nonatomic, copy) void (^onInputConfigurationChange)(void);

@end

@implementation AudioEngineInputRegistration
@end

@interface AudioEngine () {
  std::mutex _engineLock;
  BOOL _isRebuildingAudioEngine;
  /// Tracks whether voice processing is currently engaged on the system input
  /// node of the live engine instance. Reset whenever the engine is recreated.
  BOOL _voiceProcessingApplied;
}

@property (nonatomic, strong)
    NSMutableDictionary<NSString *, AudioEngineSourceRegistration *> *sourceRegistrations;
@property (nonatomic, strong) AudioEngineInputRegistration *inputRegistration;

- (void)createAudioEngineIfNeeded;
- (void)destroyAudioEnginePreservingSessionDeactivationState:(BOOL)preserveSessionDeactivationState;
- (BOOL)hasTrackedGraph;
- (AVAudioFormat *)currentInputConnectionFormat;
- (void)materializeSourceNodeWithId:(NSString *)sourceNodeId;
- (BOOL)materializeInputNodeIfNeeded;
- (void)applyVoiceProcessing;
- (void)materializeTrackedNodesIfNeeded;

- (AVAudioFormat *)liveInputFormat;
- (void)resetInputNode;
- (void)rebuildAudioEngineAndResumeIfNeeded;
- (void)notifyConfigurationChanges;

@end

@implementation AudioEngine

static AudioEngine *_sharedInstance = nil;

- (void)createAudioEngineIfNeeded
{
  if (self.audioEngine != nil) {
    return;
  }

  self.audioEngine = [[AVAudioEngine alloc] init];
}

- (void)destroyAudioEngine
{
  [self destroyAudioEnginePreservingSessionDeactivationState:NO];
}

- (BOOL)hasTrackedGraph
{
  return [self.sourceRegistrations count] > 0 || self.inputRegistration != nil;
}

- (void)destroyAudioEnginePreservingSessionDeactivationState:(BOOL)preserveSessionDeactivationState
{
  BOOL hadGraph = [self hasTrackedGraph];

  if (self.audioEngine != nil) {
    if ([self.audioEngine isRunning]) {
      [self.audioEngine stop];
    }

    [self.audioEngine reset];
  }

  self.audioEngine = nil;
  self.sourceNodes = [[NSMutableDictionary alloc] init];
  self.sourceFormats = [[NSMutableDictionary alloc] init];
  self.inputNode = nil;
  self.graphNeedsRebuild = hadGraph;
  _voiceProcessingApplied = NO;

  if (!preserveSessionDeactivationState) {
    self.sessionDeactivationInvalidatedGraph = false;
  }
}

+ (instancetype)sharedInstance
{
  return _sharedInstance;
}

- (instancetype)init
{
  if (self = [super init]) {
    self.state = AudioEngineState::AudioEngineStateIdle;
    self.audioEngine = nil;
    self.inputNode = nil;
    self.graphNeedsRebuild = false;
    self.sessionDeactivationInvalidatedGraph = false;

    self.sourceRegistrations = [[NSMutableDictionary alloc] init];
    self.sourceNodes = [[NSMutableDictionary alloc] init];
    self.sourceFormats = [[NSMutableDictionary alloc] init];
    self.inputRegistration = nil;

    self.sessionManager = [AudioSessionManager sharedInstance];
    [self createAudioEngineIfNeeded];
  }

  _sharedInstance = self;
  return self;
}

- (void)cleanup
{
  std::scoped_lock lock(_engineLock);
  [self destroyAudioEngine];
  self.state = AudioEngineState::AudioEngineStateIdle;
  self.sourceRegistrations = nil;
  self.sourceNodes = nil;
  self.sourceFormats = nil;
  self.inputRegistration = nil;
  self.inputNode = nil;
  self.graphNeedsRebuild = false;
  self.sessionDeactivationInvalidatedGraph = false;

  [self.sessionManager setActive:false error:nil];
  self.sessionManager = nil;

  if (_sharedInstance == self) {
    _sharedInstance = nil;
  }
}

- (void)materializeSourceNodeWithId:(NSString *)sourceNodeId
{
  AudioEngineSourceRegistration *registration = self.sourceRegistrations[sourceNodeId];

  if (registration == nil || self.audioEngine == nil || self.sourceNodes[sourceNodeId] != nil) {
    return;
  }

  AVAudioFormat *format =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:registration.sampleRate
                                                     channels:registration.channelCount];
  AVAudioSourceNode *sourceNode =
      [[AVAudioSourceNode alloc] initWithFormat:format renderBlock:registration.renderBlock];

  self.sourceNodes[sourceNodeId] = sourceNode;
  self.sourceFormats[sourceNodeId] = format;

  [self.audioEngine attachNode:sourceNode];
  [self.audioEngine connect:sourceNode to:self.audioEngine.mainMixerNode format:format];
}

- (AVAudioFormat *)liveInputFormat
{
  if (self.audioEngine == nil) {
    return nil;
  }

  AVAudioInputNode *engineInputNode = self.audioEngine.inputNode;

  if (engineInputNode == nil) {
    return nil;
  }

  AVAudioFormat *inputFormat = [engineInputNode outputFormatForBus:0];

  if (inputFormat == nil || inputFormat.sampleRate <= 0 || inputFormat.channelCount == 0) {
    return nil;
  }

  return inputFormat;
}

- (AVAudioFormat *)currentInputConnectionFormat
{
  AVAudioFormat *inputFormat = [self liveInputFormat];

  if (inputFormat == nil || inputFormat.sampleRate <= 0 || inputFormat.channelCount == 0) {
    return nil;
  }

  return inputFormat;
}

- (BOOL)materializeInputNodeIfNeeded
{
  if (self.inputRegistration == nil) {
    return YES;
  }

  if (self.audioEngine == nil) {
    return NO;
  }

  if (self.inputNode != nil) {
    return YES;
  }

  [self applyVoiceProcessing];
  NSError *sessionError = nil;
  if (![self.sessionManager ensureActive:true error:&sessionError]) {
    NSLog(
        @"Error while activating audio session before input materialization: %@",
        [sessionError debugDescription]);
    return NO;
  }

  AVAudioFormat *inputFormat = [self currentInputConnectionFormat];

  if (inputFormat == nil) {
    return NO;
  }

  self.inputNode =
      [[AVAudioSinkNode alloc] initWithReceiverBlock:self.inputRegistration.receiverBlock];
  [self.audioEngine attachNode:self.inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:self.inputNode format:inputFormat];
  return YES;
}

// Apple's voice-processing I/O (echo cancellation, noise suppression, AGC) is
// opt-in per recorder. Without it, full-duplex apps (VoIP, voice agents) hear
// their own speaker output looped back into the microphone. Toggling it is only
// allowed while the engine is stopped, and it changes the hardware input format,
// so this has to run before the input connection format is read.
- (void)applyVoiceProcessing
{
  BOOL wantsVoiceProcessing = self.inputRegistration.voiceProcessingEnabled;

  // A freshly created engine has voice processing off, so for playback-only
  // graphs there is nothing to undo - and reading `inputNode` would needlessly
  // pull the microphone into the engine.
  if (!wantsVoiceProcessing && !_voiceProcessingApplied) {
    return;
  }

  if (self.audioEngine == nil) {
    return;
  }

  AVAudioInputNode *systemInputNode = self.audioEngine.inputNode;

  if (systemInputNode.isVoiceProcessingEnabled == wantsVoiceProcessing) {
    _voiceProcessingApplied = wantsVoiceProcessing;
    return;
  }

  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  NSError *error = nil;

  if (![systemInputNode setVoiceProcessingEnabled:wantsVoiceProcessing error:&error]) {
    NSLog(
        @"[AudioEngine] Error while setting voice processing to %@: %@",
        wantsVoiceProcessing ? @"true" : @"false",
        [error debugDescription]);
    return;
  }

  _voiceProcessingApplied = wantsVoiceProcessing;
}

- (void)materializeTrackedNodesIfNeeded
{
  [self applyVoiceProcessing];

  NSArray<NSString *> *sourceNodeIds =
      [[self.sourceRegistrations allKeys] sortedArrayUsingSelector:@selector(compare:)];
  for (NSString *sourceNodeId in sourceNodeIds) {
    [self materializeSourceNodeWithId:sourceNodeId];
  }

  [self materializeInputNodeIfNeeded];
}

- (NSString *)attachSourceNodeWithRenderBlock:(AVAudioSourceNodeRenderBlock)renderBlock
                                   sampleRate:(float)sampleRate
                                 channelCount:(AVAudioChannelCount)channelCount
{
  std::scoped_lock lock(_engineLock);
  [self createAudioEngineIfNeeded];

  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  AudioEngineSourceRegistration *registration = [[AudioEngineSourceRegistration alloc] init];
  registration.renderBlock = renderBlock;
  registration.sampleRate = sampleRate;
  registration.channelCount = channelCount;

  self.sourceRegistrations[sourceNodeId] = registration;
  [self materializeSourceNodeWithId:sourceNodeId];

  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  std::scoped_lock lock(_engineLock);
  AVAudioSourceNode *sourceNode = self.sourceNodes[sourceNodeId];

  if (self.sourceRegistrations[sourceNodeId] == nil) {
    NSLog(@"[AudioEngine] No source node found with ID: %@", sourceNodeId);
    return;
  }

  if (sourceNode != nil && self.audioEngine != nil) {
    [self.audioEngine detachNode:sourceNode];
  }

  [self.sourceRegistrations removeObjectForKey:sourceNodeId];
  [self.sourceNodes removeObjectForKey:sourceNodeId];
  [self.sourceFormats removeObjectForKey:sourceNodeId];

  if (![self hasTrackedGraph]) {
    self.graphNeedsRebuild = false;
  }
}

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock
                  voiceProcessingEnabled:(BOOL)voiceProcessingEnabled
              onInputConfigurationChange:(void (^)(void))onInputConfigurationChange
{
  std::scoped_lock lock(_engineLock);
  [self createAudioEngineIfNeeded];

  if (self.inputRegistration != nil || self.inputNode != nil) {
    [self resetInputNode];
  }

  AudioEngineInputRegistration *registration = [[AudioEngineInputRegistration alloc] init];
  registration.receiverBlock = receiverBlock;
  registration.voiceProcessingEnabled = voiceProcessingEnabled;
  registration.onInputConfigurationChange = onInputConfigurationChange;
  self.inputRegistration = registration;

  [self materializeInputNodeIfNeeded];
}

- (void)resetInputNode
{
  if (self.inputRegistration == nil && self.inputNode == nil) {
    return;
  }

  if (self.inputNode != nil && self.audioEngine != nil) {
    [self.audioEngine detachNode:self.inputNode];
  }

  self.inputRegistration = nil;
  self.inputNode = nil;

  if (![self hasTrackedGraph]) {
    self.graphNeedsRebuild = false;
  }
}

- (void)detachInputNode
{
  std::scoped_lock lock(_engineLock);
  [self resetInputNode];
}

- (AVAudioFormat *)getLiveInputFormat
{
  return [self liveInputFormat];
}

- (void)onInterruptionBegin
{
  std::scoped_lock lock(_engineLock);
  if (self.state != AudioEngineState::AudioEngineStateRunning) {
    return;
  }

  self.state = AudioEngineState::AudioEngineStateInterrupted;
}

- (void)onSessionDeactivated
{
  std::scoped_lock lock(_engineLock);
  BOOL hadTrackedGraph = [self hasTrackedGraph];
  BOOL hadActiveState = self.state != AudioEngineState::AudioEngineStateIdle;

  if (!hadActiveState && !hadTrackedGraph) {
    return;
  }

  self.sessionDeactivationInvalidatedGraph = true;

  if (hadTrackedGraph) {
    self.graphNeedsRebuild = true;
  }

  if (self.audioEngine != nil && ![self.audioEngine isRunning]) {
    self.state = AudioEngineState::AudioEngineStatePaused;
    return;
  }

  if (self.audioEngine != nil) {
    [self.audioEngine pause];
  }

  self.state = AudioEngineState::AudioEngineStatePaused;
}

- (void)markSessionDeactivationInvalidatedGraph
{
  std::scoped_lock lock(_engineLock);
  self.sessionDeactivationInvalidatedGraph = YES;
}

- (void)onInterruptionEnd:(bool)shouldResume
{
  std::scoped_lock lock(_engineLock);
  NSError *error = nil;

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    return;
  }

  if (!shouldResume && self.inputRegistration == nil) {
    [self stopEngine];
    [self rebuildAudioEngine];
    self.state = AudioEngineState::AudioEngineStatePaused;
    return;
  }

  // BEFORE any engine call: the format read inside rebuildAudioEngine needs an active session, and a
  // refusal here must return with Interrupted still intact so a later trigger can retry.
  if (![self.sessionManager ensureActive:true error:&error]) {
    NSLog(@"Error while activating audio session after interruption: %@", [error debugDescription]);
    return;
  }

  [self stopEngine];
  [self rebuildAudioEngine];

  if (self.inputRegistration != nil && self.inputNode == nil) {
    NSLog(
        @"Error while materializing the audio input node after interruption: missing live input format");
    self.state = AudioEngineState::AudioEngineStateInterrupted;
    return;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(
        @"Error while restarting the audio engine after interruption: %@",
        [error debugDescription]);
    self.state = AudioEngineState::AudioEngineStateInterrupted;
    // not Idle: stopEngine already set that return;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
}

- (void)notifyConfigurationChanges
{
  if (self.inputRegistration != nil && self.inputRegistration.onInputConfigurationChange != nil) {
    self.inputRegistration.onInputConfigurationChange();
  }
}

- (AudioEngineState)getState
{
  std::scoped_lock lock(_engineLock);
  return self.state;
}

- (bool)isEngineRunning
{
  std::scoped_lock lock(_engineLock);
  return self.audioEngine != nil && [self.audioEngine isRunning];
}

- (void)rebuildAudioEngineAndResumeIfNeeded
{
  if (_isRebuildingAudioEngine) {
    return;
  }

  _isRebuildingAudioEngine = YES;

  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  [self rebuildAudioEngine];
  self.sessionDeactivationInvalidatedGraph = false;

  if (self.state == AudioEngineState::AudioEngineStateRunning) {
    [self startEngine];
  }

  [self notifyConfigurationChanges];

  _isRebuildingAudioEngine = NO;
}

- (void)rebuildAudioEngine
{
  [self destroyAudioEnginePreservingSessionDeactivationState:YES];
  [self createAudioEngineIfNeeded];

  [self materializeTrackedNodesIfNeeded];
  self.graphNeedsRebuild = false;
}

- (bool)startEngine
{
  NSError *error = nil;

  if (self.audioEngine != nil && [self.audioEngine isRunning] &&
      self.state == AudioEngineState::AudioEngineStateRunning) {
    return true;
  }

  [self createAudioEngineIfNeeded];

  if (![self.sessionManager ensureActive:true error:&error]) {
    NSLog(@"Error while activating audio session: %@", [error debugDescription]);
    return false;
  }

  if (self.state == AudioEngineState::AudioEngineStateInterrupted || self.graphNeedsRebuild ||
      self.sessionDeactivationInvalidatedGraph) {
    [self rebuildAudioEngineAndResumeIfNeeded];
  } else {
    [self materializeTrackedNodesIfNeeded];
  }

  if (self.inputRegistration != nil && self.inputNode == nil) {
    NSLog(@"Error while materializing the audio input node: missing live input format");
    return false;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(@"Error while starting the audio engine: %@", [error debugDescription]);
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
  return true;
}

- (void)stopEngine
{
  if (self.state == AudioEngineState::AudioEngineStateIdle) {
    return;
  }

  if (self.audioEngine != nil && [self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  self.state = AudioEngineState::AudioEngineStateIdle;
}

- (bool)startIfNecessary
{
  std::scoped_lock lock(_engineLock);
  if (self.state == AudioEngineState::AudioEngineStateRunning && self.audioEngine != nil &&
      [self.audioEngine isRunning]) {
    [self materializeTrackedNodesIfNeeded];
    return true;
  }

  if ([self hasTrackedGraph]) {
    return [self startEngine];
  }

  return false;
}

- (void)pauseIfNecessary
{
  std::scoped_lock lock(_engineLock);
  if (self.state == AudioEngineState::AudioEngineStatePaused) {
    return;
  }

  if (self.audioEngine != nil) {
    [self.audioEngine pause];
  }

  self.state = AudioEngineState::AudioEngineStatePaused;
}

- (void)stopIfNecessary
{
  std::scoped_lock lock(_engineLock);
  [self stopEngine];
}

- (void)stopIfPossible
{
  std::scoped_lock lock(_engineLock);
  BOOL hasInput = self.inputRegistration != nil;
  BOOL hasSources = [self.sourceRegistrations count] > 0;

  if (hasInput || hasSources) {
    return;
  }

  if (self.state != AudioEngineState::AudioEngineStateIdle) {
    [self stopEngine];
  }
}

- (void)restartAudioEngine
{
  std::scoped_lock lock(_engineLock);
  [self rebuildAudioEngineAndResumeIfNeeded];
}

- (void)logAudioEngineState
{
  std::scoped_lock lock(_engineLock);
  AVAudioSession *session = [AVAudioSession sharedInstance];

  NSLog(@"================ 🎧 AVAudioEngine STATE ================");

  NSLog(@"➡️ engine.isRunning: %@", self.audioEngine.isRunning ? @"true" : @"false");
  NSLog(
      @"➡️ engine.isInManualRenderingMode: %@",
      self.audioEngine.isInManualRenderingMode ? @"true" : @"false");

  NSLog(@"🎚️ Session category: %@", session.category);
  NSLog(@"🎚️ Session mode: %@", session.mode);
  NSLog(@"🎚️ Session sampleRate: %f Hz", session.sampleRate);
  NSLog(@"🎚️ Session IO buffer duration: %f s", session.IOBufferDuration);

  AVAudioSessionRouteDescription *route = session.currentRoute;

  NSLog(@"🔊 Current audio route outputs:");
  for (AVAudioSessionPortDescription *output in route.outputs) {
    NSLog(@"  Output: %@ (%@)", output.portType, output.portName);
  }

  NSLog(@"🎤 Current audio route inputs:");
  for (AVAudioSessionPortDescription *input in route.inputs) {
    NSLog(@"  Input: %@ (%@)", input.portType, input.portName);
  }

  AVAudioFormat *format = [self.audioEngine.outputNode inputFormatForBus:0];
  NSLog(@"📐 Engine output format: %.0f Hz, %u channels", format.sampleRate, format.channelCount);

  NSLog(@"=======================================================");
}

@end
