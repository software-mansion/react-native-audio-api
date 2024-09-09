#pragma once

#import "AudioNode.h"
#import "AudioParam.h"

@interface StereoPannerNode : AudioNode

@property (nonatomic, strong) AudioParam *panParam;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

@end
