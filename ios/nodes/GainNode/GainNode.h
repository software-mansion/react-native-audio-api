#pragma once

#import "AudioNode.h"

@interface GainNode : AudioNode

@property (nonatomic, assign) float gain;

- (instancetype)init:(AudioContext *)context;

- (void)setGain:(float)gain;

- (float)getGain;

@end
