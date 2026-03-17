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

- (instancetype)init;
+ (instancetype)sharedInstance;

- (void)cleanup;

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNode:(AVAudioSinkNode *)inputNode format:(AVAudioFormat *)format;
- (void)detachInputNode;

- (void)onInterruptionBegin;
- (void)onInterruptionEnd:(bool)shouldResume;

- (AudioEngineState)getState;

- (bool)startIfNecessary;
- (void)pauseIfNecessary;
- (void)stopIfNecessary;

- (void)stopIfPossible;

- (void)restartAudioEngine;

- (void)logAudioEngineState;

@end
