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

@end

@implementation AudioEngineInputRegistration
@end

@interface AudioEngine () {
  std::mutex _engineLock;

  /// Set when the engine should be running but the system refused to start it.
  BOOL _restartPending;

  /// Number of backoff retries already spent on the current pending restart.
  NSUInteger _restartRetryCount;

  /// Incremented whenever scheduled retries become obsolete, so that a retry already
  /// queued on the main queue can recognise itself as stale and do nothing.
  NSUInteger _restartRetryGeneration;
}

@property (nonatomic, strong)
    NSMutableDictionary<NSString *, AudioEngineSourceRegistration *> *sourceRegistrations;
@property (nonatomic, strong) AudioEngineInputRegistration *inputRegistration;

// Every method below assumes the caller already holds `_engineLock`; the public methods
// declared in the header acquire it. `_engineLock` is not recursive, so these must never
// be reached through a public method.
- (void)createAudioEngineIfNeeded;
- (void)destroyAudioEnginePreservingSessionDeactivationState:(BOOL)preserveSessionDeactivationState;
- (BOOL)hasTrackedGraph;
- (AVAudioFormat *)currentInputConnectionFormat;
- (void)materializeSourceNodeWithId:(NSString *)sourceNodeId;
- (BOOL)materializeInputNodeIfNeeded;
- (void)materializeTrackedNodesIfNeeded;

- (AVAudioFormat *)liveInputFormat;
- (void)resetInputNode;
- (void)rebuildAudioEngineAndResumeIfNeeded;
- (BOOL)graphRequiresRebuild;
- (NSString *)describeInputNodeAvailability;
- (void)handleRefusedRestart;
- (void)markRestartPending;
- (void)clearPendingRestart;
- (void)scheduleRestartRetry;

/// Runs a scheduled retry. Unlike the helpers above this one acquires `_engineLock`
/// itself, because it is invoked from the main queue by `scheduleRestartRetry`.
- (void)retryPendingRestartForGeneration:(NSUInteger)generation;

@end

/// Backoff for restarts the system refused, doubling from the initial delay up to the
/// maximum one. The schedule is bounded rather than endless because the conditions that
/// cause a refusal (a locked device, another application holding the session) routinely
/// outlast any reasonable polling window; once it is exhausted the engine stays pending
/// and waits for `retryPendingRestartIfNeeded` to signal that conditions have changed.
static const NSTimeInterval kInitialRestartRetryDelay = 2.0;
static const NSTimeInterval kMaximumRestartRetryDelay = 32.0;
static const NSUInteger kMaximumRestartRetryCount = 6;

static NSString *AudioEngineStateName(AudioEngineState state)
{
  switch (state) {
    case AudioEngineState::AudioEngineStateIdle:
      return @"idle";
    case AudioEngineState::AudioEngineStateRunning:
      return @"running";
    case AudioEngineState::AudioEngineStatePaused:
      return @"paused";
    case AudioEngineState::AudioEngineStateInterrupted:
      return @"interrupted";
  }

  return @"unknown";
}

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
  [self clearPendingRestart];
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

