#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>
#define LOG_MODULE AudioEngine
#import <audioapi/ios/utils/Log.h>

#include <mutex>

@interface AudioEngineSourceRegistration : NSObject

@property (nonatomic, copy) AVAudioSourceNodeRenderBlock renderBlock;
@property (nonatomic, assign) float sampleRate;
@property (nonatomic, assign) AVAudioChannelCount channelCount;
@property (nonatomic, copy) OnOutputRecoveryFailedBlock onOutputRecoveryFailed;

@end

@implementation AudioEngineSourceRegistration
@end

@interface AudioEngineInputRegistration : NSObject

@property (nonatomic, copy) AVAudioSinkNodeReceiverBlock receiverBlock;
@property (nonatomic, assign) BOOL voiceProcessingEnabled;
@property (nonatomic, copy) void (^onInputNotification)(AudioEngineInputNotification);

@end

@implementation AudioEngineInputRegistration
@end

@interface AudioEngine () {
  /// Guards the engine and its tracked graph. Never taken on a thread AVFoundation owns:
  /// dealloc, stop, reset, prepare and start can wait on the calling thread.
  std::mutex _engineLock;
  /// Tracks whether voice processing is currently engaged on the system input
  /// node of the live engine instance. Reset whenever the engine is recreated.
  BOOL _voiceProcessingApplied;
}

@property (nonatomic, strong)
    NSMutableDictionary<NSString *, AudioEngineSourceRegistration *> *sourceRegistrations;
@property (nonatomic, strong) AudioEngineInputRegistration *inputRegistration;

- (void)createAudioEngineIfNeeded;
- (void)destroyAudioEnginePreservingSessionDeactivationState:(BOOL)preserveSessionDeactivationState;
- (BOOL)hasInput;
- (BOOL)hasSources;
- (BOOL)hasTrackedGraph;
- (AVAudioFormat *)currentInputConnectionFormat;
- (void)materializeSourceNodeWithId:(NSString *)sourceNodeId;
- (BOOL)materializeInputNodeIfNeeded;
- (void)connectVoiceProcessingIOOutput:(AVAudioFormat *)format;
- (void)applyVoiceProcessing;
- (void)materializeTrackedNodesIfNeeded;

- (AVAudioFormat *)liveInputFormat;
- (void)resetInputNode;
- (BOOL)mustRebuildBeforeStart;
- (void)rebuildGraphForCurrentHardware;
- (void)settleStateAfterFailedStart;
- (void)resumeAfterRebuild;
- (dispatch_block_t)inputNotificationBlock:(AudioEngineInputNotification)notification;
- (AudioEngineInterruptionEndOutcome)rebuildAfterInterruption:(bool)shouldResume
                                                 readyToStart:(BOOL *)readyToStart;
- (AudioEngineInterruptionEndOutcome)startAfterInterruption;

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

- (BOOL)hasInput
{
  return self.inputRegistration != nil;
}

- (BOOL)hasSources
{
  return [self.sourceRegistrations count] > 0;
}

- (BOOL)hasTrackedGraph
{
  return [self hasInput] || [self hasSources];
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

- (void)notifyOutputRecoveryFailed
{
  NSMutableArray *blocks = [NSMutableArray array];
  for (AudioEngineSourceRegistration *reg in self.sourceRegistrations.allValues) {
    if (reg.onOutputRecoveryFailed != nil) {
      [blocks addObject:[reg.onOutputRecoveryFailed copy]];
    }
  }
  if (blocks.count == 0) {
    return;
  }

  // Invoke after the engine lock is released so player cleanup can detach nodes.
  dispatch_async(dispatch_get_main_queue(), ^{
    for (OnOutputRecoveryFailedBlock onOutputRecoveryFailed in blocks) {
      onOutputRecoveryFailed();
    }
  });
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

/// @brief The input node's current output format, or nil when unusable. Lock held.
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
    AUDIO_LOG_ERROR(
        "Error while activating audio session before input materialization: %{public}@",
        [sessionError debugDescription]);
    return NO;
  }

  AVAudioFormat *inputFormat = [self currentInputConnectionFormat];

  if (inputFormat == nil) {
    return NO;
  }

  if (_voiceProcessingApplied) {
    [self connectVoiceProcessingIOOutput:inputFormat];
  }

  self.inputNode =
      [[AVAudioSinkNode alloc] initWithReceiverBlock:self.inputRegistration.receiverBlock];
  [self.audioEngine attachNode:self.inputNode];
  [self.audioEngine connect:self.audioEngine.inputNode to:self.inputNode format:inputFormat];
  return YES;
}

