#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <mach/mach_time.h>
#import "ContextState.h"

@class OscillatorNode;

@interface AudioContext : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, assign) uint64_t contextStartTime;
@property (nonatomic, assign) ContextStateEnum state;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init;

- (void)close;

- (void)cleanup;

- (double)getCurrentTime;

- (NSString *)getState;

- (double)getSampleRate;

@end
