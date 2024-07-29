#pragma once

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "AudioScheduledSourceNode.h"
#import "WaveType.h"
#import "AudioContext.h"

static const double OCTAVE_IN_CENTS = 12 * 100;

@interface OscillatorNode : AudioScheduledSourceNode

@property (nonatomic, strong) AVAudioPlayerNode *playerNode;
@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) WaveTypeEnum waveType;
@property (nonatomic, assign) float frequency;
@property (nonatomic, assign) float detune;
@property (nonatomic, assign) double sampleRate;

- (instancetype)init:(AudioContext *)context;

- (void)start;

- (void)stop;

- (void)setFrequency:(float)frequency;

- (float)getFrequency;

- (void)setDetune:(float)detune;

- (float)getDetune;

- (void)setType:(NSString *)type;

- (NSString *)getType;

@end
