#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioSessionManager;

typedef NS_ENUM(NSInteger, AudioEngineState) {
  AudioEngineStateIdle = 0,
  AudioEngineStateRunning,
  AudioEngineStatePaused,
  AudioEngineStateInterrupted
};

typedef NS_ENUM(NSInteger, AudioEngineInputNotification) {
  AudioEngineInputNotificationHardwareChanged = 0,
};

/// Result of `onInterruptionEnd:`. Distinguishes a no-op from a failed resume that stays Interrupted.
typedef NS_ENUM(NSInteger, AudioEngineInterruptionEndOutcome) {
  AudioEngineInterruptionEndOutcomeNoOp = 0,
  AudioEngineInterruptionEndOutcomeRunning,
  AudioEngineInterruptionEndOutcomePaused,
  AudioEngineInterruptionEndOutcomeStillInterrupted
};

typedef void (^OnOutputRecoveryFailedBlock)(void);

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
                     onInputNotification:
                         (void (^)(AudioEngineInputNotification))onInputNotification;
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

- (bool)startIfNecessary;
- (void)pauseIfNecessary;
- (void)stopIfNecessary;

- (void)stopIfPossible;

/// @brief Rebuilds the engine against the current hardware and starts it again if it was
/// running. The input hears `HardwareChanged` between the two, outside the lock, so its
/// consumers match the new format before the first render callback.
- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
