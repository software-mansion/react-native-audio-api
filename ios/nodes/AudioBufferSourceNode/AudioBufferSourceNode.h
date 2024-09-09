#pragma once

#import "AudioContext.h"
#import "AudioScheduledSourceNode.h"

@interface AudioBufferSourceNode : AudioScheduledSourceNode

@property (nonatomic, assign) Boolean isPlaying;
@property (nonatomic, assign) bool loop;

- (instancetype)initWithContext:(AudioContext *)context;

- (void)cleanup;

- (void)setLoop:(bool)loop;

- (bool)getLoop;

@end