/// @brief Connects the main mixer to the output in the input's format.
/// @note The voice-processing I/O unit runs both directions and needs matching formats;
/// with its output bus unconnected or implicitly formatted, the engine stops itself.
- (void)connectVoiceProcessingIOOutput:(AVAudioFormat *)format
{
  [self.audioEngine connect:self.audioEngine.mainMixerNode
                         to:self.audioEngine.outputNode
                     format:format];
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
    AUDIO_LOG_ERROR(
        "Error while setting voice processing to %{public}s: %{public}@",
        wantsVoiceProcessing ? "true" : "false",
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
                       onOutputRecoveryFailed:(OnOutputRecoveryFailedBlock)onOutputRecoveryFailed
{
  std::scoped_lock lock(_engineLock);
  [self createAudioEngineIfNeeded];

  NSString *sourceNodeId = [[NSUUID UUID] UUIDString];
  AudioEngineSourceRegistration *registration = [[AudioEngineSourceRegistration alloc] init];
  registration.renderBlock = renderBlock;
  registration.sampleRate = sampleRate;
  registration.channelCount = channelCount;
  registration.onOutputRecoveryFailed = onOutputRecoveryFailed;

  self.sourceRegistrations[sourceNodeId] = registration;
  [self materializeSourceNodeWithId:sourceNodeId];

  return sourceNodeId;
}

- (void)detachSourceNodeWithId:(NSString *)sourceNodeId
{
  std::scoped_lock lock(_engineLock);
  AVAudioSourceNode *sourceNode = self.sourceNodes[sourceNodeId];

  if (self.sourceRegistrations[sourceNodeId] == nil) {
    AUDIO_LOG_WARNING("No source node found with ID: %{public}@", sourceNodeId);
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
                     onInputNotification:(void (^)(AudioEngineInputNotification))onInputNotification
{
  std::scoped_lock lock(_engineLock);
  [self createAudioEngineIfNeeded];

  if (self.inputRegistration != nil || self.inputNode != nil) {
    [self resetInputNode];
  }

  AudioEngineInputRegistration *registration = [[AudioEngineInputRegistration alloc] init];
  registration.receiverBlock = receiverBlock;
  registration.voiceProcessingEnabled = voiceProcessingEnabled;
  registration.onInputNotification = onInputNotification;
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
  std::scoped_lock lock(_engineLock);
  return [self liveInputFormat];
}

- (bool)onInterruptionBegin
{
  std::scoped_lock lock(_engineLock);
  AUDIO_LOG_TRACE("onInterruptionBegin");

  if (self.state != AudioEngineState::AudioEngineStateRunning) {
    AUDIO_LOG_DEBUG(
        "onInterruptionBegin ignored engineState=%{public}s engineRunning=%{public}s",
        audioEngineStateName(self.state),
        logBool(self.audioEngine != nil && [self.audioEngine isRunning]));
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateInterrupted;
  AUDIO_LOG_DEBUG(
      "onInterruptionBegin engineState=running -> interrupted engineRunning=%{public}s",
      logBool(self.audioEngine != nil && [self.audioEngine isRunning]));
  return true;
}

- (void)onSessionDeactivated
{
  std::scoped_lock lock(_engineLock);
  AUDIO_LOG_TRACE("onSessionDeactivated");
  BOOL hadTrackedGraph = [self hasTrackedGraph];
  BOOL hadActiveState = self.state != AudioEngineState::AudioEngineStateIdle;

  if (!hadActiveState && !hadTrackedGraph) {
    AUDIO_LOG_DEBUG(
        "onSessionDeactivated ignored engineState=%{public}s", audioEngineStateName(self.state));
    return;
  }

  self.sessionDeactivationInvalidatedGraph = true;

  if (hadTrackedGraph) {
    self.graphNeedsRebuild = true;
  }

  if (self.audioEngine != nil && [self.audioEngine isRunning]) {
    [self.audioEngine pause];
  }

  self.state = AudioEngineState::AudioEngineStatePaused;
  AUDIO_LOG_DEBUG(
      "onSessionDeactivated engineState=%{public}s graphNeedsRebuild=%{public}s sessionDeactivationInvalidatedGraph=%{public}s",
      audioEngineStateName(self.state),
      logBool(self.graphNeedsRebuild),
      logBool(self.sessionDeactivationInvalidatedGraph));
}

- (void)markSessionDeactivationInvalidatedGraph
{
  std::scoped_lock lock(_engineLock);
  self.sessionDeactivationInvalidatedGraph = YES;
}

- (void)markGraphNeedsRebuild
{
  std::scoped_lock lock(_engineLock);
  AUDIO_LOG_TRACE("markGraphNeedsRebuild");
  self.graphNeedsRebuild = true;
  AUDIO_LOG_DEBUG(
      "markGraphNeedsRebuild engineState=%{public}s graphNeedsRebuild=true",
      audioEngineStateName(self.state));
}

- (void)onRouteChange
{
  std::scoped_lock lock(_engineLock);
  if (![self hasTrackedGraph] || self.state == AudioEngineState::AudioEngineStateRunning) {
    return;
  }

  self.graphNeedsRebuild = true;
}

// According to <https://developer.apple.com/documentation/avfaudio/avaudiosession/interruptionoptions/shouldresume>:
// * `shouldResume` only refers to resuming the playback.
// * We choose to resume recording unconditionally (but see below).
// Below decision tree reflects this policy.
// On the long term it would be useful to make this policy configurable.
static BOOL
shouldEngineResumeAfterInterruption(BOOL shouldPlaybackResume, BOOL hasInput, BOOL hasSources)
{
  if (hasInput) {
    if (hasSources) {
      if (shouldPlaybackResume) {
        // Resume recording and playback.
        return YES;
      } else {
        // TODO: Ideally, we would only resume recording and not playback,
        // but it would be complex to implement and the usecase is not common,
        // therefore not resuming the engine is safer.
        return NO;
      }
    } else {
      // There is only recording to resume.
      return YES;
    }
  } else {
    // There is only playback to resume.
    return hasSources && shouldPlaybackResume;
  }
}

- (AudioEngineInterruptionEndOutcome)onInterruptionEnd:(bool)shouldResume
{
  AUDIO_LOG_TRACE("onInterruptionEnd shouldResume=%{public}s", logBool(shouldResume));

  AudioEngineInterruptionEndOutcome outcome;
  BOOL readyToStart = NO;
  dispatch_block_t notifyInput = nil;

  {
    std::scoped_lock lock(_engineLock);
    outcome = [self rebuildAfterInterruption:shouldResume readyToStart:&readyToStart];

    if (outcome == AudioEngineInterruptionEndOutcomePaused || readyToStart) {
      notifyInput = [self inputNotificationBlock:AudioEngineInputNotificationHardwareChanged];
    }
  }

  // Before starting: the input's consumers must match the new format before the first render.
  if (notifyInput != nil) {
    notifyInput();
  }

  if (!readyToStart) {
    AUDIO_LOG_DEBUG(
        "onInterruptionEnd outcome=%{public}s readyToStart=false notifiedInput=%{public}s engineState=%{public}s",
        audioEngineInterruptionEndOutcomeName(outcome),
        logBool(notifyInput != nil),
        audioEngineStateName([self getState]));
    return outcome;
  }

  std::scoped_lock lock(_engineLock);
  outcome = [self startAfterInterruption];
  AUDIO_LOG_DEBUG(
      "onInterruptionEnd outcome=%{public}s readyToStart=true notifiedInput=true engineState=%{public}s",
      audioEngineInterruptionEndOutcomeName(outcome),
      audioEngineStateName(self.state));
  return outcome;
}

/// @brief Rebuilds an interrupted engine for resuming, or leaves it `Paused` when policy says
/// not to. Lock held.
/// @discussion Sets @p readyToStart when the rebuilt engine should be started; the returned
/// outcome is final only when it is not set.
- (AudioEngineInterruptionEndOutcome)rebuildAfterInterruption:(bool)shouldResume
                                                 readyToStart:(BOOL *)readyToStart
{
  NSError *error = nil;
  *readyToStart = NO;

  AUDIO_LOG_TRACE("rebuildAfterInterruption");
  AUDIO_LOG_DEBUG(
      "rebuildAfterInterruption engineState=%{public}s shouldResume=%{public}s hasInput=%{public}s hasSources=%{public}s graphNeedsRebuild=%{public}s sessionDeactivationInvalidatedGraph=%{public}s engineRunning=%{public}s",
      audioEngineStateName(self.state),
      logBool(shouldResume),
      logBool([self hasInput]),
      logBool([self hasSources]),
      logBool(self.graphNeedsRebuild),
      logBool(self.sessionDeactivationInvalidatedGraph),
      logBool(self.audioEngine != nil && [self.audioEngine isRunning]));

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    return AudioEngineInterruptionEndOutcomeNoOp;
  }

  const BOOL resumeEngine =
      shouldEngineResumeAfterInterruption(shouldResume, [self hasInput], [self hasSources]);
  AUDIO_LOG_DEBUG("rebuildAfterInterruption resumeEngine=%{public}s", logBool(resumeEngine));

  if (!resumeEngine) {
    [self stopEngine];
    [self rebuildAudioEngine];
    self.state = AudioEngineState::AudioEngineStatePaused;
    return AudioEngineInterruptionEndOutcomePaused;
  }

  if (![self.sessionManager ensureActive:true error:&error]) {
    AUDIO_LOG_ERROR(
        "Error while activating audio session after interruption: %{public}@",
        [error debugDescription]);
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  [self stopEngine];
  [self rebuildAudioEngine];

  if (self.inputRegistration != nil && self.inputNode == nil) {
    AUDIO_LOG_ERROR(
        "Error while materializing the audio input node after interruption: missing live input format");
    self.state = AudioEngineState::AudioEngineStateInterrupted;
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  // Still Interrupted: startAfterInterruption: runs only if nothing else claimed the engine.
  self.state = AudioEngineState::AudioEngineStateInterrupted;
  *readyToStart = YES;
  AUDIO_LOG_DEBUG(
      "rebuildAfterInterruption readyToStart engineState=%{public}s inputNode=%{public}s",
      audioEngineStateName(self.state),
      logBool(self.inputNode != nil));
  return AudioEngineInterruptionEndOutcomeStillInterrupted;
}

/// @brief Starts the engine rebuilt by rebuildAfterInterruption:, unless it was stopped or
/// started meanwhile. Lock held.
- (AudioEngineInterruptionEndOutcome)startAfterInterruption
{
  AUDIO_LOG_TRACE("startAfterInterruption");
  AUDIO_LOG_DEBUG(
      "startAfterInterruption engineState=%{public}s engineRunning=%{public}s",
      audioEngineStateName(self.state),
      logBool(self.audioEngine != nil && [self.audioEngine isRunning]));

  if (self.state == AudioEngineState::AudioEngineStateRunning && [self.audioEngine isRunning]) {
    return AudioEngineInterruptionEndOutcomeRunning;
  }

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    AUDIO_LOG_DEBUG(
        "startAfterInterruption no-op engineState=%{public}s", audioEngineStateName(self.state));
    return AudioEngineInterruptionEndOutcomeNoOp;
  }

  NSError *error = nil;
  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    AUDIO_LOG_ERROR(
        "Error while restarting the audio engine after interruption: %{public}@",
        [error debugDescription]);
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
  return AudioEngineInterruptionEndOutcomeRunning;
}

/// @brief The registered input's handler bound to @p notification, or nil without one.
/// @discussion Run it only after releasing `_engineLock`: the handler calls back into the engine.
// TODO: replace the returned blocks with explicit post-lock effects in the engine refactor.
- (dispatch_block_t)inputNotificationBlock:(AudioEngineInputNotification)notification
{
  void (^handler)(AudioEngineInputNotification) = self.inputRegistration.onInputNotification;

  if (handler == nil) {
    return nil;
  }

  return ^{ handler(notification); };
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

- (bool)isCurrentEngine:(AVAudioEngine *)engine
{
  std::scoped_lock lock(_engineLock);
  return engine != nil && engine == self.audioEngine;
}

/// @brief Starts the rebuilt engine, unless it was paused, stopped or started meanwhile. Lock held.
- (void)resumeAfterRebuild
{
  if (self.state != AudioEngineState::AudioEngineStateRunning || ![self hasTrackedGraph] ||
      [self.audioEngine isRunning]) {
    return;
  }

  if (![self startEngine]) {
    [self settleStateAfterFailedStart];
    [self notifyOutputRecoveryFailed];
  }
}

/// @brief True when the graph may no longer match the hardware: interrupted, session
/// deactivated, or hardware changed while stopped.
- (BOOL)mustRebuildBeforeStart
{
  return self.state == AudioEngineState::AudioEngineStateInterrupted || self.graphNeedsRebuild ||
      self.sessionDeactivationInvalidatedGraph;
}

/// @brief Stops the engine if running, rebuilds it, and forgets a past session deactivation;
/// leaves it stopped. Lock held.
/// @discussion Never starts, so rebuilding and starting cannot recurse into each other.
- (void)rebuildGraphForCurrentHardware
{
  if ([self.audioEngine isRunning]) {
    [self.audioEngine stop];
  }

  [self rebuildAudioEngine];
  self.sessionDeactivationInvalidatedGraph = false;
}

/// @brief After a failed start: `Paused` with a tracked graph, `Idle` without one.
/// @discussion `Interrupted` is kept: the interruption-end path and the foreground retry
/// recover only from that state.
- (void)settleStateAfterFailedStart
{
  if (self.state == AudioEngineState::AudioEngineStateInterrupted) {
    return;
  }

  self.state = [self hasTrackedGraph] ? AudioEngineState::AudioEngineStatePaused
                                      : AudioEngineState::AudioEngineStateIdle;
}

/// @brief Destroys the engine and creates a new one with the tracked graph, left stopped. Lock held.
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
    AUDIO_LOG_ERROR("Error while activating audio session: %{public}@", [error debugDescription]);
    return false;
  }

  // Never rebuilds; callers check `mustRebuildBeforeStart` first.
  [self materializeTrackedNodesIfNeeded];

  if (self.inputRegistration != nil && self.inputNode == nil) {
    AUDIO_LOG_ERROR("Error while materializing the audio input node: missing live input format");
    return false;
  }

  [self.audioEngine prepare];
  [self.audioEngine startAndReturnError:&error];

  if (error != nil) {
    AUDIO_LOG_ERROR("Error while starting the audio engine: %{public}@", [error debugDescription]);
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

  if (![self hasTrackedGraph]) {
    return false;
  }

  if ([self mustRebuildBeforeStart]) {
    [self rebuildGraphForCurrentHardware];
  }

  if ([self startEngine]) {
    return true;
  }

  [self settleStateAfterFailedStart];
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

  if ([self hasInput] || [self hasSources]) {
    return;
  }

  if (self.state != AudioEngineState::AudioEngineStateIdle) {
    [self stopEngine];
  }
}

- (void)restartAudioEngine
{
  BOOL shouldResume = NO;
  dispatch_block_t notifyInput = nil;

  {
    std::scoped_lock lock(_engineLock);

    // The engine is created lazily on first node attach. Apps that only use
    // session management and notifications never have one, and a system-driven
    // restart (media services reset, configuration change) must not create it.
    if (![self hasTrackedGraph] && self.audioEngine == nil) {
      return;
    }

    shouldResume = self.state == AudioEngineState::AudioEngineStateRunning;
    [self rebuildGraphForCurrentHardware];

    if (shouldResume) {
      notifyInput = [self inputNotificationBlock:AudioEngineInputNotificationHardwareChanged];
    }
  }

  // Before starting: the input's consumers must match the new format before the first render.
  if (notifyInput != nil) {
    notifyInput();
  }

  if (shouldResume) {
    std::scoped_lock lock(_engineLock);
    [self resumeAfterRebuild];
  }
}

- (void)logAudioEngineState
{
#if defined(DEBUG) && DEBUG
  std::scoped_lock lock(_engineLock);
  AVAudioSession *session = [AVAudioSession sharedInstance];

  AUDIO_LOG_DEBUG(
      "Engine isRunning: %{public}s, isInManualRenderingMode: %{public}s",
      self.audioEngine.isRunning ? "true" : "false",
      self.audioEngine.isInManualRenderingMode ? "true" : "false");

  AUDIO_LOG_DEBUG(
      "Session category: %{public}@, mode: %{public}@, sampleRate: %f Hz, IO buffer duration: %f s",
      session.category,
      session.mode,
      session.sampleRate,
      session.IOBufferDuration);

  AVAudioSessionRouteDescription *route = session.currentRoute;

  for (AVAudioSessionPortDescription *output in route.outputs) {
    AUDIO_LOG_DEBUG("Route output: %{public}@ (%{public}@)", output.portType, output.portName);
  }

  for (AVAudioSessionPortDescription *input in route.inputs) {
    AUDIO_LOG_DEBUG("Route input: %{public}@ (%{public}@)", input.portType, input.portName);
  }

  AVAudioFormat *format = [self.audioEngine.outputNode inputFormatForBus:0];
  AUDIO_LOG_DEBUG(
      "Engine output format: %.0f Hz, %u channels", format.sampleRate, format.channelCount);
#endif
}

@end
