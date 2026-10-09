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
  AudioEngineInputNotificationCaptureLost
};

/// Result of `onInterruptionEnd:`. Distinguishes a no-op from a failed resume that stays Interrupted.
typedef NS_ENUM(NSInteger, AudioEngineInterruptionEndOutcome) {
  AudioEngineInterruptionEndOutcomeNoOp = 0,
  AudioEngineInterruptionEndOutcomeRunning,
  AudioEngineInterruptionEndOutcomePaused,
  AudioEngineInterruptionEndOutcomeStillInterrupted
};

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
                                 channelCount:(AVAudioChannelCount)channelCount;
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
/// Responds to an audio route change without restarting the engine. A running engine is left
/// as it is: when a route change alters the hardware sample rate or channel count, the engine
/// stops itself and posts `AVAudioEngineConfigurationChangeNotification`, whose handler
/// rebuilds it. A stopped engine with a tracked graph is marked for rebuild instead, because
/// whether a stopped engine observes the change and posts that notification is not
/// documented; the next start then builds the graph against the current hardware.
- (void)onRouteChange;

- (AudioEngineState)getState;
- (bool)isEngineRunning;
- (bool)isInUse;
/// True when @p engine is the AVAudioEngine this object currently drives. False for nil, for
/// engines owned by other code in the process, and for engines this object has already
/// replaced during a rebuild.
- (bool)isCurrentEngine:(AVAudioEngine *)engine;

- (bool)startIfNecessary;
- (void)pauseIfNecessary;
- (void)stopIfNecessary;

- (void)stopIfPossible;

- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
