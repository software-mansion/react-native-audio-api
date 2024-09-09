#pragma once

#import <limits.h>

#import "AudioNode.h"

@interface AudioDestinationNode : AudioNode

- (instancetype)initWithContext:(AudioContext *)context;

@end
