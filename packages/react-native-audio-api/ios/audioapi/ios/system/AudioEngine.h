#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioSessionManager;

@interface AudioEngine : NSObject

@property (nonatomic, assign) bool isInterrupted;
@property (nonatomic, assign) bool isSupposedToBeRunning;
@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableDictionary *sourceNodes;
@property (nonatomic, strong) NSMutableDictionary *sourceFormats;
@property (nonatomic, strong) NSMutableDictionary *sourceStates;
@property (nonatomic, strong) AVAudioSinkNode *inputNode;
@property (nonatomic, weak) AudioSessionManager *sessionManager;

- (instancetype)initWithAudioSessionManager:(AudioSessionManager *)sessionManager;

+ (instancetype)sharedInstance;
- (void)cleanup;
- (bool)rebuildAudioEngineAndStartIfNecessary;
- (bool)restartAudioEngine;
- (bool)startEngine;
- (void)stopEngine;
- (void)pauseEngine:(NSString *)sourceNodeId;
- (bool)isRunning;
- (void)markAsInterrupted;
- (void)unmarkAsInterrupted;
- (bool)isSupposedToRun;

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNode:(AVAudioSinkNode *)inputNode;
- (void)detachInputNode;

- (void)logAudioEngineState;

- (bool)startIfNecessary;
- (void)stopIfNecessary;
- (void)pauseIfNecessary;

@end
