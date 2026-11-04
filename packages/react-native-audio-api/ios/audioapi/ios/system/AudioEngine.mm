#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>
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
  /// Set when `rebuildAudioEngine` builds a graph with an input; cleared once that input heard
  /// `HardwareChanged` for this exact engine (`notifyInputOfRebuiltGraph:`) or when the engine
  /// is destroyed. No graph is started while it is set, so the input's consumers match the new
  /// format before the first render.
  BOOL _inputAwaitsHardwareChange;
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
- (BOOL)graphIsStale;
- (void)rebuildGraphForCurrentHardware;
- (void)settleStateAfterFailedStart;
- (BOOL)startBuiltEngineLocked;
- (void)resumeAfterRebuild;
- (BOOL)notifyInputOfRebuiltGraph:(std::unique_lock<std::mutex> &)lock;
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
  _inputAwaitsHardwareChange = NO;

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
  // outside of lock because it locks on its own
  (void)audioapi::ActiveRecorderHandle::global().stopAndReturnInfo();

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
    NSLog(
        @"Error while activating audio session before input materialization: %@",
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
  if (self.state != AudioEngineState::AudioEngineStateRunning) {
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateInterrupted;
  return true;
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

- (void)markGraphNeedsRebuild
{
  std::scoped_lock lock(_engineLock);
  self.graphNeedsRebuild = true;
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
  std::unique_lock lock(_engineLock);

  while (true) {
    BOOL readyToStart = NO;
    const AudioEngineInterruptionEndOutcome outcome = [self rebuildAfterInterruption:shouldResume
                                                                        readyToStart:&readyToStart];

    if (outcome == AudioEngineInterruptionEndOutcomePaused) {
      [self notifyInputOfRebuiltGraph:lock];
      return outcome;
    }

    if (!readyToStart) {
      return outcome;
    }

    // A graph replaced while the input was notified is decided again while still Interrupted;
    // one that left Interrupted meanwhile (a start that succeeded) is settled by
    // startAfterInterruption.
    if ([self notifyInputOfRebuiltGraph:lock] ||
        self.state != AudioEngineState::AudioEngineStateInterrupted) {
      return [self startAfterInterruption];
    }
  }
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

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    return AudioEngineInterruptionEndOutcomeNoOp;
  }

  const BOOL resumeEngine =
      shouldEngineResumeAfterInterruption(shouldResume, [self hasInput], [self hasSources]);

  if (!resumeEngine) {
    [self stopEngine];
    [self rebuildAudioEngine];
    self.state = AudioEngineState::AudioEngineStatePaused;
    return AudioEngineInterruptionEndOutcomePaused;
  }

  if (![self.sessionManager ensureActive:true error:&error]) {
    NSLog(@"Error while activating audio session after interruption: %@", [error debugDescription]);
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  [self stopEngine];
  [self rebuildAudioEngine];

  if (self.inputRegistration != nil && self.inputNode == nil) {
    NSLog(
        @"Error while materializing the audio input node after interruption: missing live input format");
    self.state = AudioEngineState::AudioEngineStateInterrupted;
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  // Still Interrupted: startAfterInterruption: runs only if nothing else claimed the engine.
  self.state = AudioEngineState::AudioEngineStateInterrupted;
  *readyToStart = YES;
  return AudioEngineInterruptionEndOutcomeStillInterrupted;
}

/// @brief Starts the engine rebuilt by rebuildAfterInterruption: and let through by
/// `notifyInputOfRebuiltGraph:`, unless it was stopped or started meanwhile. Lock held.
- (AudioEngineInterruptionEndOutcome)startAfterInterruption
{
  if (self.state == AudioEngineState::AudioEngineStateRunning && [self.audioEngine isRunning]) {
    return AudioEngineInterruptionEndOutcomeRunning;
  }

  if (self.state != AudioEngineState::AudioEngineStateInterrupted) {
    return AudioEngineInterruptionEndOutcomeNoOp;
  }

  if (![self startBuiltEngineLocked]) {
    return AudioEngineInterruptionEndOutcomeStillInterrupted;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
  return AudioEngineInterruptionEndOutcomeRunning;
}

/// @brief Lets the input catch up with the graph `rebuildAudioEngine` built for it. Lock held on
/// entry and exit.
/// @discussion While `_inputAwaitsHardwareChange` is set, runs the input's `HardwareChanged`
/// handler with @p lock released, since the handler calls back into the engine. Others may
/// rebuild, drop or start the engine meanwhile; a build another caller is still notifying is
/// notified again here, and started as soon as this call lets it through. So the handler must
/// leave the input's consumers matching the live format by the time any call returns, even
/// while another call for the same build is in flight.
/// @return Whether the current graph may be started. NO when the engine was replaced while the
/// lock was released: the caller decides again.
- (BOOL)notifyInputOfRebuiltGraph:(std::unique_lock<std::mutex> &)lock
{
  if (!_inputAwaitsHardwareChange) {
    return YES;
  }

  AVAudioEngine *notifiedEngine = self.audioEngine;
  void (^onInputNotification)(AudioEngineInputNotification) =
      self.inputRegistration.onInputNotification;

  if (onInputNotification != nil) {
    lock.unlock();
    onInputNotification(AudioEngineInputNotificationHardwareChanged);
    lock.lock();
  }

  if (self.audioEngine != notifiedEngine) {
    return NO;
  }

  _inputAwaitsHardwareChange = NO;
  return YES;
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

/// @brief True when the graph may no longer match the hardware: interrupted, or marked stale.
- (BOOL)mustRebuildBeforeStart
{
  return self.state == AudioEngineState::AudioEngineStateInterrupted || [self graphIsStale];
}

/// @brief True when the graph was marked for rebuild: session deactivated, or hardware changed
/// while stopped.
- (BOOL)graphIsStale
{
  return self.graphNeedsRebuild || self.sessionDeactivationInvalidatedGraph;
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
  _inputAwaitsHardwareChange = [self hasInput];
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

  // Never rebuilds; callers check `mustRebuildBeforeStart` first.
  [self materializeTrackedNodesIfNeeded];

  if (self.inputRegistration != nil && self.inputNode == nil) {
    NSLog(@"Error while materializing the audio input node: missing live input format");
    return false;
  }

  if (![self startBuiltEngineLocked]) {
    return false;
  }

  self.state = AudioEngineState::AudioEngineStateRunning;
  self.sessionDeactivationInvalidatedGraph = false;
  return true;
}

/// @brief Prepares and starts the current engine. Lock held.
/// @discussion Every start goes through here, and only its YES lets the state become `Running`:
/// `startAndReturnError:` may return NO without setting an error.
- (BOOL)startBuiltEngineLocked
{
  NSError *error = nil;
  [self.audioEngine prepare];

  if ([self.audioEngine startAndReturnError:&error]) {
    return YES;
  }

  NSLog(@"[AudioEngine] Error while starting the audio engine: %@", [error debugDescription]);
  return NO;
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
  std::unique_lock lock(_engineLock);
  BOOL mustRebuild = [self mustRebuildBeforeStart];
  BOOL inputNotified = NO;

  // The guards run again after the input was notified: the graph may have been started or
  // emptied while the lock was released.
  while (true) {
    if (self.state == AudioEngineState::AudioEngineStateRunning && self.audioEngine != nil &&
        [self.audioEngine isRunning]) {
      [self materializeTrackedNodesIfNeeded];
      return true;
    }

    if (![self hasTrackedGraph]) {
      return false;
    }

    if (inputNotified) {
      break;
    }

    if (mustRebuild) {
      [self rebuildGraphForCurrentHardware];
    }

    inputNotified = [self notifyInputOfRebuiltGraph:lock];
    mustRebuild = !inputNotified && [self graphIsStale];
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
  std::unique_lock lock(_engineLock);

  // The engine is created lazily on first node attach. Apps that only use
  // session management and notifications never have one, and a system-driven
  // restart (media services reset, configuration change) must not create it.
  if (![self hasTrackedGraph] && self.audioEngine == nil) {
    return;
  }

  const BOOL shouldResume = self.state == AudioEngineState::AudioEngineStateRunning;
  [self rebuildGraphForCurrentHardware];

  if (!shouldResume) {
    return;
  }

  while (![self notifyInputOfRebuiltGraph:lock]) {
    if (self.state != AudioEngineState::AudioEngineStateRunning) {
      return;
    }

    if ([self graphIsStale]) {
      [self rebuildGraphForCurrentHardware];
    }
  }

  [self resumeAfterRebuild];
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
