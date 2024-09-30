#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <mach/mach_time.h>

#import "AudioBufferSourceNode.h"
#import "AudioDestinationNode.h"
#import "BiquadFilterNode.h"
#import "ContextState.h"
#import "GainNode.h"
#import "OscillatorNode.h"
#import "RNAA_AudioBuffer.h"
#import "StereoPannerNode.h"

@class OscillatorNode;
@class GainNode;
@class StereoPannerNode;
@class BiquadFilterNode;
@class AudioBufferSourceNode;
@class RNAA_AudioBuffer;

@interface AudioContext : NSObject

@property (nonatomic, strong) AVAudioEngine *audioEngine;
@property (nonatomic, strong) AudioDestinationNode *destination;
@property (nonatomic, assign) uint64_t contextStartTime;
@property (nonatomic, assign) ContextStateEnum state;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init;

- (void)close;

- (void)cleanup;

- (OscillatorNode *)createOscillator;

- (GainNode *)createGain;

- (StereoPannerNode *)createStereoPanner;

- (BiquadFilterNode *)createBiquadFilter;

- (AudioBufferSourceNode *)createBufferSource;

- (RNAA_AudioBuffer *)createBuffer:(int)numberOfChannels length:(int)length sampleRate:(int)sampleRate;

- (double)getCurrentTime;

- (NSString *)getState;

- (double)getSampleRate;

@end
