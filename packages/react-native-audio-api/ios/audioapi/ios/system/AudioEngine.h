#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioEngine : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableDictionary *sourceNodes;
@property (nonatomic, strong) NSMutableDictionary *sourceFormats;
@property (nonatomic, strong) AVAudioSinkNode *inputNode;

+ (instancetype)sharedInstance;
- (void)cleanup;
- (bool)rebuildAudioEngine;
- (void)startEngine;
- (void)stopEngine;
- (bool)isRunning;

- (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
- (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

- (void)attachInputNode:(AVAudioSinkNode *)inputNode;
- (void)detachInputNode;

- (void)startIfNecessary;
- (void)stopIfNecessary;

@end
