#pragma once

#import "AudioNode.h"

@interface StereoPannerNode : AudioNode

@property (nonatomic, assign) float pan;

- (instancetype)init:(AudioContext *)context;

- (void)setPan:(float)pan;

- (float)getPan;

@end
