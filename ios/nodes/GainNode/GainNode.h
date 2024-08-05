#pragma once

#import "AudioNode.h"
#import "AudioParam.h"

@interface GainNode : AudioNode

@property (nonatomic, strong) AudioParam *audioParam;

- (instancetype)initWithContext:(AudioContext *)context;

@end
