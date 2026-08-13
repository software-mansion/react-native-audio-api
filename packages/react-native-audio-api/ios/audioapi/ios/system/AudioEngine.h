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

- (void)attachInputNodeWithReceiverBlock:(AVAudioSinkNodeReceiverBlock)receiverBlock;
- (void)detachInputNode;
- (AVAudioFormat *)getLiveInputFormat;

- (void)onInterruptionBegin;
- (void)onInterruptionEnd:(bool)shouldResume;
- (void)onSessionDeactivated;
- (void)markSessionDeactivationInvalidatedGraph;

- (AudioEngineState)getState;
- (bool)isEngineRunning;

/// @brief Whether a restart refused by the system is still waiting to be retried.
- (bool)isRestartPending;

/// @brief Immediately re-attempts a restart that the system previously refused.
///
/// The operating system can reject an engine restart triggered by a route or
/// configuration change, most notably while the device is locked with an input node
/// attached, or while another application holds the audio session. Such a refusal
/// leaves the engine stopped with a restart pending, retried on a bounded backoff.
/// Callers use this method to retry as soon as conditions are known to have improved
/// (the application returned to the foreground, the route changed again) instead of
/// waiting for the next scheduled attempt, and to grant a fresh retry budget once the
/// scheduled ones are exhausted.
///
/// Does nothing when no restart is pending.
- (void)retryPendingRestartIfNeeded;

- (bool)startIfNecessary;
- (void)pauseIfNecessary;
- (void)stopIfNecessary;

- (void)stopIfPossible;

- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
