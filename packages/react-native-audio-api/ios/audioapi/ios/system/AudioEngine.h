#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@class AudioSessionManager;

@interface AudioEngine : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableDictionary *sourceNodes;
@property (nonatomic, strong) NSMutableDictionary *sourceFormats;
@property (nonatomic, strong) NSMutableDictionary *sourceStates;
@property (nonatomic, strong) AVAudioSinkNode *inputNode;
@property (nonatomic, weak) AudioSessionManager *sessionManager;

- (instancetype)initWithAudioSessionManager:(AudioSessionManager *)sessionManager;

+ (instancetype)sharedInstance;
- (void)cleanup;
- (bool)rebuildAudioEngine;
- (void)startEngine;
- (void)stopEngine;
- (void)pauseEngine:(NSString *)sourceNodeId;
- (bool)isRunning;

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNode:(AVAudioSinkNode *)inputNode;
- (void)detachInputNode;

@end