- (void)materializeTrackedNodesIfNeeded
{
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
{
  std::scoped_lock lock(_engineLock);
  [self createAudioEngineIfNeeded];

  if (self.inputRegistration != nil || self.inputNode != nil) {
    [self resetInputNode];
  }

  AudioEngineInputRegistration *registration = [[AudioEngineInputRegistration alloc] init];
  registration.receiverBlock = receiverBlock;
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

- (AVAudioFormat *)getLiveInputFormat
{
  std::scoped_lock lock(_engineLock);
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

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    return;
  }

  [self stopEngine];
  [self rebuildAudioEngine];
  // The graph is fresh, so the deactivation that invalidated the previous one is settled.
  // Leaving the flag raised would make the start below rebuild a second time.
  self.sessionDeactivationInvalidatedGraph = false;

  if (!shouldResume) {
    self.state = AudioEngineState::AudioEngineStatePaused;
    return;
  }

  if (![self startEngine]) {
    NSLog(@"[AudioEngine] Audio engine restart after interruption was refused.");
    [self handleRefusedRestart];
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
  // A restart already pending means the engine is meant to be running even though its
  // state currently says otherwise, so this rebuild must resume it as well.
  BOOL shouldResume = self.state == AudioEngineState::AudioEngineStateRunning || _restartPending;

  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  [self rebuildAudioEngine];
  self.sessionDeactivationInvalidatedGraph = false;

  if (!shouldResume) {
    NSLog(
        @"[AudioEngine] Graph rebuilt without resuming, engine is %@ (input node %@).",
        AudioEngineStateName(self.state),
        [self describeInputNodeAvailability]);

    // An interruption suspends an engine that is still meant to be running, and the system
    // does not always follow one with an end notification. Arming the retry ladder here
    // means the next opportunity resumes the engine, rather than leaving it stopped until
    // something on the JavaScript side happens to start it again. Only an interruption
    // qualifies: an idle or paused engine was stopped deliberately.
    if (self.state == AudioEngineState::AudioEngineStateInterrupted) {
      [self markRestartPending];
    }

    return;
  }

  if (![self startEngine]) {
    [self handleRefusedRestart];
    return;
  }

  NSLog(@"[AudioEngine] Graph rebuilt and engine restarted.");
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

  if ([self graphRequiresRebuild]) {
    if ([self.audioEngine isRunning]) {
      [self.audioEngine stop];
    }

    [self rebuildAudioEngine];
    self.sessionDeactivationInvalidatedGraph = false;
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

  if (_restartPending) {
    NSLog(
        @"[AudioEngine] Audio engine restart succeeded after %lu scheduled retry(-ies).",
        (unsigned long)_restartRetryCount);
  }

  [self clearPendingRestart];
  return true;
}

- (BOOL)graphRequiresRebuild
{
  return self.state == AudioEngineState::AudioEngineStateInterrupted || self.graphNeedsRebuild ||
      self.sessionDeactivationInvalidatedGraph;
}

/// Reports whether a registered input node is currently materialized. This distinguishes a
/// healthy rebuild from one that silently dropped the microphone because the route reported
/// no usable format, which is the state a rebuild leaves behind mid route change.
- (NSString *)describeInputNodeAvailability
{
  if (self.inputRegistration == nil) {
    return @"not requested";
  }

  return self.inputNode != nil ? @"materialized" : @"missing live input format";
}

/// Records that the system refused to start the engine and queues another attempt.
///
/// The engine is never left reporting the running state: `getState` and `isEngineRunning`
/// feed player and recorder status, so a running engine the system never started would hide
/// the failure from every consumer. It is left paused when something is still tracked and
/// meant to be running, or idle when nothing is.
- (void)handleRefusedRestart
{
  self.state = [self hasTrackedGraph] ? AudioEngineState::AudioEngineStatePaused
                                      : AudioEngineState::AudioEngineStateIdle;
  [self markRestartPending];
}

/// Records that the engine should be running even though it is not, and queues an attempt.
///
/// The graph is marked for rebuild because a graph assembled while the route was unsettled
/// can be missing its input node. An engine with nothing attached has nothing to restart,
/// so the pending state is dropped rather than retried forever.
- (void)markRestartPending
{
  if (![self hasTrackedGraph]) {
    [self clearPendingRestart];
    return;
  }

  _restartPending = YES;
  self.graphNeedsRebuild = YES;

  [self scheduleRestartRetry];
}

- (void)clearPendingRestart
{
  _restartPending = NO;
  _restartRetryCount = 0;
  _restartRetryGeneration += 1;
}

- (void)scheduleRestartRetry
{
  if (_restartRetryCount >= kMaximumRestartRetryCount) {
    NSLog(
        @"[AudioEngine] Audio engine restart is still refused after %lu attempts. Waiting for the "
        @"application to return to the foreground or for the audio route to change.",
        (unsigned long)_restartRetryCount);
    return;
  }

  NSTimeInterval delay =
      MIN(kInitialRestartRetryDelay * static_cast<NSTimeInterval>(1u << _restartRetryCount),
          kMaximumRestartRetryDelay);
  _restartRetryCount += 1;

  NSLog(
      @"[AudioEngine] Audio engine restart is pending, retrying in %.0f s (attempt %lu of %lu).",
      delay,
      (unsigned long)_restartRetryCount,
      (unsigned long)kMaximumRestartRetryCount);

  // Bumping the generation invalidates any retry queued earlier, so that an immediate
  // attempt through `retryPendingRestartIfNeeded` cannot leave two retries racing.
  NSUInteger generation = ++_restartRetryGeneration;
  __weak AudioEngine *weakSelf = self;

  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delay * NSEC_PER_SEC)),
      dispatch_get_main_queue(),
      ^{ [weakSelf retryPendingRestartForGeneration:generation]; });
}

- (void)retryPendingRestartForGeneration:(NSUInteger)generation
{
  std::scoped_lock lock(_engineLock);

  if (!_restartPending || generation != _restartRetryGeneration) {
    return;
  }

  if (![self hasTrackedGraph]) {
    [self clearPendingRestart];
    return;
  }

  if (![self startEngine]) {
    [self scheduleRestartRetry];
  }
}

- (void)retryPendingRestartIfNeeded
{
  std::scoped_lock lock(_engineLock);

  if (!_restartPending) {
    return;
  }

  if (![self hasTrackedGraph]) {
    [self clearPendingRestart];
    return;
  }

  // An external trigger is evidence that conditions changed, so the backoff starts over
  // and a pending restart that already exhausted its schedule becomes retryable again.
  _restartRetryCount = 0;

  if (![self startEngine]) {
    [self handleRefusedRestart];
  }
}

- (bool)isRestartPending
{
  std::scoped_lock lock(_engineLock);
  return _restartPending;
}

- (void)stopEngine
{
  // Stopping expresses that the engine is no longer meant to run, which retires any
  // restart the system had refused.
  [self clearPendingRestart];

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
  [self clearPendingRestart];

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
