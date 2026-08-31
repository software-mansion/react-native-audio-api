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

- (AudioEngineState)getState;
- (bool)isEngineRunning;
- (bool)hasInputRegistration;

- (bool)startIfNecessary;
- (void)pauseIfNecessary;
- (void)stopIfNecessary;

- (void)stopIfPossible;

- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
