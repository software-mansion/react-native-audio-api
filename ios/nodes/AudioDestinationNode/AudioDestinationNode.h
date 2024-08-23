#pragma once

#import "AudioNode.h"

#import <limits.h>

@interface AudioDestinationNode : AudioNode

- (instancetype)initWithContext:(AudioContext *)context;

@end
