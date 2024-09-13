#pragma once

#import "AudioContext.h"
#import "AudioNode.h"

@interface AudioScheduledSourceNode : AudioNode

@property (nonatomic, assign) Boolean isPlaying;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)start:(double)time;

- (void)stop:(double)time;

- (void)startPlayback;

- (void)stopPlayback;

@end
