#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "AudioScheduledSourceNode.h"
#import "WaveType.h"
#import "AudioContext.h"
#import "AudioParam.h"

static const double OCTAVE_IN_CENTS = 12 * 100;

@interface OscillatorNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, strong) AudioParam *frequencyParam;
@property (nonatomic, strong) AudioParam *detuneParam;
@property (nonatomic, assign) WaveTypeEnum waveType;

- (instancetype)init:(AudioContext *)context;

- (void)start;

- (void)stop;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end
