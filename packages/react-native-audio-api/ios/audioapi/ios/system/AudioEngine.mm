#import <audioapi/ios/system/AudioAPIDiagnostics.h>
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
  std::recursive_mutex _engineLock;
  BOOL _isRebuildingAudioEngine;
  /// Tracks whether voice processing is currently engaged on the system input
  /// node of the live engine instance. Reset whenever the engine is recreated.
  BOOL _voiceProcessingApplied;
  /// Rebuild cadence, used only to report a restart storm. A rebuild is driven
  /// by a user pulling a cable or the OS reclaiming the session, so several per
  /// second means the library is answering its own notification.
  NSTimeInterval _lastRebuildUptime;
  NSInteger _rapidRebuildCount;
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
- (NSString *)lockedStateSnapshot;
- (void)traceRebuildCadence;

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
  std::scoped_lock lock(_engineLock);

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
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"could not set voice processing to %@: %@",
        wantsVoiceProcessing ? @"true" : @"false",
        AudioAPIDescribeError(error));
    return;
  }

  _voiceProcessingApplied = wantsVoiceProcessing;

  // Enabling voice processing rewrites the session to voiceChat and drops
  // AllowBluetoothA2DP, which posts a configuration change of its own. Every
  // rebuild re-toggles it, so this line repeating is the churn itself.
  AUDIOAPI_LOG(
      AudioAPIDiagnosticsCategoryEngine,
      @"voice processing set to %@, session is now {%@}",
      wantsVoiceProcessing ? @"true" : @"false",
      AudioAPIDescribeSession([AVAudioSession sharedInstance]));
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

  [self stopEngine];
  [self rebuildAudioEngine];

  if (!shouldResume) {
    self.state = AudioEngineState::AudioEngineStatePaused;
    [self notifyConfigurationChanges];
    return;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    NSLog(
        @"Error while restarting the audio engine after interruption: %@",
        [error debugDescription]);
    self.state = AudioEngineState::AudioEngineStateIdle;
    [self notifyConfigurationChanges];
    return;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
  [self notifyConfigurationChanges];
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

- (bool)isInUse
{
  std::scoped_lock lock(_engineLock);
  return [self hasTrackedGraph] || self.audioEngine != nil;
}

/// Reports the engine's own bookkeeping. Reads no AVAudioSession property, so it
/// is safe to call from anywhere on the restart path; the caller is expected to
/// hold `_engineLock`, which every path that reaches this does.
- (NSString *)lockedStateSnapshot
{
  return [NSString stringWithFormat:
                       @"state=%ld, engine=%@, running=%@, inputNode=%@, "
                       @"graphNeedsRebuild=%@, sessionInvalidated=%@, voiceProc=%@",
                       (long)self.state,
                       self.audioEngine == nil ? @"nil" : @"live",
                       [self.audioEngine isRunning] ? @"true" : @"false",
                       self.inputNode == nil ? @"nil" : @"live",
                       self.graphNeedsRebuild ? @"true" : @"false",
                       self.sessionDeactivationInvalidatedGraph ? @"true" : @"false",
                       _voiceProcessingApplied ? @"true" : @"false"];
}

- (void)traceRebuildCadence
{
  NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
  NSTimeInterval sinceLast = now - _lastRebuildUptime;

  if (_lastRebuildUptime > 0 && sinceLast < 1.0) {
    _rapidRebuildCount += 1;
  } else {
    _rapidRebuildCount = 0;
  }

  _lastRebuildUptime = now;

  if (_rapidRebuildCount >= 5) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"restart storm: %ld rebuilds under a second apart, last gap %.3fs - the restart path is "
        @"answering a notification it emitted itself: {%@}",
        (long)_rapidRebuildCount,
        sinceLast,
        [self lockedStateSnapshot]);
    return;
  }

  AUDIOAPI_LOG(
      AudioAPIDiagnosticsCategoryEngine,
      @"rebuild requested, %.3fs since the previous one: {%@}",
      _lastRebuildUptime > 0 ? sinceLast : 0.0,
      [self lockedStateSnapshot]);
}

