#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "OscillatorNode.h"
#import "ContextState.h"
#import <mach/mach_time.h>

@class OscillatorNode;

@interface AudioContext : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) NSMutableArray<OscillatorNode *> *connectedOscillators;
@property (nonatomic, assign) uint64_t contextStartTime;
@property (nonatomic, assign) ContextStateEnum state;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init;

- (void)connectOscillator:(OscillatorNode *)node;

- (void)processNodes;

- (double)getCurrentTime;

- (NSString *)getState;

- (double)getSampleRate;

@end
