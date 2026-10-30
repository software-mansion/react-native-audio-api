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
  /// The engine restarted on a rebuilt graph; the live input format may have changed.
  AudioEngineInputNotificationHardwareChanged = 0
};

/// Delivered asynchronously on the main queue, after the engine lock is released, so the
/// source owner can detach its node.
typedef NS_ENUM(NSInteger, AudioEngineOutputNotification) {
  /// The engine could not be restarted; the source's stream has failed.
  AudioEngineOutputNotificationRecoveryFailed = 0
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
                         onOutputNotification:
                             (nullable void (^)(AudioEngineOutputNotification))onOutputNotification;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock
                  voiceProcessingEnabled:(BOOL)voiceProcessingEnabled
                     onInputNotification:
                         (void (^)(AudioEngineInputNotification))onInputNotification;
- (void)detachInputNode;
- (AVAudioFormat *)getLiveInputFormat;

/// Marks the session inactive, since the system deactivated it.
/// @return true if the engine transitioned from Running to Interrupted.
- (bool)onInterruptionBegin;
/// Leaves `Interrupted` for `Idle` and marks the graph for rebuilding. Does not start the
/// engine: deciding what resumes belongs to the owners of the input and the sources, which
/// start it through `startIfNecessary`.
- (void)onInterruptionEnd;
/// The I/O unit stopped because the hardware configuration changed. Rebuilds and restarts a
/// running engine. While `Interrupted`, only marks the graph for rebuilding, so the restart
/// does not race the interruption's end.
- (void)onConfigurationChange;
/// Media services were reset: re-activates a session that was active, then rebuilds and
/// restarts a running engine.
- (void)onMediaServicesReset;
- (void)onSessionDeactivated;
- (void)markSessionDeactivationInvalidatedGraph;
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