- (void)rebuildAudioEngineAndResumeIfNeeded
{
  AUDIOAPI_TRACE_SCOPE(@"rebuild");

  [self traceRebuildCadence];

  if (_isRebuildingAudioEngine) {
    // The request is dropped, not deferred: the rebuild in flight finishes with
    // the configuration this one was meant to adopt, and nothing asks again.
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"dropping a nested rebuild request, one is already in flight: {%@}",
        [self lockedStateSnapshot]);
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

  // The silent death: startEngine reported the engine running and it stopped
  // itself again before this point, usually because voice processing changed the
  // IO format underneath it. The graph looks complete, so nothing downstream
  // knows a rebuild is still owed.
  if (self.state == AudioEngineState::AudioEngineStateRunning && ![self.audioEngine isRunning]) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"rebuild finished with the engine stopped while the state still says Running: {%@}",
        [self lockedStateSnapshot]);
  }
}

- (void)rebuildAudioEngine
{
  AUDIOAPI_TRACE_SCOPE(@"rebuildGraph");

  [self destroyAudioEnginePreservingSessionDeactivationState:YES];
  [self createAudioEngineIfNeeded];

  [self materializeTrackedNodesIfNeeded];

  // Clearing the flag while the registered input node is still missing is how a
  // refused restart becomes permanent: nothing downstream knows a rebuild is owed.
  if (self.inputRegistration != nil && self.inputNode == nil) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"clearing graphNeedsRebuild even though the registered input node did not "
        @"materialize: {%@}",
        [self lockedStateSnapshot]);
  }

  self.graphNeedsRebuild = false;
}

- (bool)startEngine
{
  AUDIOAPI_TRACE_SCOPE(@"startEngine");

  NSError *error = nil;

  if (self.audioEngine != nil && [self.audioEngine isRunning] &&
      self.state == AudioEngineState::AudioEngineStateRunning) {
    return true;
  }

  [self createAudioEngineIfNeeded];

  if (![self.sessionManager ensureActive:true error:&error]) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"giving up on start, the session would not activate: %@; {%@}",
        AudioAPIDescribeError(error),
        [self lockedStateSnapshot]);
    return false;
  }

  if (self.state == AudioEngineState::AudioEngineStateInterrupted || self.graphNeedsRebuild ||
      self.sessionDeactivationInvalidatedGraph) {
    [self rebuildAudioEngineAndResumeIfNeeded];
  } else {
    [self materializeTrackedNodesIfNeeded];
  }

  if (self.inputRegistration != nil && self.inputNode == nil) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"giving up on start, the input node has no live format: {%@}",
        [self lockedStateSnapshot]);
    return false;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    AUDIOAPI_LOG_FAILURE(
        AudioAPIDiagnosticsCategoryEngine,
        @"engine refused to start: %@; {%@}",
        AudioAPIDescribeError(error),
        [self lockedStateSnapshot]);
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;

  // Read back rather than trusted: an engine can report a successful start and
  // stop itself before the caller looks again, which is what a voice-processing
  // format change does. A `running=false` here is the loop's fingerprint.
  AUDIOAPI_LOG(
      AudioAPIDiagnosticsCategoryEngine,
      @"engine started, reading back: {%@}",
      [self lockedStateSnapshot]);

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
  AUDIOAPI_TRACE_SCOPE(@"restart");

  std::scoped_lock lock(_engineLock);

  // The engine is created lazily on first node attach. Apps that only use
  // session management and notifications never have one, and a system-driven
  // restart (media services reset, configuration change) must not create it.
  if (![self hasTrackedGraph] && self.audioEngine == nil) {
    AUDIOAPI_LOG(
        AudioAPIDiagnosticsCategoryEngine, @"restart ignored, this app has no engine of its own");
    return;
  }

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
