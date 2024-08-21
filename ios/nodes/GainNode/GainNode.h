#pragma once

#import "AudioNode.h"
#import "AudioParam.h"

@interface GainNode : AudioNode

@property (nonatomic, strong) AudioParam *gainParam;

- (instancetype)initWithContext:(AudioContext *)context;
- (void)cleanup;

@end
