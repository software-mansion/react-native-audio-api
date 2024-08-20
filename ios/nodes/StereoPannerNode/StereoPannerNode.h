#pragma once

#import "AudioNode.h"
#import "AudioParam.h"

@interface StereoPannerNode : AudioNode

@property (nonatomic, strong) AudioParam *audioParam;

- (instancetype)initWithContext:(AudioContext *)context;
- (void)clean;

@end
