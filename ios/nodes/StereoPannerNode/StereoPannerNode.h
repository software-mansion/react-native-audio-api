#pragma once

#import "AudioNode.h"
#import "AudioParam.h"

@interface StereoPannerNode : AudioNode

@property (nonatomic, strong) AudioParam *audioParam;

- (instancetype)init:(AudioContext *)context;

@end
