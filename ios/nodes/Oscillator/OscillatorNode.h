#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "AudioScheduledSourceNode.h"
#import "WaveType.h"
#import "AudioContext.h"
#import "AudioParam.h"

static const double OCTAVE_IN_CENTS = 12 * 100;

@interface OscillatorNode : AudioScheduledSourceNode

@property(nonatomic, strong) AVAudioFormat *format;
@property(nonatomic, strong) AudioParam *frequencyParam;
@property(nonatomic, strong) AudioParam *detuneParam;
@property(nonatomic, assign) WaveTypeEnum waveType;
@property(nonatomic, assign) Boolean isPlaying;

@property(nonatomic, assign) float deltaTime;
@property(nonatomic, strong) AVAudioSourceNode *sourceNode;
@property(nonatomic, assign) float phase;
// @property (nonatomic, strong) PlaybackParameters* playbackParameters;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)start:(double)time;

- (void)stop:(double)time;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end
