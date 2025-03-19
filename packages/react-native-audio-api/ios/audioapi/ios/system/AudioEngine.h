#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioEngine : NSObject

- (instancetype)init;
+ (void)cleanup;

+ (bool)rebuildAudioEngine;
+ (void)startEngine;
+ (void)stopEngine;
+ (bool)isRunning;

+ (NSString *)attachSourceNode:(AVAudioSourceNode *)sourceNode format:(AVAudioFormat *)format;
+ (void)detachSourceNodeWithId:(NSString *)sourceNodeId;

@end
