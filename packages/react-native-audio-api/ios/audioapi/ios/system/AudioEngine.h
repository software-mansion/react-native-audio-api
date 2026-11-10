#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioSessionManager;

// TODO: add more granular lint rules selection depending on source type
// NOLINTBEGIN(modernize-use-using, cppcoreguidelines-use-enum-class, performance-enum-size, modernize-redundant-void-arg)
typedef NS_ENUM(NSInteger, AudioEngineState) {
  AudioEngineStateIdle = 0,
  AudioEngineStateRunning,
  AudioEngineStatePaused,
  AudioEngineStateInterrupted
};

typedef NS_ENUM(NSInteger, AudioEngineInputNotification) {
  AudioEngineInputNotificationHardwareChanged = 0,
  AudioEngineInputNotificationRecoveryFailed,
};

/// @brief Tells the registered input that something happened to it. The engine calls it after
/// releasing its lock, so the block may call back into the engine.
/// @param notification `HardwareChanged` when the input format may have changed and the input's
/// consumers must re-read it before the next render. `RecoveryFailed` when the engine gave up
/// restarting after an interruption or a route change, so the input stays silent.
/// @param error Why recovery failed, for `RecoveryFailed`; nil for `HardwareChanged`.
typedef void (^AudioEngineInputNotificationBlock)(
    AudioEngineInputNotification notification,
    NSError *_Nullable error);

/// Result of `onInterruptionEnd:`. Distinguishes a no-op from a failed resume that stays Interrupted.
typedef NS_ENUM(NSInteger, AudioEngineInterruptionEndOutcome) {
  AudioEngineInterruptionEndOutcomeNoOp = 0,
  AudioEngineInterruptionEndOutcomeRunning,
  AudioEngineInterruptionEndOutcomePaused,
  AudioEngineInterruptionEndOutcomeStillInterrupted
};

__unused static inline const char *_Nonnull audioEngineStateName(AudioEngineState state)
{
  switch (state) {
    case AudioEngineStateIdle:
      return "idle";
    case AudioEngineStateRunning:
      return "running";
    case AudioEngineStatePaused:
      return "paused";
    case AudioEngineStateInterrupted:
      return "interrupted";
    default:
      return "unknown";
  }
}

__unused static inline const char *_Nonnull audioEngineInterruptionEndOutcomeName(
    AudioEngineInterruptionEndOutcome outcome)
{
  switch (outcome) {
    case AudioEngineInterruptionEndOutcomeNoOp:
      return "noOp";
    case AudioEngineInterruptionEndOutcomeRunning:
      return "running";
    case AudioEngineInterruptionEndOutcomePaused:
      return "paused";
    case AudioEngineInterruptionEndOutcomeStillInterrupted:
      return "stillInterrupted";
    default:
      return "unknown";
  }
}

typedef void (^OnOutputRecoveryFailedBlock)(void);
// NOLINTEND(modernize-use-using, cppcoreguidelines-use-enum-class, performance-enum-size, modernize-redundant-void-arg)

@interface AudioEngine : NSObject

@property (nonatomic, assign) AudioEngineState state;
@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableDictionary *sourceNodes;
@property (nonatomic, strong) NSMutableDictionary *sourceFormats;
@property (nonatomic, strong) AVAudioSinkNode *inputNode;
@property (nonatomic, weak) AudioSessionManager *sessionManager;
@property (nonatomic, assign) BOOL graphNeedsRebuild;
@property (nonatomic, assign) BOOL sessionDeactivationInvalidatedGraph;

- (instancetype)init;
+ (instancetype)sharedInstance;

- (void)cleanup;

- (NSString *)attachSourceNodeWithRenderBlock:(AVAudioSourceNodeRenderBlock)renderBlock
                                   sampleRate:(float)sampleRate
                                 channelCount:(AVAudioChannelCount)channelCount
                       onOutputRecoveryFailed:
                           (nullable OnOutputRecoveryFailedBlock)onOutputRecoveryFailed;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock
                  voiceProcessingEnabled:(BOOL)voiceProcessingEnabled
                     onInputNotification:(AudioEngineInputNotificationBlock)onInputNotification;
- (void)detachInputNode;
- (AVAudioFormat *)getLiveInputFormat;

/// @return true if the engine transitioned from Running to Interrupted.
- (bool)onInterruptionBegin;
- (AudioEngineInterruptionEndOutcome)onInterruptionEnd:(bool)shouldResume;
- (void)onSessionDeactivated;
- (void)markSessionDeactivationInvalidatedGraph;
/// Records that hardware format may have changed while the engine must not rebuild
/// yet (`Interrupted`). The next start or interruption-end resume rebuilds the graph.
- (void)markGraphNeedsRebuild;
/// Marks a stopped graph for rebuild on the next start and leaves a running engine alone:
/// a real format change arrives as AVAudioEngineConfigurationChangeNotification instead.
- (void)onRouteChange;

- (AudioEngineState)getState;
- (bool)isEngineRunning;
- (bool)isInUse;
/// True only for the AVAudioEngine this object drives now: not nil, foreign or replaced ones.
- (bool)isCurrentEngine:(AVAudioEngine *)engine;

/// On false, `error` is an `AudioAPIErrorDomain` error naming the step that failed.
- (bool)startIfNecessary:(NSError **)error;
/// @brief Stops capture without stopping playback. The input stays registered but hears no
/// notifications and reports no live format until `resumeInput:`; its owner drops the audio.
/// While sources play, the running graph is left untouched, so the microphone stays open and
/// voice processing stays on: releasing them takes a rebuild, which cuts the playback. Without
/// playback, or once the last source detaches, the graph is rebuilt without the input, closing
/// the microphone, and the engine is left `Paused`.
- (void)suspendInput;
/// @brief Lets a suspended input capture again. A sink still in the running graph is reused as
/// is; otherwise the engine is rebuilt with the input and started, and the input hears
/// `HardwareChanged` before the start. If that start fails, playing sources are told through
/// their `onOutputRecoveryFailed`.
/// @return false with an `AudioAPIErrorDomain` error when the engine could not be started.
- (bool)resumeInput:(NSError **)error;
- (void)stopIfNecessary;

- (void)stopIfPossible;

/// @brief Rebuilds the engine against the current hardware and starts it again if it was
/// running. The input hears `HardwareChanged` between the two, outside the lock, so its
/// consumers match the new format before the first render callback.
- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
