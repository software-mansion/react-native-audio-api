#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import "AudioParam.h"
#import "AudioScheduledSourceNode.h"
#import "WaveType.h"

static const double OCTAVE_IN_CENTS = 12 * 100;

@interface OscillatorNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AudioParam *frequencyParam;
@property (nonatomic, strong) AudioParam *detuneParam;
@property (nonatomic, assign) WaveTypeEnum waveType;
@property (nonatomic, strong) AVAudioSourceNode *sourceNode;
@property (nonatomic, assign) float phase;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end
